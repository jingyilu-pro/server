//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "manager_service.h"

#include "http_code_message.h"
#include "log/glogger.h"
#include "protocol/gateway.pb.h"

#include <algorithm>
#include <chrono>

namespace
{

int64_t now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string make_trace_id()
{
    const auto ticks = std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();
    return "mgr-" + std::to_string(ticks);
}

} // namespace

ManagerService::ManagerService(const RuntimeConfig& config, std::shared_ptr<IServiceDiscovery> discovery)
    : BasicHttpService("manager", config.server.manager),
      m_config(config),
      m_discovery(std::move(discovery))
{
    m_local_instance.role = "manager";
    m_local_instance.endpoint = config.server.manager;
    m_local_instance.weight = 1;
    m_local_instance.instance_id = name() + std::string("@") + make_endpoint_text(config.server.manager);

    register_handler("/v1/route/login", [this](evhttp_request* request) {
        route_login_async(request);
    });
}

ManagerService::~ManagerService() = default;

bool ManagerService::start()
{
    if(!m_discovery)
    {
        spdlog::error("manager discovery unavailable");
        return false;
    }

    m_stopping.store(false);
    m_registered.store(false);
    m_register_inflight.store(false);
    m_heartbeat_inflight = false;

    {
        std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
        m_start_register_waiting = true;
        m_start_register_done = false;
        m_start_register_success = false;
        m_start_register_error.clear();
    }

    if(!BasicHttpService::start())
    {
        std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
        m_start_register_waiting = false;
        m_start_register_done = false;
        return false;
    }

    m_local_instance.endpoint = endpoint();
    m_local_instance.instance_id = name() + std::string("@") + make_endpoint_text(m_local_instance.endpoint);

    bool register_done = false;
    bool register_success = false;
    std::string register_error;
    {
        const auto timeout_ms = std::max(1000, m_config.redis.op_timeout_ms + 500);
        std::unique_lock<std::mutex> lock(m_lifecycle_mutex);
        register_done = m_lifecycle_cv.wait_for(lock,
                                                std::chrono::milliseconds(timeout_ms),
                                                [this]() { return m_start_register_done; });
        register_success = m_start_register_success;
        register_error = m_start_register_error;
        m_start_register_waiting = false;
    }

    if(!register_done || !register_success)
    {
        spdlog::error("manager register to redis failed: {}",
                      register_done ? register_error : "register_wait_timeout");
        BasicHttpService::stop();
        return false;
    }

    return true;
}

bool ManagerService::stop()
{
    m_stopping.store(true);

    if(m_discovery && m_registered.load())
    {
        {
            std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
            m_stop_unregister_waiting = true;
            m_stop_unregister_requested = true;
            m_stop_unregister_done = false;
            m_stop_unregister_success = false;
            m_stop_unregister_error.clear();
        }

        bool unregister_done = false;
        bool unregister_success = false;
        std::string unregister_error;
        {
            const auto timeout_ms = std::max(1000, m_config.redis.op_timeout_ms + 500);
            std::unique_lock<std::mutex> lock(m_lifecycle_mutex);
            unregister_done = m_lifecycle_cv.wait_for(lock,
                                                      std::chrono::milliseconds(timeout_ms),
                                                      [this]() { return m_stop_unregister_done; });
            unregister_success = m_stop_unregister_success;
            unregister_error = m_stop_unregister_error;
            m_stop_unregister_waiting = false;
        }

        if(!unregister_done || !unregister_success)
        {
            spdlog::warn("manager unregister from redis failed: {}",
                         unregister_done ? unregister_error : "unregister_wait_timeout");
        }
    }

    m_registered.store(false);
    m_register_inflight.store(false);

    return BasicHttpService::stop();
}

void ManagerService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    BasicHttpService::update(delta_time, last_tick_time);
}

void ManagerService::on_event_loop_tick()
{
    if(m_stopping.load())
    {
        if(m_discovery)
        {
            m_discovery->poll();
        }

        bool should_unregister = false;
        {
            std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
            should_unregister = m_stop_unregister_waiting &&
                               m_stop_unregister_requested &&
                               !m_stop_unregister_done;
            if(should_unregister)
            {
                m_stop_unregister_requested = false;
            }
        }
        if(should_unregister)
        {
            unregister_instance_async();
        }
        return;
    }

    if(m_discovery)
    {
        m_discovery->poll();

        if(!m_registered.load() && !m_register_inflight.load())
        {
            register_instance_async();
        }
    }

    auto now = std::chrono::steady_clock::now();
    if(m_discovery && m_registered.load() && !m_heartbeat_inflight &&
       now - m_last_heartbeat >= std::chrono::seconds(std::max(1, m_config.redis.refresh_sec)))
    {
        heartbeat_async();
        m_last_heartbeat = now;
    }
}

coro_task_t ManagerService::register_instance_async()
{
    m_register_inflight.store(true);
    auto* register_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->register_instance(m_local_instance));
    bool register_success = false;
    std::string register_error = "null result";
    if(register_result != nullptr && register_result->success)
    {
        register_success = true;
        register_error.clear();
        m_registered.store(true);
        m_last_heartbeat = std::chrono::steady_clock::now();
    }
    else
    {
        register_error = register_result == nullptr ? "null result" : register_result->error;
        spdlog::warn("manager register to redis failed: {}",
                     register_error);
    }

    bool notify_startup_waiter = false;
    {
        std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
        if(m_start_register_waiting && !m_start_register_done)
        {
            m_start_register_done = true;
            m_start_register_success = register_success;
            m_start_register_error = register_error;
            notify_startup_waiter = true;
        }
    }

    m_register_inflight.store(false);
    if(notify_startup_waiter)
    {
        m_lifecycle_cv.notify_all();
    }
}

coro_task_t ManagerService::unregister_instance_async()
{
    auto* unregister_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->unregister_instance(m_local_instance));
    const bool unregister_success = unregister_result != nullptr && unregister_result->success;
    const std::string unregister_error = unregister_result == nullptr ? "null result" : unregister_result->error;

    if(unregister_success)
    {
        m_registered.store(false);
    }

    bool notify_stop_waiter = false;
    {
        std::lock_guard<std::mutex> lock(m_lifecycle_mutex);
        if(m_stop_unregister_waiting && !m_stop_unregister_done)
        {
            m_stop_unregister_done = true;
            m_stop_unregister_requested = false;
            m_stop_unregister_success = unregister_success;
            m_stop_unregister_error = unregister_error;
            notify_stop_waiter = true;
        }
    }

    if(notify_stop_waiter)
    {
        m_lifecycle_cv.notify_all();
    }
}

EndpointConfig ManagerService::choose_weighted_endpoint(const std::vector<ServiceInstance>& instances,
                                                        const EndpointConfig& fallback,
                                                        std::size_t* round_robin_counter)
{
    if(instances.empty() || round_robin_counter == nullptr)
    {
        return fallback;
    }

    int total_weight = 0;
    for(const auto& instance : instances)
    {
        total_weight += std::max(1, instance.weight);
    }

    if(total_weight <= 0)
    {
        return fallback;
    }

    const std::size_t start = *round_robin_counter % static_cast<std::size_t>(total_weight);
    ++(*round_robin_counter);

    std::size_t cursor = start;
    for(const auto& instance : instances)
    {
        const auto weight = static_cast<std::size_t>(std::max(1, instance.weight));
        if(cursor < weight)
        {
            return instance.endpoint;
        }
        cursor -= weight;
    }

    return instances.front().endpoint;
}

coro_task_t ManagerService::heartbeat_async()
{
    m_heartbeat_inflight = true;
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->heartbeat(m_local_instance));
    if(result == nullptr || !result->success)
    {
        spdlog::warn("manager heartbeat to redis failed: {}",
                     result == nullptr ? "null result" : result->error);
    }
    m_heartbeat_inflight = false;
}

coro_task_t ManagerService::route_login_async(evhttp_request* request)
{
    retain_request(request);

    gateway::RouteLoginRequest route_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!route_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    std::vector<ServiceInstance> login_instances;
    std::vector<ServiceInstance> game_instances;

    if(auto* login_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->list_instances("login"));
       login_result != nullptr && login_result->success)
    {
        login_instances = login_result->instances;
    }
    if(auto* game_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->list_instances("game"));
       game_result != nullptr && game_result->success)
    {
        game_instances = game_result->instances;
    }

    gateway::RouteLoginResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    const auto endpoint_configured = [](const EndpointConfig& endpoint) {
        return !endpoint.host.empty() && endpoint.port > 0;
    };

    const bool login_available = !login_instances.empty() || endpoint_configured(m_config.server.login);
    const bool game_available = !game_instances.empty() || endpoint_configured(m_config.server.game);
    if(!login_available || !game_available)
    {
        if(!login_available && !game_available)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kLoginAndGameServiceUnavailable,
                                                         http_code_message::gateway::message::kLoginAndGameServiceUnavailable);
        }
        else if(!login_available)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kLoginServiceUnavailable,
                                                         http_code_message::gateway::message::kLoginServiceUnavailable);
        }
        else
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kGameServiceUnavailable,
                                                         http_code_message::gateway::message::kGameServiceUnavailable);
        }

        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    auto picked_login = choose_weighted_endpoint(login_instances, m_config.server.login, &m_login_round_robin_counter);
    auto picked_game = choose_weighted_endpoint(game_instances, m_config.server.game, &m_game_round_robin_counter);

    http_code_message::gateway::set_code_message(&response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
    response.mutable_login_endpoint()->set_host(picked_login.host);
    response.mutable_login_endpoint()->set_port(static_cast<uint32_t>(picked_login.port));
    response.mutable_game_endpoint()->set_host(picked_game.host);
    response.mutable_game_endpoint()->set_port(static_cast<uint32_t>(picked_game.port));

    write_protobuf_response(request, response, 200);
    release_request(request);
}

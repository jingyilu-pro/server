//
// Copyright (c) 2024-2025 JingyiLu jingyilupro@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "login_service.h"

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
    return "login-" + std::to_string(ticks);
}

bool is_valid_endpoint(const EndpointConfig& endpoint)
{
    return !endpoint.host.empty() && endpoint.port > 0;
}

ServiceInstance make_fallback_game_instance(const EndpointConfig& endpoint)
{
    ServiceInstance instance;
    instance.role = "game";
    instance.endpoint = endpoint;
    instance.weight = 1;
    instance.instance_id = "game@config";
    return instance;
}

} // namespace

LoginService::LoginService(const RuntimeConfig& config,
                           std::shared_ptr<IServiceDiscovery> discovery,
                           std::shared_ptr<IAccountRepository> account_repository,
                           std::shared_ptr<ITokenProvider> token_provider)
    : BasicHttpService("login", config.server.login, true),
      m_config(config),
      m_discovery(std::move(discovery)),
      m_account_repository(std::move(account_repository)),
      m_token_provider(std::move(token_provider))
{
    m_local_instance.role = "login";
    m_local_instance.weight = 1;

    register_handler("/v1/auth/register", [this](evhttp_request* request) {
        register_async(request);
    });

    register_handler("/v1/auth/login", [this](evhttp_request* request) {
        login_async(request);
    });
}

LoginService::~LoginService() = default;

bool LoginService::start()
{
    if(!BasicHttpService::start())
    {
        return false;
    }

    m_local_instance.endpoint = endpoint();
    m_local_instance.instance_id = std::string(name()) + "@" + make_endpoint_text(m_local_instance.endpoint);

    if(!m_discovery)
    {
        spdlog::error("login discovery unavailable");
        BasicHttpService::stop();
        return false;
    }
    if(!m_account_repository)
    {
        spdlog::error("login account repository unavailable");
        BasicHttpService::stop();
        return false;
    }
    if(!m_account_repository->ready())
    {
        spdlog::error("login account repository not ready");
        BasicHttpService::stop();
        return false;
    }

    m_registered.store(false);
    m_register_inflight.store(false);

    m_last_heartbeat = std::chrono::steady_clock::now();
    return true;
}

bool LoginService::stop()
{
    if(m_discovery && m_registered.load())
    {
        ServiceDiscoveryOpResult unregister_result;
        wait_service_discovery_result(m_discovery.get(),
                                      m_discovery->unregister_instance(m_local_instance),
                                      &unregister_result,
                                      m_config.redis.op_timeout_ms);
    }

    m_registered.store(false);
    m_register_inflight.store(false);

    return BasicHttpService::stop();
}

void LoginService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    BasicHttpService::update(delta_time, last_tick_time);
}

void LoginService::on_event_loop_tick()
{
    if(m_discovery)
    {
        m_discovery->poll();
        if(!m_registered.load() && !m_register_inflight.load())
        {
            register_instance_async();
        }
    }
    if(m_account_repository)
    {
        m_account_repository->poll();
    }

    auto now = std::chrono::steady_clock::now();
    if(m_discovery && m_registered.load() && !m_heartbeat_inflight &&
       now - m_last_heartbeat >= std::chrono::seconds(std::max(1, m_config.redis.refresh_sec)))
    {
        heartbeat_async();
        m_last_heartbeat = now;
    }
}

coro_task_t LoginService::register_instance_async()
{
    m_register_inflight.store(true);
    auto* register_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->register_instance(m_local_instance));
    if(register_result != nullptr && register_result->success)
    {
        m_registered.store(true);
        m_last_heartbeat = std::chrono::steady_clock::now();
    }
    else
    {
        spdlog::warn("login register to redis failed: {}",
                     register_result == nullptr ? "null result" : register_result->error);
    }
    m_register_inflight.store(false);
}

EndpointConfig LoginService::choose_weighted_game_endpoint(const std::vector<ServiceInstance>& instances)
{
    if(instances.empty())
    {
        return {};
    }

    int total_weight = 0;
    for(const auto& instance : instances)
    {
        total_weight += std::max(1, instance.weight);
    }

    if(total_weight <= 0)
    {
        return instances.front().endpoint;
    }

    const auto start = m_game_round_robin_counter % static_cast<std::size_t>(total_weight);
    ++m_game_round_robin_counter;

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

coro_task_t LoginService::heartbeat_async()
{
    m_heartbeat_inflight = true;
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->heartbeat(m_local_instance));
    if(result == nullptr || !result->success)
    {
        spdlog::warn("login heartbeat to redis failed: {}",
                     result == nullptr ? "null result" : result->error);
    }
    m_heartbeat_inflight = false;
}

coro_task_t LoginService::register_async(evhttp_request* request)
{
    retain_request(request);

    gateway::AuthRegisterRequest register_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request, 400, "empty protobuf body");
        release_request(request);
        co_return;
    }
    if(!register_request.ParseFromString(body))
    {
        evhttp_send_error(request, 400, "invalid protobuf");
        release_request(request);
        co_return;
    }

    gateway::AuthRegisterResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    if(m_account_repository == nullptr)
    {
        response.set_code(50001);
        response.set_message("account repository unavailable");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    auto* create_result = dynamic_cast<AccountRepositoryOpResult*>(
        co_await m_account_repository->create_account(register_request.account(), register_request.password()));

    if(create_result == nullptr || !create_result->success)
    {
        response.set_code(50001);
        response.set_message("account repository unavailable");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(!create_result->create_ok)
    {
        response.set_code(40001);
        response.set_message("account already exists or invalid input");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    response.set_code(0);
    response.set_message("registered");
    write_protobuf_response(request, response, 200);
    release_request(request);
}

coro_task_t LoginService::login_async(evhttp_request* request)
{
    retain_request(request);

    gateway::AuthLoginRequest login_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request, 400, "empty protobuf body");
        release_request(request);
        co_return;
    }
    if(!login_request.ParseFromString(body))
    {
        evhttp_send_error(request, 400, "invalid protobuf");
        release_request(request);
        co_return;
    }

    gateway::AuthLoginResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    if(m_account_repository == nullptr)
    {
        response.set_code(50001);
        response.set_message("account repository unavailable");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    auto* verify_result = dynamic_cast<AccountRepositoryOpResult*>(
        co_await m_account_repository->verify_password(login_request.account(), login_request.password()));
    if(verify_result == nullptr || !verify_result->success)
    {
        response.set_code(50001);
        response.set_message("account repository unavailable");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(!verify_result->password_ok)
    {
        response.set_code(40101);
        response.set_message("invalid account or password");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(m_token_provider == nullptr)
    {
        response.set_code(50002);
        response.set_message("token provider unavailable");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    const auto token = m_token_provider->issue(login_request.account(), m_config.jwt.expire_sec);
    if(token.empty())
    {
        response.set_code(50003);
        response.set_message("token issue failed");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    std::vector<ServiceInstance> game_instances;
    if(auto* list_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->list_instances("game"));
       list_result != nullptr && list_result->success)
    {
        game_instances = list_result->instances;
    }
    if(game_instances.empty() && is_valid_endpoint(m_config.server.game))
    {
        game_instances.push_back(make_fallback_game_instance(m_config.server.game));
    }

    if(game_instances.empty())
    {
        response.set_code(50012);
        response.set_message("game service unavailable");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    auto game_endpoint = choose_weighted_game_endpoint(game_instances);

    response.set_code(0);
    response.set_message("ok");
    response.set_jwt(token);
    response.mutable_game_endpoint()->set_host(game_endpoint.host);
    response.mutable_game_endpoint()->set_port(static_cast<uint32_t>(game_endpoint.port));

    write_protobuf_response(request, response, 200);
    release_request(request);
}

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

#include "manager_service.h"

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
        gateway::RouteLoginRequest route_request;
        auto body = read_request_body(request);
        if(body.empty())
        {
            evhttp_send_error(request, 400, "empty protobuf body");
            return;
        }
        if(!route_request.ParseFromString(body))
        {
            evhttp_send_error(request, 400, "invalid protobuf");
            return;
        }

        auto login_instances = m_discovery ? m_discovery->list_instances("login") : std::vector<ServiceInstance>{};
        auto game_instances = m_discovery ? m_discovery->list_instances("game") : std::vector<ServiceInstance>{};

        gateway::RouteLoginResponse response;
        auto picked_login = choose_weighted_endpoint(login_instances, m_config.server.login, &m_login_round_robin_counter);
        auto picked_game = choose_weighted_endpoint(game_instances, m_config.server.game, &m_game_round_robin_counter);

        response.set_code(0);
        response.set_message("ok");
        response.set_trace_id(make_trace_id());
        response.set_server_time_ms(now_ms());
        response.mutable_login_endpoint()->set_host(picked_login.host);
        response.mutable_login_endpoint()->set_port(static_cast<uint32_t>(picked_login.port));
        response.mutable_game_endpoint()->set_host(picked_game.host);
        response.mutable_game_endpoint()->set_port(static_cast<uint32_t>(picked_game.port));

        write_protobuf_response(request, response, 200);
    });
}

ManagerService::~ManagerService() = default;

bool ManagerService::start()
{
    if(!BasicHttpService::start())
    {
        return false;
    }

    if(m_discovery)
    {
        if(!m_discovery->register_instance(m_local_instance))
        {
            spdlog::warn("manager register to redis failed, continue with local fallback route");
        }
    }

    m_last_heartbeat = std::chrono::steady_clock::now();
    return true;
}

bool ManagerService::stop()
{
    if(m_discovery)
    {
        m_discovery->unregister_instance(m_local_instance);
    }

    return BasicHttpService::stop();
}

void ManagerService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    BasicHttpService::update(delta_time, last_tick_time);

    if(!m_discovery)
    {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if(now - m_last_heartbeat >= std::chrono::seconds(std::max(1, m_config.redis.refresh_sec)))
    {
        m_discovery->heartbeat(m_local_instance);
        m_last_heartbeat = now;
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

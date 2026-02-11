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

} // namespace

LoginService::LoginService(const RuntimeConfig& config,
                           std::shared_ptr<IServiceDiscovery> discovery,
                           std::shared_ptr<IAccountRepository> account_repository,
                           std::shared_ptr<ITokenProvider> token_provider)
    : BasicHttpService("login", config.server.login),
      m_config(config),
      m_discovery(std::move(discovery)),
      m_account_repository(std::move(account_repository)),
      m_token_provider(std::move(token_provider))
{
    m_local_instance.role = "login";
    m_local_instance.endpoint = config.server.login;
    m_local_instance.weight = 1;
    m_local_instance.instance_id = name() + std::string("@") + make_endpoint_text(config.server.login);

    register_handler("/v1/auth/register", [this](evhttp_request* request) {
        gateway::AuthRegisterRequest register_request;
        auto body = read_request_body(request);
        if(body.empty())
        {
            evhttp_send_error(request, 400, "empty protobuf body");
            return;
        }
        if(!register_request.ParseFromString(body))
        {
            evhttp_send_error(request, 400, "invalid protobuf");
            return;
        }

        gateway::AuthRegisterResponse response;
        response.set_trace_id(make_trace_id());
        response.set_server_time_ms(now_ms());

        if(m_account_repository == nullptr)
        {
            response.set_code(50001);
            response.set_message("account repository unavailable");
            write_protobuf_response(request, response, 200);
            return;
        }

        const bool created = m_account_repository->create_account(register_request.account(), register_request.password());
        if(!created)
        {
            response.set_code(40001);
            response.set_message("account already exists or invalid input");
            write_protobuf_response(request, response, 200);
            return;
        }

        response.set_code(0);
        response.set_message("registered");
        write_protobuf_response(request, response, 200);
    });

    register_handler("/v1/auth/login", [this](evhttp_request* request) {
        gateway::AuthLoginRequest login_request;
        auto body = read_request_body(request);
        if(body.empty())
        {
            evhttp_send_error(request, 400, "empty protobuf body");
            return;
        }
        if(!login_request.ParseFromString(body))
        {
            evhttp_send_error(request, 400, "invalid protobuf");
            return;
        }

        gateway::AuthLoginResponse response;
        response.set_trace_id(make_trace_id());
        response.set_server_time_ms(now_ms());

        if(m_account_repository == nullptr)
        {
            response.set_code(50001);
            response.set_message("account repository unavailable");
            write_protobuf_response(request, response, 200);
            return;
        }

        const bool password_ok = m_account_repository->verify_password(login_request.account(), login_request.password());
        if(!password_ok)
        {
            response.set_code(40101);
            response.set_message("invalid account or password");
            write_protobuf_response(request, response, 200);
            return;
        }

        if(m_token_provider == nullptr)
        {
            response.set_code(50002);
            response.set_message("token provider unavailable");
            write_protobuf_response(request, response, 200);
            return;
        }

        const auto token = m_token_provider->issue(login_request.account(), m_config.jwt.expire_sec);
        if(token.empty())
        {
            response.set_code(50003);
            response.set_message("token issue failed");
            write_protobuf_response(request, response, 200);
            return;
        }

        auto game_instances = m_discovery ? m_discovery->list_instances("game") : std::vector<ServiceInstance>{};
        auto game_endpoint = choose_weighted_game_endpoint(game_instances);

        response.set_code(0);
        response.set_message("ok");
        response.set_jwt(token);
        response.mutable_game_endpoint()->set_host(game_endpoint.host);
        response.mutable_game_endpoint()->set_port(static_cast<uint32_t>(game_endpoint.port));

        write_protobuf_response(request, response, 200);
    });
}

LoginService::~LoginService() = default;

bool LoginService::start()
{
    if(!BasicHttpService::start())
    {
        return false;
    }

    if(m_discovery)
    {
        if(!m_discovery->register_instance(m_local_instance))
        {
            spdlog::warn("login register to redis failed, continue with config fallback route");
        }
    }

    m_last_heartbeat = std::chrono::steady_clock::now();
    return true;
}

bool LoginService::stop()
{
    if(m_discovery)
    {
        m_discovery->unregister_instance(m_local_instance);
    }

    return BasicHttpService::stop();
}

void LoginService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
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

EndpointConfig LoginService::choose_weighted_game_endpoint(const std::vector<ServiceInstance>& instances)
{
    if(instances.empty())
    {
        return m_config.server.game;
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

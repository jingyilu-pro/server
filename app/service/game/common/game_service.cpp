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

#include "game_service.h"

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
    return "game-" + std::to_string(ticks);
}

} // namespace

GameService::GameService(const RuntimeConfig& config,
                         std::shared_ptr<IServiceDiscovery> discovery,
                         std::shared_ptr<ITokenProvider> token_provider)
    : BasicHttpService("game", config.server.game),
      m_config(config),
      m_discovery(std::move(discovery)),
      m_token_provider(std::move(token_provider))
{
    m_local_instance.role = "game";
    m_local_instance.endpoint = config.server.game;
    m_local_instance.weight = 1;
    m_local_instance.instance_id = name() + std::string("@") + make_endpoint_text(config.server.game);

    register_handler("/v1/game/enter", [this](evhttp_request* request) {
        enter_game_async(request);
    });
}

GameService::~GameService() = default;

bool GameService::start()
{
    if(!BasicHttpService::start())
    {
        return false;
    }

    if(!m_discovery)
    {
        spdlog::error("game discovery unavailable");
        BasicHttpService::stop();
        return false;
    }

    ServiceDiscoveryOpResult register_result;
    if(!wait_service_discovery_result(m_discovery.get(),
                                      m_discovery->register_instance(m_local_instance),
                                      &register_result,
                                      std::max(1000, m_config.redis.op_timeout_ms)))
    {
        spdlog::error("game register to redis timeout or empty result");
        BasicHttpService::stop();
        return false;
    }

    if(!register_result.success)
    {
        spdlog::error("game register to redis failed: {}",
                      register_result.error);
        BasicHttpService::stop();
        return false;
    }

    m_last_heartbeat = std::chrono::steady_clock::now();
    return true;
}

bool GameService::stop()
{
    if(m_discovery)
    {
        ServiceDiscoveryOpResult unregister_result;
        wait_service_discovery_result(m_discovery.get(),
                                      m_discovery->unregister_instance(m_local_instance),
                                      &unregister_result,
                                      m_config.redis.op_timeout_ms);
    }

    return BasicHttpService::stop();
}

void GameService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    BasicHttpService::update(delta_time, last_tick_time);
}

void GameService::on_event_loop_tick()
{
    if(m_discovery)
    {
        m_discovery->poll();
    }

    auto now = std::chrono::steady_clock::now();
    if(m_discovery && !m_heartbeat_inflight &&
       now - m_last_heartbeat >= std::chrono::seconds(std::max(1, m_config.redis.refresh_sec)))
    {
        heartbeat_async();
        m_last_heartbeat = now;
    }
}

coro_task_t GameService::heartbeat_async()
{
    m_heartbeat_inflight = true;
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->heartbeat(m_local_instance));
    if(result == nullptr || !result->success)
    {
        spdlog::warn("game heartbeat to redis failed: {}",
                     result == nullptr ? "null result" : result->error);
    }
    m_heartbeat_inflight = false;
}

coro_task_t GameService::enter_game_async(evhttp_request* request)
{
    retain_request(request);

    gateway::GameEnterRequest game_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request, 400, "empty protobuf body");
        release_request(request);
        co_return;
    }
    if(!game_request.ParseFromString(body))
    {
        evhttp_send_error(request, 400, "invalid protobuf");
        release_request(request);
        co_return;
    }

    auto token = extract_authorization_token(request);
    gateway::GameEnterResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());
    if(token.empty())
    {
        response.set_code(40101);
        response.set_message("missing jwt");
    }
    else if(m_token_provider == nullptr)
    {
        response.set_code(50002);
        response.set_message("token provider unavailable");
    }
    else
    {
        auto verified = m_token_provider->verify(token);
        if(!verified)
        {
            response.set_code(40102);
            response.set_message("invalid or expired jwt");
        }
        else if(!game_request.account().empty() && verified->subject != game_request.account())
        {
            response.set_code(40103);
            response.set_message("jwt subject mismatch");
        }
        else
        {
            response.set_code(0);
            response.set_message("welcome " + verified->subject);
        }
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

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
    return "game-" + std::to_string(ticks);
}

} // namespace

GameService::GameService(const RuntimeConfig& config,
                         std::shared_ptr<IServiceDiscovery> discovery,
                         std::shared_ptr<ITokenProvider> token_provider)
    : BasicHttpService("game", config.server.game, true),
      m_config(config),
      m_discovery(std::move(discovery)),
      m_token_provider(std::move(token_provider))
{
    m_local_instance.role = "game";
    m_local_instance.weight = 1;

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

    m_local_instance.endpoint = endpoint();
    m_local_instance.instance_id = std::string(name()) + "@" + make_endpoint_text(m_local_instance.endpoint);

    if(!m_discovery)
    {
        spdlog::error("game discovery unavailable");
        BasicHttpService::stop();
        return false;
    }

    m_registered.store(false);
    m_register_inflight.store(false);

    m_last_heartbeat = std::chrono::steady_clock::now();
    return true;
}

bool GameService::stop()
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

void GameService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    BasicHttpService::update(delta_time, last_tick_time);
}

void GameService::on_event_loop_tick()
{
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

coro_task_t GameService::register_instance_async()
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
        spdlog::warn("game register to redis failed: {}",
                     register_result == nullptr ? "null result" : register_result->error);
    }
    m_register_inflight.store(false);
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
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!game_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    auto token = extract_authorization_token(request);
    gateway::GameEnterResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());
    if(token.empty())
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kMissingJwt,
                                                     http_code_message::gateway::message::kMissingJwt);
    }
    else if(m_token_provider == nullptr)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kTokenProviderUnavailable,
                                                     http_code_message::gateway::message::kTokenProviderUnavailable);
    }
    else
    {
        auto verified = m_token_provider->verify(token);
        if(!verified)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                         http_code_message::gateway::message::kInvalidOrExpiredJwt);
        }
        else if(!game_request.account().empty() && verified->subject != game_request.account())
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kJwtSubjectMismatch,
                                                         http_code_message::gateway::message::kJwtSubjectMismatch);
        }
        else
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kSuccess,
                                                         "welcome " + verified->subject);
        }
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

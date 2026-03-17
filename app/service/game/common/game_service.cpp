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
#include "protocol/mud.pb.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <unordered_set>

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

std::string sha256_hex_string(const std::string& input)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(context == nullptr)
    {
        return {};
    }

    const bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
                    EVP_DigestUpdate(context, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(context, digest.data(), &digest_len) == 1;

    EVP_MD_CTX_free(context);
    if(!ok)
    {
        return {};
    }

    static constexpr char kHexTable[] = "0123456789abcdef";
    std::string out;
    out.resize(digest_len * 2);
    for(unsigned int i = 0; i < digest_len; ++i)
    {
        out[2 * i] = kHexTable[digest[i] >> 4];
        out[2 * i + 1] = kHexTable[digest[i] & 0x0F];
    }
    return out;
}

template <typename TResponse>
void attach_mud_events_to_response(const std::vector<MudEventEnvelope>& events,
                                   uint64_t latest_event_id,
                                   TResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    auto* output_events = response->mutable_events();
    output_events->Clear();
    for(const auto& event : events)
    {
        auto* output = output_events->Add();
        output->set_event_id(event.event_id);
        output->set_type(event.type);
        output->set_title(event.title);
        output->set_content(event.content);
        output->set_server_time_ms(event.server_time_ms);
        output->set_unread(true);
    }
    response->set_next_event_id(latest_event_id);
}

std::pair<std::string, std::string> next_world_event_text(int64_t now_ms, int interval_sec)
{
    static const std::vector<std::pair<std::string, std::string>> kWorldEvents = {
        {"黄枫谷收徒", "黄枫谷外门今日开山，越国散修纷纷前往试灵根。"},
        {"血色禁地异动", "血色禁地外围灵气暴涨，有修士称见到异草出世。"},
        {"乱星海风暴", "乱星海近海突起风暴，数支采药小队请求同道支援。"},
        {"虚天殿传闻", "天南坊间再起虚天殿消息，诸多宗门已派人暗中探查。"}
    };

    if(kWorldEvents.empty())
    {
        return {};
    }

    const auto bucket = std::max<int64_t>(0, now_ms / (std::max(1, interval_sec) * 1000LL));
    return kWorldEvents[static_cast<size_t>(bucket % static_cast<int64_t>(kWorldEvents.size()))];
}

std::string extract_request_token(evhttp_request* request)
{
    if(request == nullptr)
    {
        return {};
    }

    auto* headers = evhttp_request_get_input_headers(request);
    if(headers == nullptr)
    {
        return {};
    }

    const char* authorization = evhttp_find_header(headers, "Authorization");
    if(authorization == nullptr)
    {
        return {};
    }

    std::string token = authorization;
    while(!token.empty() && std::isspace(static_cast<unsigned char>(token.front())) != 0)
    {
        token.erase(token.begin());
    }
    while(!token.empty() && std::isspace(static_cast<unsigned char>(token.back())) != 0)
    {
        token.pop_back();
    }

    const std::string bearer_scheme = "bearer";
    if(token.size() > bearer_scheme.size())
    {
        std::string scheme = token.substr(0, bearer_scheme.size());
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if(scheme == bearer_scheme && std::isspace(static_cast<unsigned char>(token[bearer_scheme.size()])) != 0)
        {
            std::size_t value_offset = bearer_scheme.size();
            while(value_offset < token.size() &&
                  std::isspace(static_cast<unsigned char>(token[value_offset])) != 0)
            {
                ++value_offset;
            }
            token = token.substr(value_offset);
        }
    }

    return token;
}

bool begin_authorize_mud_request(evhttp_request* request,
                                 const std::string& request_account,
                                 ITokenProvider* token_provider,
                                 MudGameRuntime* mud_runtime,
                                 std::string* resolved_account,
                                 std::string* token_out,
                                 int* error_code,
                                 std::string* error_message)
{
    if(error_code != nullptr)
    {
        *error_code = http_code_message::gateway::code::kSuccess;
    }
    if(error_message != nullptr)
    {
        error_message->clear();
    }

    auto fail = [&](int code_value, const std::string& message_value) {
        if(error_code != nullptr)
        {
            *error_code = code_value;
        }
        if(error_message != nullptr)
        {
            *error_message = message_value;
        }
    };

    const auto token = extract_request_token(request);
    if(token.empty())
    {
        fail(http_code_message::gateway::code::kMissingJwt,
             http_code_message::gateway::message::kMissingJwt);
        return false;
    }
    if(token_provider == nullptr)
    {
        fail(http_code_message::gateway::code::kTokenProviderUnavailable,
             http_code_message::gateway::message::kTokenProviderUnavailable);
        return false;
    }
    if(mud_runtime == nullptr || !mud_runtime->ready())
    {
        fail(http_code_message::gateway::code::kMudWorldUnavailable,
             mud_runtime == nullptr ? http_code_message::gateway::message::kMudWorldUnavailable
                                    : mud_runtime->ready_error());
        return false;
    }

    auto verified = token_provider->verify(token);
    if(!verified)
    {
        fail(http_code_message::gateway::code::kInvalidOrExpiredJwt,
             http_code_message::gateway::message::kInvalidOrExpiredJwt);
        return false;
    }

    std::string account;
    if(!mud_runtime->verify_account_match(verified->subject, request_account, &account))
    {
        fail(http_code_message::gateway::code::kJwtSubjectMismatch,
             http_code_message::gateway::message::kJwtSubjectMismatch);
        return false;
    }

    if(resolved_account != nullptr)
    {
        *resolved_account = account;
    }
    if(token_out != nullptr)
    {
        *token_out = token;
    }
    return true;
}

bool validate_mud_session(ISessionStore* session_store,
                          const std::string& account,
                          const std::string& token,
                          int timeout_ms)
{
    if(session_store == nullptr || account.empty())
    {
        return true;
    }

    SessionStoreOpResult session_result;
    const bool session_ok = wait_session_store_result(session_store,
                                                      session_store->get_session(account),
                                                      &session_result,
                                                      timeout_ms);
    if(!session_ok)
    {
        spdlog::warn("game mud session wait timeout account={}", account);
        return true;
    }
    if(!session_result.success)
    {
        spdlog::warn("game mud session query failed: {}", session_result.error);
        return true;
    }
    if(!session_result.hit || !session_result.session.has_value())
    {
        return true;
    }

    const auto token_digest = sha256_hex_string(token);
    if(!token_digest.empty() && session_result.session->token_digest != token_digest)
    {
        return false;
    }

    if(session_result.session->expire_at > 0)
    {
        const int64_t now_sec = now_ms() / 1000;
        int ttl_sec = static_cast<int>(session_result.session->expire_at - now_sec);
        ttl_sec = std::max(1, ttl_sec);
        SessionStoreOpResult touch_result;
        const bool touch_ok = wait_session_store_result(session_store,
                                                        session_store->touch_session(account, ttl_sec),
                                                        &touch_result,
                                                        timeout_ms);
        if(!touch_ok || !touch_result.success)
        {
            spdlog::warn("game mud session touch failed: {}",
                         touch_ok ? touch_result.error : "touch_wait_timeout");
        }
    }

    return true;
}

} // namespace

GameService::GameService(const RuntimeConfig& config,
                         std::shared_ptr<IServiceDiscovery> discovery,
                         std::shared_ptr<ITokenProvider> token_provider,
                         std::shared_ptr<ISessionStore> session_store,
                         std::shared_ptr<IMudPlayerRepository> mud_player_repository,
                         std::shared_ptr<IMudEventStore> mud_event_store)
    : BasicHttpService("game", config.server.game, true),
      m_config(config),
      m_discovery(std::move(discovery)),
      m_token_provider(std::move(token_provider)),
      m_session_store(std::move(session_store)),
      m_mud_player_repository(std::move(mud_player_repository)),
      m_mud_event_store(std::move(mud_event_store))
{
    m_local_instance.role = "game";
    m_local_instance.weight = 1;

    m_mud_world = std::make_shared<MudWorld>();
    std::string world_error;
    bool world_loaded = false;
    for(const auto& candidate : {std::string("doc/mud/world_data.json"),
                                 std::string("../doc/mud/world_data.json"),
                                 std::string("../../doc/mud/world_data.json"),
                                 std::string("../../../doc/mud/world_data.json")})
    {
        if(m_mud_world->load_from_file(candidate, &world_error))
        {
            world_loaded = true;
            break;
        }
    }
    if(!world_loaded)
    {
        spdlog::error("load mud world failed: {}", world_error);
    }
    m_mud_runtime = std::make_unique<MudGameRuntime>(m_mud_world, m_mud_player_repository);

    register_handler("/v1/game/enter", [this](evhttp_request* request) {
        enter_game_async(request);
    });
    register_handler("/v1/game/bootstrap", [this](evhttp_request* request) {
        bootstrap_async(request);
    });
    register_handler("/v1/game/character/create", [this](evhttp_request* request) {
        create_character_async(request);
    });
    register_handler("/v1/game/command/execute", [this](evhttp_request* request) {
        execute_command_async(request);
    });
    register_handler("/v1/game/feed/pull", [this](evhttp_request* request) {
        pull_feed_async(request);
    });
    register_handler("/v1/game/rank/list", [this](evhttp_request* request) {
        rank_list_async(request);
    });
}

GameService::~GameService() = default;

bool GameService::start()
{
    if(!m_discovery)
    {
        spdlog::error("game discovery unavailable");
        return false;
    }
    if(m_mud_runtime == nullptr || !m_mud_runtime->ready())
    {
        spdlog::error("game mud runtime unavailable: {}",
                      m_mud_runtime == nullptr ? "null runtime" : m_mud_runtime->ready_error());
        return false;
    }
    if(m_mud_event_store == nullptr || !m_mud_event_store->ready())
    {
        spdlog::error("game mud event store unavailable");
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
    m_local_instance.instance_id = std::string(name()) + "@" + make_endpoint_text(m_local_instance.endpoint);

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
        spdlog::error("game register to redis failed: {}",
                      register_done ? register_error : "register_wait_timeout");
        BasicHttpService::stop();
        return false;
    }

    return true;
}

bool GameService::stop()
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
            spdlog::warn("game unregister from redis failed: {}",
                         unregister_done ? unregister_error : "unregister_wait_timeout");
        }
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
    if(m_stopping.load())
    {
    if(m_discovery)
    {
        m_discovery->poll();
    }
    if(m_session_store)
    {
        m_session_store->poll();
    }
    if(m_mud_runtime)
    {
        m_mud_runtime->poll();
    }
    if(m_mud_event_store)
    {
        m_mud_event_store->poll();
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
    if(m_session_store)
    {
        m_session_store->poll();
    }
    if(m_mud_runtime)
    {
        m_mud_runtime->poll();
    }
    if(m_mud_event_store)
    {
        m_mud_event_store->poll();
    }

    auto now = std::chrono::steady_clock::now();
    if(m_discovery && m_registered.load() && !m_heartbeat_inflight &&
       now - m_last_heartbeat >= std::chrono::seconds(std::max(1, m_config.redis.refresh_sec)))
    {
        heartbeat_async();
        m_last_heartbeat = now;
    }

    if(m_mud_event_store && !m_world_event_inflight &&
       now.time_since_epoch() != std::chrono::steady_clock::duration::zero())
    {
        static auto last_world_event = std::chrono::steady_clock::time_point{};
        if(last_world_event.time_since_epoch() == std::chrono::steady_clock::duration::zero() ||
           now - last_world_event >= std::chrono::seconds(std::max(1, m_config.mud.world_event_interval_sec)))
        {
            emit_world_event();
            last_world_event = now;
        }
    }
}

coro_task_t GameService::register_instance_async()
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
        spdlog::warn("game register to redis failed: {}",
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

coro_task_t GameService::unregister_instance_async()
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

void GameService::emit_world_event()
{
    if(m_mud_event_store == nullptr)
    {
        return;
    }

    m_world_event_inflight = true;
    const auto rate_bucket = "world_event";
    MudEventStoreOpResult rate_result;
    const bool rate_ok = wait_mud_event_store_result(m_mud_event_store.get(),
                                                     m_mud_event_store->check_rate_limit(rate_bucket,
                                                                                        1,
                                                                                        std::max(1, m_config.mud.world_event_interval_sec)),
                                                     &rate_result,
                                                     m_config.redis.op_timeout_ms);
    if(rate_ok && rate_result.success && rate_result.allowed)
    {
        const auto current_now_ms = now_ms();
        auto world_event = next_world_event_text(current_now_ms, m_config.mud.world_event_interval_sec);
        if(!world_event.first.empty() && !world_event.second.empty())
        {
            MudEventEnvelope event;
            event.target_account.clear();
            event.type = "world";
            event.title = world_event.first;
            event.content = world_event.second;
            event.server_time_ms = current_now_ms;
            std::vector<MudEventEnvelope> events_to_append;
            events_to_append.push_back(event);
            MudEventStoreOpResult append_result;
            const bool append_ok = wait_mud_event_store_result(m_mud_event_store.get(),
                                                               m_mud_event_store->append_events(events_to_append),
                                                               &append_result,
                                                               m_config.redis.op_timeout_ms);
            if(!append_ok || !append_result.success)
            {
                spdlog::warn("game append world event failed: {}",
                             append_ok ? append_result.error : "append_wait_timeout");
            }
        }
    }
    else if(rate_ok && !rate_result.success)
    {
        spdlog::warn("game world event limiter failed: {}", rate_result.error);
    }
    m_world_event_inflight = false;
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
            bool session_mismatch = false;
            if(m_session_store && !verified->subject.empty())
            {
                auto* session_result = dynamic_cast<SessionStoreOpResult*>(
                    co_await m_session_store->get_session(verified->subject));
                if(session_result != nullptr && session_result->success && session_result->hit &&
                   session_result->session.has_value())
                {
                    const auto token_digest = sha256_hex_string(token);
                    if(!token_digest.empty() && session_result->session->token_digest != token_digest)
                    {
                        session_mismatch = true;
                    }
                    else if(session_result->session->expire_at > 0)
                    {
                        const int64_t now_sec = now_ms() / 1000;
                        int ttl_sec = static_cast<int>(session_result->session->expire_at - now_sec);
                        ttl_sec = std::max(1, ttl_sec);
                        auto* touch_result = dynamic_cast<SessionStoreOpResult*>(
                            co_await m_session_store->touch_session(verified->subject, ttl_sec));
                        if(touch_result == nullptr || !touch_result->success)
                        {
                            spdlog::warn("game session touch failed: {}",
                                         touch_result == nullptr ? "null result" : touch_result->error);
                        }
                    }
                }
                else if(session_result != nullptr && !session_result->success)
                {
                    spdlog::warn("game session query failed: {}", session_result->error);
                }
            }

            if(session_mismatch)
            {
                http_code_message::gateway::set_code_message(&response,
                                                             http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                             http_code_message::gateway::message::kInvalidOrExpiredJwt);
            }
            else
            {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kSuccess,
                                                         "welcome " + verified->subject);
            }
        }
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

coro_task_t GameService::bootstrap_async(evhttp_request* request)
{
    retain_request(request);

    mud::BootstrapRequest mud_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!mud_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    mud::BootstrapResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    std::string account;
    std::string request_token;
    int error_code = http_code_message::gateway::code::kSuccess;
    std::string error_message;
    if(begin_authorize_mud_request(request,
                                   mud_request.account(),
                                   m_token_provider.get(),
                                   m_mud_runtime.get(),
                                   &account,
                                   &request_token,
                                   &error_code,
                                   &error_message))
    {
        if(!validate_mud_session(m_session_store.get(), account, request_token, m_config.redis.op_timeout_ms))
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                         http_code_message::gateway::message::kInvalidOrExpiredJwt);
            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        auto* load_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_mud_player_repository->load_player(account));
        if(load_result == nullptr || !load_result->success)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                         "load mud player failed");
        }
        else
        {
            if(load_result->player.has_value())
            {
                if(load_result->player->team_id.empty())
                {
                    m_mud_runtime->forget_team_state(load_result->player->account);
                }
                else
                {
                    auto* team_result = dynamic_cast<MudPlayerRepositoryOpResult*>(
                        co_await m_mud_player_repository->list_team_members(load_result->player->team_id));
                    if(team_result != nullptr && team_result->success)
                    {
                        std::vector<MudPlayerState> team_members;
                        team_members.reserve(team_result->players.size());
                        for(const auto& entry : team_result->players)
                        {
                            team_members.push_back(entry.player);
                        }
                        m_mud_runtime->restore_team_state(team_members);
                    }
                }
            }
            m_mud_runtime->build_bootstrap_response(account, load_result->player, &response);
            MudEventStoreOpResult event_result;
            if(wait_mud_event_store_result(m_mud_event_store.get(),
                                           m_mud_event_store->list_recent(account, m_config.mud.bootstrap_recent_event_limit),
                                           &event_result,
                                           m_config.redis.op_timeout_ms) &&
               event_result.success)
            {
                attach_mud_events_to_response(event_result.events, event_result.latest_event_id, &response);
            }
        }
    }
    else
    {
        http_code_message::gateway::set_code_message(&response, error_code, error_message);
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

coro_task_t GameService::create_character_async(evhttp_request* request)
{
    retain_request(request);

    mud::CharacterCreateRequest mud_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!mud_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    mud::CharacterCreateResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    std::string account;
    std::string request_token;
    int error_code = http_code_message::gateway::code::kSuccess;
    std::string error_message;
    if(begin_authorize_mud_request(request,
                                   mud_request.account(),
                                   m_token_provider.get(),
                                   m_mud_runtime.get(),
                                   &account,
                                   &request_token,
                                   &error_code,
                                   &error_message))
    {
        if(!validate_mud_session(m_session_store.get(), account, request_token, m_config.redis.op_timeout_ms))
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                         http_code_message::gateway::message::kInvalidOrExpiredJwt);
            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        auto normalized_name = mud_trim(mud_request.character_name());
        if(normalized_name.empty() || normalized_name.size() > 24)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kCharacterAlreadyExistsOrInvalidInput,
                                                         "invalid character name");
        }
        else
        {
            auto* load_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_mud_player_repository->load_player(account));
            if(load_result == nullptr || !load_result->success)
            {
                http_code_message::gateway::set_code_message(&response,
                                                             http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                             "load mud player failed");
            }
            else if(load_result->found)
            {
                http_code_message::gateway::set_code_message(&response,
                                                             http_code_message::gateway::code::kCharacterAlreadyExistsOrInvalidInput,
                                                             "character already exists");
            }
            else
            {
                auto player = m_mud_runtime->build_default_player(account, normalized_name);
                auto* create_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_mud_player_repository->create_player(player));
                if(create_result == nullptr || !create_result->success || !create_result->create_ok)
                {
                    http_code_message::gateway::set_code_message(&response,
                                                                 http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                                 "create mud player failed");
                }
                else
                {
                    MudEventEnvelope created_event;
                    created_event.target_account = account;
                    created_event.type = "system";
                    created_event.title = "踏入修仙路";
                    created_event.content = normalized_name + "自七玄门山脚启程，正式踏上凡人修仙之路。";
                    created_event.server_time_ms = now_ms();
                    std::vector<MudEventEnvelope> created_events{created_event};
                    MudEventStoreOpResult append_result;
                    m_mud_runtime->build_create_character_response(player, &response);
                    if(wait_mud_event_store_result(m_mud_event_store.get(),
                                                   m_mud_event_store->append_events(created_events),
                                                   &append_result,
                                                   m_config.redis.op_timeout_ms) &&
                       append_result.success)
                    {
                        attach_mud_events_to_response(append_result.events,
                                                      append_result.latest_event_id,
                                                      &response);
                    }
                }
            }
        }
    }
    else
    {
        http_code_message::gateway::set_code_message(&response, error_code, error_message);
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

coro_task_t GameService::execute_command_async(evhttp_request* request)
{
    retain_request(request);

    mud::CommandExecuteRequest mud_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!mud_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    mud::CommandExecuteResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    std::string account;
    std::string request_token;
    int error_code = http_code_message::gateway::code::kSuccess;
    std::string error_message;
    if(begin_authorize_mud_request(request,
                                   mud_request.account(),
                                   m_token_provider.get(),
                                   m_mud_runtime.get(),
                                   &account,
                                   &request_token,
                                   &error_code,
                                   &error_message))
    {
        if(!validate_mud_session(m_session_store.get(), account, request_token, m_config.redis.op_timeout_ms))
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                         http_code_message::gateway::message::kInvalidOrExpiredJwt);
            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        auto* load_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_mud_player_repository->load_player(account));
        if(load_result == nullptr || !load_result->success)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                         "load mud player failed");
        }
        else if(!load_result->found || !load_result->player.has_value())
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kCharacterNotFound,
                                                         http_code_message::gateway::message::kCharacterNotFound);
        }
        else
        {
            if(load_result->player->team_id.empty())
            {
                m_mud_runtime->forget_team_state(load_result->player->account);
            }
            else
            {
                auto* team_result = dynamic_cast<MudPlayerRepositoryOpResult*>(
                    co_await m_mud_player_repository->list_team_members(load_result->player->team_id));
                if(team_result != nullptr && team_result->success)
                {
                    std::vector<MudPlayerState> team_members;
                    team_members.reserve(team_result->players.size());
                    for(const auto& entry : team_result->players)
                    {
                        team_members.push_back(entry.player);
                    }
                    m_mud_runtime->restore_team_state(team_members);
                }
            }

            auto player = *load_result->player;
            const auto trimmed_command = mud_trim(mud_request.command());
            if(trimmed_command.rfind("chat ", 0) == 0)
            {
                const auto raw_channel = mud_trim(trimmed_command.substr(5));
                const auto split_pos = raw_channel.find(' ');
                const auto channel = mud_to_lower_ascii(split_pos == std::string::npos
                                                            ? raw_channel
                                                            : raw_channel.substr(0, split_pos));
                MudEventStoreOpResult rate_result;
                if(!wait_mud_event_store_result(m_mud_event_store.get(),
                                                m_mud_event_store->check_rate_limit("chat:" + account + ":" + channel,
                                                                                   m_config.mud.chat_rate_limit_count,
                                                                                   m_config.mud.chat_rate_limit_window_sec),
                                                &rate_result,
                                                m_config.redis.op_timeout_ms) ||
                   !rate_result.success)
                {
                    http_code_message::gateway::set_code_message(&response,
                                                                 http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                                 "chat rate limit unavailable");
                    write_protobuf_response(request, response, 200);
                    release_request(request);
                    co_return;
                }
                if(!rate_result.allowed)
                {
                    http_code_message::gateway::set_code_message(&response,
                                                                 http_code_message::gateway::code::kInvalidMudCommand,
                                                                 "chat rate limited");
                    write_protobuf_response(request, response, 200);
                    release_request(request);
                    co_return;
                }
            }
            auto execution = m_mud_runtime->run_command(&player, mud_request.command());
            if(execution.success)
            {
                auto* save_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_mud_player_repository->save_player(player));
                if(save_result == nullptr || !save_result->success || !save_result->save_ok)
                {
                    http_code_message::gateway::set_code_message(&response,
                                                                 http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                                 "save mud player failed");
                    write_protobuf_response(request, response, 200);
                    release_request(request);
                    co_return;
                }

                for(const auto& extra_player : execution.extra_players_to_save)
                {
                    auto* extra_save_result = dynamic_cast<MudPlayerRepositoryOpResult*>(
                        co_await m_mud_player_repository->save_player(extra_player));
                    if(extra_save_result == nullptr || !extra_save_result->success || !extra_save_result->save_ok)
                    {
                        http_code_message::gateway::set_code_message(&response,
                                                                     http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                                     "save team player failed");
                        write_protobuf_response(request, response, 200);
                        release_request(request);
                        co_return;
                    }
                }
            }
            if(!execution.events.empty())
            {
                MudEventStoreOpResult append_result;
                if(!wait_mud_event_store_result(m_mud_event_store.get(),
                                                m_mud_event_store->append_events(execution.events),
                                                &append_result,
                                                m_config.redis.op_timeout_ms) ||
                   !append_result.success)
                {
                    http_code_message::gateway::set_code_message(&response,
                                                                 http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                                 "append mud events failed");
                    write_protobuf_response(request, response, 200);
                    release_request(request);
                    co_return;
                }
                execution.events = append_result.events;
            }
            m_mud_runtime->build_command_response(player, mud_request.command(), execution, &response);
        }
    }
    else
    {
        http_code_message::gateway::set_code_message(&response, error_code, error_message);
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

coro_task_t GameService::pull_feed_async(evhttp_request* request)
{
    retain_request(request);

    mud::FeedPullRequest mud_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!mud_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    mud::FeedPullResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    std::string account;
    std::string request_token;
    int error_code = http_code_message::gateway::code::kSuccess;
    std::string error_message;
    if(begin_authorize_mud_request(request,
                                   mud_request.account(),
                                   m_token_provider.get(),
                                   m_mud_runtime.get(),
                                   &account,
                                   &request_token,
                                   &error_code,
                                   &error_message))
    {
        if(!validate_mud_session(m_session_store.get(), account, request_token, m_config.redis.op_timeout_ms))
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                         http_code_message::gateway::message::kInvalidOrExpiredJwt);
            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        MudEventStoreOpResult event_result;
        if(!wait_mud_event_store_result(m_mud_event_store.get(),
                                        m_mud_event_store->list_after(account, mud_request.after_event_id(), mud_request.limit()),
                                        &event_result,
                                        m_config.redis.op_timeout_ms) ||
           !event_result.success)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                         "load mud events failed");
        }
        else
        {
            attach_mud_events_to_response(event_result.events, event_result.latest_event_id, &response);
            response.set_recommended_poll_interval_ms(1500);
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kSuccess,
                                                         http_code_message::gateway::message::kOk);
        }
    }
    else
    {
        http_code_message::gateway::set_code_message(&response, error_code, error_message);
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

coro_task_t GameService::rank_list_async(evhttp_request* request)
{
    retain_request(request);

    mud::RankListRequest mud_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!mud_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    mud::RankListResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    std::string account;
    std::string request_token;
    int error_code = http_code_message::gateway::code::kSuccess;
    std::string error_message;
    if(begin_authorize_mud_request(request,
                                   mud_request.account(),
                                   m_token_provider.get(),
                                   m_mud_runtime.get(),
                                   &account,
                                   &request_token,
                                   &error_code,
                                   &error_message))
    {
        bool session_mismatch = false;
        if(m_session_store && !account.empty())
        {
            auto* session_result = dynamic_cast<SessionStoreOpResult*>(co_await m_session_store->get_session(account));
            if(session_result != nullptr && session_result->success && session_result->hit &&
               session_result->session.has_value())
            {
                const auto token_digest = sha256_hex_string(request_token);
                if(!token_digest.empty() && session_result->session->token_digest != token_digest)
                {
                    session_mismatch = true;
                }
                else if(session_result->session->expire_at > 0)
                {
                    const int64_t now_sec = now_ms() / 1000;
                    int ttl_sec = static_cast<int>(session_result->session->expire_at - now_sec);
                    ttl_sec = std::max(1, ttl_sec);
                    auto* touch_result = dynamic_cast<SessionStoreOpResult*>(co_await m_session_store->touch_session(account, ttl_sec));
                    if(touch_result == nullptr || !touch_result->success)
                    {
                        spdlog::warn("game mud session touch failed: {}",
                                     touch_result == nullptr ? "null result" : touch_result->error);
                    }
                }
            }
            else if(session_result != nullptr && !session_result->success)
            {
                spdlog::warn("game mud session query failed: {}", session_result->error);
            }
        }

        if(session_mismatch)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kInvalidOrExpiredJwt,
                                                         http_code_message::gateway::message::kInvalidOrExpiredJwt);
            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        const auto leaderboard_type = mud_parse_leaderboard_type(mud_request.leaderboard());
        auto* rank_result = dynamic_cast<MudPlayerRepositoryOpResult*>(
            co_await m_mud_player_repository->list_top_players(leaderboard_type, mud_request.limit()));
        if(rank_result == nullptr || !rank_result->success)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                         "load leaderboard failed");
        }
        else
        {
            m_mud_runtime->build_rank_response(leaderboard_type, rank_result->players, &response);
        }
    }
    else
    {
        http_code_message::gateway::set_code_message(&response, error_code, error_message);
    }

    write_protobuf_response(request, response, 200);
    release_request(request);
}

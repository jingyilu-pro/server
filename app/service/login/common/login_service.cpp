//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "login_service.h"

#include "http_code_message.h"
#include "log/glogger.h"
#include "protocol/gateway.pb.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <sstream>

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

std::string bytes_to_hex(const unsigned char* data, size_t len)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for(size_t i = 0; i < len; ++i)
    {
        output << std::setw(2) << static_cast<int>(data[i]);
    }
    return output.str();
}

std::string make_password_hash(const std::string& password, const std::string& salt, int iterations)
{
    std::array<unsigned char, 32> digest{};
    const int rounds = std::max(1, iterations);
    const int rc = PKCS5_PBKDF2_HMAC(password.c_str(),
                                     static_cast<int>(password.size()),
                                     reinterpret_cast<const unsigned char*>(salt.data()),
                                     static_cast<int>(salt.size()),
                                     rounds,
                                     EVP_sha256(),
                                     static_cast<int>(digest.size()),
                                     digest.data());
    if(rc != 1)
    {
        return {};
    }
    return bytes_to_hex(digest.data(), digest.size());
}

bool verify_password_by_record(const std::string& password,
                               const AccountRecord& record,
                               int password_hash_iterations)
{
    const int normalized_iterations = std::max(1, password_hash_iterations);
    const auto expected_hash = make_password_hash(password, record.salt, normalized_iterations);
    if(!expected_hash.empty() && record.password_hash == expected_hash)
    {
        return true;
    }

    if(normalized_iterations != 20000)
    {
        const auto fast_hash = make_password_hash(password, record.salt, 20000);
        if(!fast_hash.empty() && record.password_hash == fast_hash)
        {
            return true;
        }
    }

    const auto legacy_hash = sha256_hex_string(record.salt + ":" + password);
    if(!legacy_hash.empty() && record.password_hash == legacy_hash)
    {
        return true;
    }

    return record.salt.empty() && record.password_hash == password;
}

bool is_ascii_alnum_account(const std::string& account)
{
    if(account.empty())
    {
        return false;
    }

    return std::all_of(account.begin(), account.end(), [](unsigned char ch) {
        return (ch >= '0' && ch <= '9') ||
               (ch >= 'A' && ch <= 'Z') ||
               (ch >= 'a' && ch <= 'z');
    });
}

} // namespace

LoginService::LoginService(const RuntimeConfig& config,
                           std::shared_ptr<IServiceDiscovery> discovery,
                           std::shared_ptr<IAccountRepository> account_repository,
                           std::shared_ptr<IAccountCacheStore> account_cache_store,
                           std::shared_ptr<ITokenProvider> token_provider,
                           std::shared_ptr<ISessionStore> session_store)
    : BasicHttpService("login", config.server.login, true),
      m_config(config),
      m_discovery(std::move(discovery)),
      m_account_repository(std::move(account_repository)),
      m_account_cache_store(std::move(account_cache_store)),
      m_token_provider(std::move(token_provider)),
      m_session_store(std::move(session_store))
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
    if(!m_discovery)
    {
        spdlog::error("login discovery unavailable");
        return false;
    }
    if(!m_account_repository)
    {
        spdlog::error("login account repository unavailable");
        return false;
    }
    if(!m_account_repository->ready())
    {
        spdlog::error("login account repository not ready");
        return false;
    }

    m_stopping.store(false);
    m_registered.store(false);
    m_register_inflight.store(false);
    m_heartbeat_inflight = false;
    m_game_instances_refresh_inflight = false;
    m_last_game_instances_refresh = std::chrono::steady_clock::time_point{};
    m_cached_game_instances.clear();

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
        spdlog::error("login register to redis failed: {}",
                      register_done ? register_error : "register_wait_timeout");
        BasicHttpService::stop();
        return false;
    }

    return true;
}

bool LoginService::stop()
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
            spdlog::warn("login unregister from redis failed: {}",
                         unregister_done ? unregister_error : "unregister_wait_timeout");
        }
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
    if(m_stopping.load())
    {
        if(m_discovery)
        {
            m_discovery->poll();
        }
        if(m_account_repository)
        {
            m_account_repository->poll();
        }
        if(m_account_cache_store)
        {
            m_account_cache_store->poll();
        }
        if(m_session_store)
        {
            m_session_store->poll();
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
    if(m_account_repository)
    {
        m_account_repository->poll();
    }
    if(m_account_cache_store)
    {
        m_account_cache_store->poll();
    }
    if(m_session_store)
    {
        m_session_store->poll();
    }

    auto now = std::chrono::steady_clock::now();
    if(m_discovery && m_registered.load() && !m_heartbeat_inflight &&
       now - m_last_heartbeat >= std::chrono::seconds(std::max(1, m_config.redis.refresh_sec)))
    {
        heartbeat_async();
        m_last_heartbeat = now;
    }

    if(m_discovery && m_registered.load() && !m_game_instances_refresh_inflight)
    {
        const bool never_refreshed =
            m_last_game_instances_refresh.time_since_epoch() == std::chrono::steady_clock::duration::zero();
        if(never_refreshed || now - m_last_game_instances_refresh >= std::chrono::seconds(1))
        {
            refresh_game_instances_async();
        }
    }
}

coro_task_t LoginService::register_instance_async()
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
        spdlog::warn("login register to redis failed: {}",
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

coro_task_t LoginService::unregister_instance_async()
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

coro_task_t LoginService::refresh_game_instances_async()
{
    m_game_instances_refresh_inflight = true;
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->list_instances("game"));
    if(result != nullptr && result->success)
    {
        m_cached_game_instances = result->instances;
    }
    else
    {
        spdlog::warn("login refresh game instances failed: {}",
                     result == nullptr ? "null result" : result->error);
    }
    m_last_game_instances_refresh = std::chrono::steady_clock::now();
    m_game_instances_refresh_inflight = false;
}

coro_task_t LoginService::register_async(evhttp_request* request)
{
    retain_request(request);

    gateway::AuthRegisterRequest register_request;
    auto body = read_request_body(request);
    if(body.empty())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!register_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    gateway::AuthRegisterResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    if(m_account_repository == nullptr)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kAccountRepositoryUnavailable,
                                                     http_code_message::gateway::message::kAccountRepositoryUnavailable);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(!is_ascii_alnum_account(register_request.account()))
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kAccountAlreadyExistsOrInvalidInput,
                                                     "account must contain only English letters and digits");
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    auto* create_result = dynamic_cast<AccountRepositoryOpResult*>(
        co_await m_account_repository->create_account(register_request.account(), register_request.password()));

    if(create_result == nullptr || !create_result->success)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kAccountRepositoryUnavailable,
                                                     http_code_message::gateway::message::kAccountRepositoryUnavailable);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(!create_result->create_ok)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kAccountAlreadyExistsOrInvalidInput,
                                                     http_code_message::gateway::message::kAccountAlreadyExistsOrInvalidInput);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(m_account_cache_store)
    {
        auto* cache_erase_result = dynamic_cast<AccountCacheOpResult*>(
            co_await m_account_cache_store->erase_account(register_request.account()));
        if(cache_erase_result == nullptr || !cache_erase_result->success)
        {
            spdlog::warn("login register erase cache failed: {}",
                         cache_erase_result == nullptr ? "null result" : cache_erase_result->error);
        }
    }

    http_code_message::gateway::set_code_message(&response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kRegistered);
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
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kEmptyProtobufBody);
        release_request(request);
        co_return;
    }
    if(!login_request.ParseFromString(body))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidProtobuf);
        release_request(request);
        co_return;
    }

    gateway::AuthLoginResponse response;
    response.set_trace_id(make_trace_id());
    response.set_server_time_ms(now_ms());

    if(m_account_repository == nullptr)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kAccountRepositoryUnavailable,
                                                     http_code_message::gateway::message::kAccountRepositoryUnavailable);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    bool password_ok = false;
    bool verified_by_repository = false;
    std::optional<AccountRecord> verified_record;
    if(m_account_cache_store)
    {
        auto* cache_result = dynamic_cast<AccountCacheOpResult*>(
            co_await m_account_cache_store->get_account(login_request.account()));
        if(cache_result != nullptr && cache_result->success && cache_result->hit && cache_result->record.has_value())
        {
            verified_record = cache_result->record;
            password_ok = verify_password_by_record(login_request.password(),
                                                    *cache_result->record,
                                                    m_config.mysql.password_hash_iterations);
        }
        else if(cache_result != nullptr && !cache_result->success)
        {
            spdlog::warn("login get account cache failed: {}", cache_result->error);
        }
    }

    if(!password_ok)
    {
        auto* verify_result = dynamic_cast<AccountRepositoryOpResult*>(
            co_await m_account_repository->verify_password(login_request.account(), login_request.password()));
        if(verify_result == nullptr || !verify_result->success)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kAccountRepositoryUnavailable,
                                                         http_code_message::gateway::message::kAccountRepositoryUnavailable);
            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        verified_by_repository = true;
        password_ok = verify_result->password_ok;
        verified_record = verify_result->record;
    }

    if(!password_ok)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kInvalidAccountOrPassword,
                                                     http_code_message::gateway::message::kInvalidAccountOrPassword);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(verified_by_repository && m_account_cache_store && verified_record.has_value())
    {
        auto* cache_put_result = dynamic_cast<AccountCacheOpResult*>(
            co_await m_account_cache_store->put_account(*verified_record,
                                                        std::max(1, m_config.redis.account_cache_ttl_sec)));
        if(cache_put_result == nullptr || !cache_put_result->success)
        {
            spdlog::warn("login put account cache failed: {}",
                         cache_put_result == nullptr ? "null result" : cache_put_result->error);
        }
    }

    if(m_token_provider == nullptr)
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kTokenProviderUnavailable,
                                                     http_code_message::gateway::message::kTokenProviderUnavailable);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    const auto token = m_token_provider->issue(login_request.account(), m_config.jwt.expire_sec);
    if(token.empty())
    {
        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kTokenIssueFailed,
                                                     http_code_message::gateway::message::kTokenIssueFailed);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    if(m_session_store)
    {
        SessionRecord session;
        session.account = login_request.account();
        session.token_digest = sha256_hex_string(token);
        session.expire_at = now_ms() / 1000 + std::max(1, m_config.jwt.expire_sec);

        if(session.token_digest.empty())
        {
            spdlog::warn("login session digest empty, skip session upsert");
        }
        else
        {
            auto* session_result = dynamic_cast<SessionStoreOpResult*>(
                co_await m_session_store->upsert_session(session, std::max(1, m_config.jwt.expire_sec)));
            if(session_result == nullptr || !session_result->success || !session_result->upsert_ok)
            {
                spdlog::warn("login session upsert failed: {}",
                             session_result == nullptr ? "null result" : session_result->error);
            }
        }
    }

    std::vector<ServiceInstance> game_instances = m_cached_game_instances;
    if(game_instances.empty())
    {
        if(auto* list_result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await m_discovery->list_instances("game"));
           list_result != nullptr && list_result->success)
        {
            game_instances = list_result->instances;
            m_cached_game_instances = game_instances;
            m_last_game_instances_refresh = std::chrono::steady_clock::now();
        }
    }
    if(game_instances.empty())
    {
        if(!m_config.server.game.host.empty() && m_config.server.game.port > 0)
        {
            http_code_message::gateway::set_code_message(&response,
                                                         http_code_message::gateway::code::kSuccess,
                                                         http_code_message::gateway::message::kOk);
            response.set_jwt(token);
            response.mutable_game_endpoint()->set_host(m_config.server.game.host);
            response.mutable_game_endpoint()->set_port(static_cast<uint32_t>(m_config.server.game.port));

            write_protobuf_response(request, response, 200);
            release_request(request);
            co_return;
        }

        http_code_message::gateway::set_code_message(&response,
                                                     http_code_message::gateway::code::kGameServiceUnavailable,
                                                     http_code_message::gateway::message::kGameServiceUnavailable);
        write_protobuf_response(request, response, 200);
        release_request(request);
        co_return;
    }

    auto game_endpoint = choose_weighted_game_endpoint(game_instances);

    http_code_message::gateway::set_code_message(&response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
    response.set_jwt(token);
    response.mutable_game_endpoint()->set_host(game_endpoint.host);
    response.mutable_game_endpoint()->set_port(static_cast<uint32_t>(game_endpoint.port));

    write_protobuf_response(request, response, 200);
    release_request(request);
}

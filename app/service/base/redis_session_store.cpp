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

#include "redis_session_store.h"

#include "log/glogger.h"

#include <algorithm>
#include <sstream>

namespace
{

bool is_reply_ok(redisReply* reply)
{
    return reply != nullptr &&
           reply->type == REDIS_REPLY_STATUS &&
           reply->str != nullptr &&
           std::string(reply->str) == "OK";
}

} // namespace

RedisSessionManager::RedisSessionManager(int worker_count)
    : CoroManager(worker_count)
{
    init();
}

RedisSessionManager::~RedisSessionManager() = default;

CoroResult* RedisSessionManager::alloc()
{
    expand<SessionStoreOpResult>();
    return inner_alloc();
}

RedisSessionStore::RedisSessionStore(const RedisConfig& config)
    : m_config(config)
{
    m_manager = std::make_unique<RedisSessionManager>(std::max(1, m_config.coro_workers));

    std::lock_guard lock(m_mutex);
    m_ready = ensure_connected(&m_context, nullptr);
}

RedisSessionStore::~RedisSessionStore()
{
    std::lock_guard lock(m_mutex);
    if(m_context != nullptr)
    {
        redisFree(m_context);
        m_context = nullptr;
    }
}

bool RedisSessionStore::ready() const
{
    return m_ready;
}

void RedisSessionStore::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

SessionStoreOpResult* RedisSessionStore::alloc_result()
{
    if(m_manager == nullptr)
    {
        return nullptr;
    }

    return dynamic_cast<SessionStoreOpResult*>(m_manager->alloc());
}

CoroAwaitable RedisSessionStore::get_session(const std::string& account)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::get_session,
                 account,
                 std::nullopt,
                 0,
                 [this](SessionStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisSessionStore::upsert_session(const SessionRecord& session, int ttl_sec)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::upsert_session,
                 session.account,
                 session,
                 ttl_sec,
                 [this](SessionStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisSessionStore::touch_session(const std::string& account, int ttl_sec)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::touch_session,
                 account,
                 std::nullopt,
                 ttl_sec,
                 [this](SessionStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisSessionStore::remove_session(const std::string& account)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::remove_session,
                 account,
                 std::nullopt,
                 0,
                 [this](SessionStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

void RedisSessionStore::execute_operation(SessionStoreOpResult* result)
{
    if(result == nullptr)
    {
        return;
    }

    std::string connect_error;
    redisContext* worker_context = ensure_worker_connection(&connect_error);
    if(worker_context == nullptr)
    {
        result->success = false;
        result->error = connect_error.empty() ? "redis_unavailable" : connect_error;
        return;
    }

    switch(result->op_type)
    {
    case SessionStoreOpType::get_session:
    {
        if(result->request_account.empty())
        {
            result->success = true;
            result->hit = false;
            break;
        }

        const auto key = make_session_key(result->request_account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context,
                                                            "HMGET %s account token_digest expire_at",
                                                            key.c_str()));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_get_session_failed";
            break;
        }

        if(reply->type != REDIS_REPLY_ARRAY || reply->elements < 3)
        {
            freeReplyObject(reply);
            result->success = false;
            result->error = "redis_invalid_session_reply";
            break;
        }

        const auto* account_reply = reply->element[0];
        const auto* digest_reply = reply->element[1];
        const auto* expire_reply = reply->element[2];
        if(account_reply == nullptr || digest_reply == nullptr || expire_reply == nullptr ||
           account_reply->type == REDIS_REPLY_NIL ||
           digest_reply->type == REDIS_REPLY_NIL ||
           expire_reply->type == REDIS_REPLY_NIL)
        {
            freeReplyObject(reply);
            result->success = true;
            result->hit = false;
            break;
        }

        SessionRecord record;
        record.account = account_reply->str == nullptr ? "" : account_reply->str;
        record.token_digest = digest_reply->str == nullptr ? "" : digest_reply->str;
        if(expire_reply->str != nullptr)
        {
            try
            {
                record.expire_at = std::stoll(expire_reply->str);
            }
            catch(...)
            {
                record.expire_at = 0;
            }
        }
        freeReplyObject(reply);

        if(record.account.empty() || record.token_digest.empty())
        {
            result->success = true;
            result->hit = false;
            break;
        }

        result->success = true;
        result->hit = true;
        result->session = record;
        break;
    }
    case SessionStoreOpType::upsert_session:
    {
        if(!result->request_session.has_value() || result->request_session->account.empty())
        {
            result->success = true;
            result->upsert_ok = false;
            break;
        }

        const auto ttl_sec = std::max(1, result->request_ttl_sec);
        const auto key = make_session_key(result->request_session->account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context,
                                                            "HSET %s account %s token_digest %s expire_at %lld",
                                                            key.c_str(),
                                                            result->request_session->account.c_str(),
                                                            result->request_session->token_digest.c_str(),
                                                            static_cast<long long>(result->request_session->expire_at)));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_hset_session_failed";
            break;
        }
        freeReplyObject(reply);

        reply = static_cast<redisReply*>(redisCommand(worker_context, "EXPIRE %s %d", key.c_str(), ttl_sec));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_expire_session_failed";
            break;
        }
        const bool expire_ok = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
        freeReplyObject(reply);

        result->success = expire_ok;
        result->upsert_ok = expire_ok;
        if(!expire_ok)
        {
            result->error = "redis_expire_session_rejected";
        }
        break;
    }
    case SessionStoreOpType::touch_session:
    {
        if(result->request_account.empty())
        {
            result->success = true;
            result->touch_ok = false;
            break;
        }

        const auto ttl_sec = std::max(1, result->request_ttl_sec);
        const auto key = make_session_key(result->request_account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context, "EXPIRE %s %d", key.c_str(), ttl_sec));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_touch_failed";
            break;
        }
        const bool touch_ok = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
        freeReplyObject(reply);

        result->success = true;
        result->touch_ok = touch_ok;
        break;
    }
    case SessionStoreOpType::remove_session:
    {
        if(result->request_account.empty())
        {
            result->success = true;
            result->remove_ok = true;
            break;
        }

        const auto key = make_session_key(result->request_account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context, "DEL %s", key.c_str()));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_remove_session_failed";
            break;
        }
        freeReplyObject(reply);

        result->success = true;
        result->remove_ok = true;
        break;
    }
    default:
        result->success = false;
        result->error = "redis_unknown_operation";
        break;
    }
}

bool RedisSessionStore::ensure_connected(redisContext** context, std::string* error)
{
    if(context == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_invalid_context";
        }
        return false;
    }

    if(*context != nullptr && (*context)->err == 0)
    {
        return true;
    }

    if(*context != nullptr)
    {
        redisFree(*context);
        *context = nullptr;
    }

    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 500 * 1000;

    *context = redisConnectWithTimeout(m_config.host.c_str(), m_config.port, timeout);
    if(*context == nullptr || (*context)->err != 0)
    {
        spdlog::error("redis session connect failed host={} port={} err={}",
                      m_config.host,
                      m_config.port,
                      *context == nullptr ? "null context" : (*context)->errstr);
        if(*context != nullptr)
        {
            redisFree(*context);
            *context = nullptr;
        }
        if(error != nullptr)
        {
            *error = "redis_connect_failed";
        }
        return false;
    }

    if(!m_config.password.empty())
    {
        auto* auth_reply = static_cast<redisReply*>(redisCommand(*context, "AUTH %s", m_config.password.c_str()));
        if(!is_reply_ok(auth_reply))
        {
            spdlog::error("redis session auth failed");
            if(auth_reply != nullptr)
            {
                freeReplyObject(auth_reply);
            }
            redisFree(*context);
            *context = nullptr;
            if(error != nullptr)
            {
                *error = "redis_auth_failed";
            }
            return false;
        }
        freeReplyObject(auth_reply);
    }

    if(m_config.db > 0)
    {
        auto* select_reply = static_cast<redisReply*>(redisCommand(*context, "SELECT %d", m_config.db));
        if(!is_reply_ok(select_reply))
        {
            spdlog::error("redis session select db={} failed", m_config.db);
            if(select_reply != nullptr)
            {
                freeReplyObject(select_reply);
            }
            redisFree(*context);
            *context = nullptr;
            if(error != nullptr)
            {
                *error = "redis_select_db_failed";
            }
            return false;
        }
        freeReplyObject(select_reply);
    }

    return true;
}

redisContext* RedisSessionStore::ensure_worker_connection(std::string* error)
{
    thread_local const RedisSessionStore* tls_owner = nullptr;
    thread_local redisContext* tls_context = nullptr;

    if(tls_owner != this)
    {
        if(tls_context != nullptr)
        {
            redisFree(tls_context);
            tls_context = nullptr;
        }
        tls_owner = this;
    }

    if(ensure_connected(&tls_context, error))
    {
        return tls_context;
    }
    return nullptr;
}

std::string RedisSessionStore::make_session_key(const std::string& account) const
{
    std::ostringstream output;
    output << m_config.key_prefix << ":sess:" << account;
    return output.str();
}

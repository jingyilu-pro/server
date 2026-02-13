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

#include "redis_account_cache_store.h"

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

RedisAccountCacheManager::RedisAccountCacheManager(int worker_count)
    : CoroManager(worker_count)
{
    init();
}

RedisAccountCacheManager::~RedisAccountCacheManager() = default;

CoroResult* RedisAccountCacheManager::alloc()
{
    expand<AccountCacheOpResult>();
    return inner_alloc();
}

RedisAccountCacheStore::RedisAccountCacheStore(const RedisConfig& config)
    : m_config(config)
{
    m_manager = std::make_unique<RedisAccountCacheManager>(std::max(1, m_config.coro_workers));

    std::lock_guard lock(m_mutex);
    m_ready = ensure_connected(&m_context, nullptr);
}

RedisAccountCacheStore::~RedisAccountCacheStore()
{
    std::lock_guard lock(m_mutex);
    if(m_context != nullptr)
    {
        redisFree(m_context);
        m_context = nullptr;
    }
}

bool RedisAccountCacheStore::ready() const
{
    return m_ready;
}

void RedisAccountCacheStore::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

AccountCacheOpResult* RedisAccountCacheStore::alloc_result()
{
    if(m_manager == nullptr)
    {
        return nullptr;
    }

    return dynamic_cast<AccountCacheOpResult*>(m_manager->alloc());
}

CoroAwaitable RedisAccountCacheStore::get_account(const std::string& account)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountCacheOpType::get_account,
                 account,
                 std::nullopt,
                 0,
                 [this](AccountCacheOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisAccountCacheStore::put_account(const AccountRecord& record, int ttl_sec)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountCacheOpType::put_account,
                 record.account,
                 record,
                 ttl_sec,
                 [this](AccountCacheOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisAccountCacheStore::erase_account(const std::string& account)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountCacheOpType::erase_account,
                 account,
                 std::nullopt,
                 0,
                 [this](AccountCacheOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

void RedisAccountCacheStore::execute_operation(AccountCacheOpResult* result)
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
    case AccountCacheOpType::get_account:
    {
        if(result->request_account.empty())
        {
            result->success = true;
            result->hit = false;
            break;
        }

        const auto key = make_account_key(result->request_account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context,
                                                            "HMGET %s account password_hash salt",
                                                            key.c_str()));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_get_failed";
            break;
        }

        if(reply->type != REDIS_REPLY_ARRAY || reply->elements < 3)
        {
            freeReplyObject(reply);
            result->success = false;
            result->error = "redis_invalid_hmget_reply";
            break;
        }

        const auto* account_reply = reply->element[0];
        const auto* hash_reply = reply->element[1];
        const auto* salt_reply = reply->element[2];

        if(account_reply == nullptr || hash_reply == nullptr || salt_reply == nullptr ||
           account_reply->type == REDIS_REPLY_NIL ||
           hash_reply->type == REDIS_REPLY_NIL ||
           salt_reply->type == REDIS_REPLY_NIL)
        {
            freeReplyObject(reply);
            result->success = true;
            result->hit = false;
            break;
        }

        AccountRecord record;
        record.account = account_reply->str == nullptr ? "" : account_reply->str;
        record.password_hash = hash_reply->str == nullptr ? "" : hash_reply->str;
        record.salt = salt_reply->str == nullptr ? "" : salt_reply->str;
        freeReplyObject(reply);

        if(record.account.empty() || record.password_hash.empty())
        {
            result->success = true;
            result->hit = false;
            break;
        }

        result->success = true;
        result->hit = true;
        result->record = record;
        break;
    }
    case AccountCacheOpType::put_account:
    {
        if(!result->request_record.has_value() || result->request_record->account.empty())
        {
            result->success = true;
            result->put_ok = false;
            break;
        }

        const auto ttl_sec = std::max(1, result->request_ttl_sec);
        const auto key = make_account_key(result->request_record->account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context,
                                                            "HSET %s account %s password_hash %s salt %s",
                                                            key.c_str(),
                                                            result->request_record->account.c_str(),
                                                            result->request_record->password_hash.c_str(),
                                                            result->request_record->salt.c_str()));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_hset_failed";
            break;
        }
        freeReplyObject(reply);

        reply = static_cast<redisReply*>(redisCommand(worker_context, "EXPIRE %s %d", key.c_str(), ttl_sec));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_expire_failed";
            break;
        }
        const bool expire_ok = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
        freeReplyObject(reply);

        result->success = expire_ok;
        result->put_ok = expire_ok;
        if(!expire_ok)
        {
            result->error = "redis_expire_rejected";
        }
        break;
    }
    case AccountCacheOpType::erase_account:
    {
        if(result->request_account.empty())
        {
            result->success = true;
            result->erase_ok = true;
            break;
        }

        const auto key = make_account_key(result->request_account);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context, "DEL %s", key.c_str()));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_del_failed";
            break;
        }
        freeReplyObject(reply);

        result->success = true;
        result->erase_ok = true;
        break;
    }
    default:
        result->success = false;
        result->error = "redis_unknown_operation";
        break;
    }
}

bool RedisAccountCacheStore::ensure_connected(redisContext** context, std::string* error)
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
        spdlog::error("redis account cache connect failed host={} port={} err={}",
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
            spdlog::error("redis account cache auth failed");
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
            spdlog::error("redis account cache select db={} failed", m_config.db);
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

redisContext* RedisAccountCacheStore::ensure_worker_connection(std::string* error)
{
    thread_local const RedisAccountCacheStore* tls_owner = nullptr;
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

std::string RedisAccountCacheStore::make_account_key(const std::string& account) const
{
    std::ostringstream output;
    output << m_config.key_prefix << ":acct:" << account;
    return output.str();
}

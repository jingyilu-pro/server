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

#include "mud_event_store.h"

#include "log/glogger.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>
#include <unordered_set>

namespace
{

bool is_reply_ok(redisReply* reply)
{
    return reply != nullptr &&
           reply->type == REDIS_REPLY_STATUS &&
           reply->str != nullptr &&
           std::string(reply->str) == "OK";
}

class RedisMudEventManager : public CoroManager
{
public:
    explicit RedisMudEventManager(int worker_count)
        : CoroManager(worker_count)
    {
        init();
    }

    ~RedisMudEventManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<MudEventStoreOpResult>();
        return inner_alloc();
    }
};

} // namespace

class RedisMudEventStore::RedisMudEventManager : public ::RedisMudEventManager
{
public:
    explicit RedisMudEventManager(int worker_count)
        : ::RedisMudEventManager(worker_count)
    {
    }
};

namespace
{

struct MudEventStoreWaitState
{
    bool done = false;
    bool has_result = false;
    MudEventStoreOpResult snapshot;
};

coro_task_t capture_mud_event_store_result(CoroAwaitable awaitable,
                                           std::shared_ptr<MudEventStoreWaitState> state)
{
    auto* result = dynamic_cast<MudEventStoreOpResult*>(co_await awaitable);
    if(!state)
    {
        co_return;
    }

    if(result != nullptr)
    {
        state->snapshot.op_type = result->op_type;
        state->snapshot.request_account = result->request_account;
        state->snapshot.request_after_event_id = result->request_after_event_id;
        state->snapshot.request_limit = result->request_limit;
        state->snapshot.request_events = result->request_events;
        state->snapshot.request_bucket = result->request_bucket;
        state->snapshot.request_rate_limit = result->request_rate_limit;
        state->snapshot.request_window_sec = result->request_window_sec;
        state->snapshot.success = result->success;
        state->snapshot.error = result->error;
        state->snapshot.events = result->events;
        state->snapshot.latest_event_id = result->latest_event_id;
        state->snapshot.allowed = result->allowed;
        state->snapshot.current_count = result->current_count;
        state->has_result = true;
    }
    state->done = true;
}

} // namespace

bool wait_mud_event_store_result(IMudEventStore* store,
                                 CoroAwaitable awaitable,
                                 MudEventStoreOpResult* out_result,
                                 int timeout_ms)
{
    if(out_result == nullptr)
    {
        return false;
    }
    out_result->clear();

    auto state = std::make_shared<MudEventStoreWaitState>();
    capture_mud_event_store_result(awaitable, state);

    if(timeout_ms <= 0)
    {
        timeout_ms = 1000;
    }

    auto begin = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - begin)
              .count() <= timeout_ms)
    {
        if(store != nullptr)
        {
            store->poll();
        }

        if(state->done)
        {
            if(state->has_result)
            {
                *out_result = state->snapshot;
            }
            return state->has_result;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}

RedisMudEventStore::RedisMudEventStore(const RedisConfig& redis_config, const MudConfig& mud_config)
    : m_redis_config(redis_config),
      m_mud_config(mud_config)
{
    m_manager = std::make_unique<RedisMudEventManager>(std::max(1, m_redis_config.coro_workers));
    std::lock_guard lock(m_mutex);
    m_ready = ensure_connected(&m_context, nullptr);
}

RedisMudEventStore::~RedisMudEventStore()
{
    std::lock_guard lock(m_mutex);
    if(m_context != nullptr)
    {
        redisFree(m_context);
        m_context = nullptr;
    }
}

bool RedisMudEventStore::ready() const
{
    return m_ready;
}

void RedisMudEventStore::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

MudEventStoreOpResult* RedisMudEventStore::alloc_result()
{
    if(m_manager == nullptr)
    {
        return nullptr;
    }
    return dynamic_cast<MudEventStoreOpResult*>(m_manager->alloc());
}

CoroAwaitable RedisMudEventStore::append_events(const std::vector<MudEventEnvelope>& events)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudEventStoreOpType::append_events,
                 "",
                 0,
                 0,
                 events,
                 "",
                 0,
                 0,
                 [this](MudEventStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisMudEventStore::list_recent(const std::string& account, int limit)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudEventStoreOpType::list_recent,
                 account,
                 0,
                 limit,
                 {},
                 "",
                 0,
                 0,
                 [this](MudEventStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisMudEventStore::list_after(const std::string& account, uint64_t after_event_id, int limit)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudEventStoreOpType::list_after,
                 account,
                 after_event_id,
                 limit,
                 {},
                 "",
                 0,
                 0,
                 [this](MudEventStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisMudEventStore::check_rate_limit(const std::string& bucket, int limit, int window_sec)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudEventStoreOpType::check_rate_limit,
                 "",
                 0,
                 0,
                 {},
                 bucket,
                 limit,
                 window_sec,
                 [this](MudEventStoreOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

void RedisMudEventStore::execute_operation(MudEventStoreOpResult* result)
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
    case MudEventStoreOpType::append_events:
    {
        result->events.clear();
        result->events.reserve(result->request_events.size());
        bool ok = true;
        for(auto event : result->request_events)
        {
            if(!append_event(worker_context, &event, &result->error))
            {
                ok = false;
                break;
            }
            result->events.push_back(std::move(event));
        }
        result->success = ok;
        if(ok)
        {
            latest_event_id(worker_context, &result->latest_event_id, nullptr);
        }
        break;
    }
    case MudEventStoreOpType::list_recent:
    case MudEventStoreOpType::list_after:
    {
        const int limit = std::clamp(result->request_limit <= 0 ? 50 : result->request_limit, 1, 100);
        std::vector<uint64_t> global_ids;
        std::vector<uint64_t> account_ids;
        const bool descending = result->op_type == MudEventStoreOpType::list_recent;
        bool ok = collect_event_ids(worker_context,
                                    make_global_index_key(),
                                    result->request_after_event_id,
                                    limit,
                                    descending,
                                    &global_ids,
                                    &result->error);
        if(ok && !result->request_account.empty())
        {
            ok = collect_event_ids(worker_context,
                                   make_account_index_key(result->request_account),
                                   result->request_after_event_id,
                                   limit,
                                   descending,
                                   &account_ids,
                                   &result->error);
        }
        if(!ok)
        {
            result->success = false;
            break;
        }

        std::vector<uint64_t> merged;
        merged.reserve(global_ids.size() + account_ids.size());
        std::unordered_set<uint64_t> seen;
        const auto append_unique = [&](const std::vector<uint64_t>& source) {
            for(const auto id : source)
            {
                if(seen.insert(id).second)
                {
                    merged.push_back(id);
                }
            }
        };
        append_unique(global_ids);
        append_unique(account_ids);
        std::sort(merged.begin(), merged.end(), [descending](uint64_t left, uint64_t right) {
            return descending ? left > right : left < right;
        });
        if(static_cast<int>(merged.size()) > limit)
        {
            merged.resize(static_cast<size_t>(limit));
        }

        result->events.clear();
        for(const auto event_id : merged)
        {
            MudEventEnvelope event;
            if(load_event(worker_context, event_id, &event, nullptr))
            {
                result->events.push_back(std::move(event));
            }
        }
        if(descending)
        {
            std::reverse(result->events.begin(), result->events.end());
        }

        latest_event_id(worker_context, &result->latest_event_id, nullptr);
        if(!result->events.empty())
        {
            result->latest_event_id = result->events.back().event_id;
        }
        result->success = true;
        break;
    }
    case MudEventStoreOpType::check_rate_limit:
    {
        const auto key = make_rate_limit_key(result->request_bucket);
        auto* reply = static_cast<redisReply*>(redisCommand(worker_context, "INCR %s", key.c_str()));
        if(reply == nullptr || reply->type != REDIS_REPLY_INTEGER)
        {
            if(reply != nullptr)
            {
                freeReplyObject(reply);
            }
            result->success = false;
            result->error = "redis_rate_limit_incr_failed";
            break;
        }

        result->current_count = static_cast<int>(reply->integer);
        freeReplyObject(reply);
        if(result->current_count == 1)
        {
            reply = static_cast<redisReply*>(redisCommand(worker_context,
                                                         "EXPIRE %s %d",
                                                         key.c_str(),
                                                         std::max(1, result->request_window_sec)));
            if(reply != nullptr)
            {
                freeReplyObject(reply);
            }
        }
        result->allowed = result->current_count <= std::max(1, result->request_rate_limit);
        result->success = true;
        break;
    }
    default:
        result->success = false;
        result->error = "redis_unknown_event_operation";
        break;
    }
}

bool RedisMudEventStore::ensure_connected(redisContext** context, std::string* error)
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

    *context = redisConnectWithTimeout(m_redis_config.host.c_str(), m_redis_config.port, timeout);
    if(*context == nullptr || (*context)->err != 0)
    {
        if(error != nullptr)
        {
            *error = "redis_connect_failed";
        }
        if(*context != nullptr)
        {
            redisFree(*context);
            *context = nullptr;
        }
        return false;
    }

    if(!m_redis_config.password.empty())
    {
        auto* auth_reply = static_cast<redisReply*>(redisCommand(*context, "AUTH %s", m_redis_config.password.c_str()));
        if(!is_reply_ok(auth_reply))
        {
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

    if(m_redis_config.db > 0)
    {
        auto* select_reply = static_cast<redisReply*>(redisCommand(*context, "SELECT %d", m_redis_config.db));
        if(!is_reply_ok(select_reply))
        {
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

redisContext* RedisMudEventStore::ensure_worker_connection(std::string* error)
{
    thread_local const RedisMudEventStore* tls_owner = nullptr;
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

std::string RedisMudEventStore::make_seq_key() const
{
    return m_redis_config.key_prefix + ":mud:event:seq";
}

std::string RedisMudEventStore::make_global_index_key() const
{
    return m_redis_config.key_prefix + ":mud:event:index:global";
}

std::string RedisMudEventStore::make_account_index_key(const std::string& account) const
{
    return m_redis_config.key_prefix + ":mud:event:index:acct:" + account;
}

std::string RedisMudEventStore::make_event_data_key(uint64_t event_id) const
{
    return m_redis_config.key_prefix + ":mud:event:data:" + std::to_string(event_id);
}

std::string RedisMudEventStore::make_rate_limit_key(const std::string& bucket) const
{
    return m_redis_config.key_prefix + ":mud:rate:" + bucket;
}

uint64_t RedisMudEventStore::next_event_id(redisContext* context, std::string* error)
{
    auto* reply = static_cast<redisReply*>(redisCommand(context, "INCR %s", make_seq_key().c_str()));
    if(reply == nullptr || reply->type != REDIS_REPLY_INTEGER)
    {
        if(reply != nullptr)
        {
            freeReplyObject(reply);
        }
        if(error != nullptr)
        {
            *error = "redis_event_seq_failed";
        }
        return 0;
    }
    const auto next_id = static_cast<uint64_t>(reply->integer);
    freeReplyObject(reply);
    return next_id;
}

bool RedisMudEventStore::load_event(redisContext* context, uint64_t event_id, MudEventEnvelope* out_event, std::string* error)
{
    if(context == nullptr || out_event == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_invalid_load_event_args";
        }
        return false;
    }

    auto* reply = static_cast<redisReply*>(redisCommand(context,
                                                        "HMGET %s target_account type title content server_time_ms",
                                                        make_event_data_key(event_id).c_str()));
    if(reply == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_load_event_failed";
        }
        return false;
    }
    if(reply->type != REDIS_REPLY_ARRAY || reply->elements < 5)
    {
        freeReplyObject(reply);
        return false;
    }

    out_event->event_id = event_id;
    out_event->target_account = reply->element[0] != nullptr && reply->element[0]->type == REDIS_REPLY_STRING && reply->element[0]->str != nullptr
                                    ? reply->element[0]->str
                                    : "";
    out_event->type = reply->element[1] != nullptr && reply->element[1]->type == REDIS_REPLY_STRING && reply->element[1]->str != nullptr
                          ? reply->element[1]->str
                          : "";
    out_event->title = reply->element[2] != nullptr && reply->element[2]->type == REDIS_REPLY_STRING && reply->element[2]->str != nullptr
                           ? reply->element[2]->str
                           : "";
    out_event->content = reply->element[3] != nullptr && reply->element[3]->type == REDIS_REPLY_STRING && reply->element[3]->str != nullptr
                             ? reply->element[3]->str
                             : "";
    out_event->server_time_ms = 0;
    if(reply->element[4] != nullptr && reply->element[4]->type == REDIS_REPLY_STRING && reply->element[4]->str != nullptr)
    {
        try
        {
            out_event->server_time_ms = std::stoll(reply->element[4]->str);
        }
        catch(...)
        {
            out_event->server_time_ms = 0;
        }
    }
    freeReplyObject(reply);
    return !out_event->type.empty();
}

bool RedisMudEventStore::append_event(redisContext* context, MudEventEnvelope* event, std::string* error)
{
    if(context == nullptr || event == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_invalid_append_event_args";
        }
        return false;
    }

    event->event_id = next_event_id(context, error);
    if(event->event_id == 0)
    {
        return false;
    }
    if(event->server_time_ms <= 0)
    {
        event->server_time_ms = mud_now_ms();
    }

    auto* reply = static_cast<redisReply*>(redisCommand(context,
                                                        "HSET %s target_account %s type %s title %s content %s server_time_ms %lld",
                                                        make_event_data_key(event->event_id).c_str(),
                                                        event->target_account.c_str(),
                                                        event->type.c_str(),
                                                        event->title.c_str(),
                                                        event->content.c_str(),
                                                        static_cast<long long>(event->server_time_ms)));
    if(reply == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_hset_event_failed";
        }
        return false;
    }
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(context,
                                                  "EXPIRE %s %d",
                                                  make_event_data_key(event->event_id).c_str(),
                                                  std::max(60, m_mud_config.event_ttl_sec)));
    if(reply != nullptr)
    {
        freeReplyObject(reply);
    }

    const auto index_key = event->target_account.empty() ? make_global_index_key() : make_account_index_key(event->target_account);
    reply = static_cast<redisReply*>(redisCommand(context,
                                                  "ZADD %s %llu %llu",
                                                  index_key.c_str(),
                                                  static_cast<unsigned long long>(event->event_id),
                                                  static_cast<unsigned long long>(event->event_id)));
    if(reply == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_zadd_event_failed";
        }
        return false;
    }
    freeReplyObject(reply);
    trim_index(context, index_key);
    return true;
}

void RedisMudEventStore::trim_index(redisContext* context, const std::string& key)
{
    if(context == nullptr || key.empty())
    {
        return;
    }
    const int max_count = std::max(100, m_mud_config.event_index_max);
    auto* reply = static_cast<redisReply*>(redisCommand(context,
                                                        "ZREMRANGEBYRANK %s 0 -%d",
                                                        key.c_str(),
                                                        max_count + 1));
    if(reply != nullptr)
    {
        freeReplyObject(reply);
    }
}

bool RedisMudEventStore::collect_event_ids(redisContext* context,
                                           const std::string& key,
                                           uint64_t after_event_id,
                                           int limit,
                                           bool descending,
                                           std::vector<uint64_t>* out_ids,
                                           std::string* error)
{
    if(context == nullptr || out_ids == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_invalid_collect_ids_args";
        }
        return false;
    }

    out_ids->clear();
    redisReply* reply = nullptr;
    if(descending)
    {
        reply = static_cast<redisReply*>(redisCommand(context,
                                                      "ZREVRANGE %s 0 %d",
                                                      key.c_str(),
                                                      std::max(0, limit - 1)));
    }
    else
    {
        const std::string min_score = "(" + std::to_string(after_event_id);
        reply = static_cast<redisReply*>(redisCommand(context,
                                                      "ZRANGEBYSCORE %s %s +inf LIMIT 0 %d",
                                                      key.c_str(),
                                                      min_score.c_str(),
                                                      limit));
    }

    if(reply == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_collect_ids_failed";
        }
        return false;
    }
    if(reply->type != REDIS_REPLY_ARRAY)
    {
        freeReplyObject(reply);
        return false;
    }

    for(size_t index = 0; index < reply->elements; ++index)
    {
        auto* element = reply->element[index];
        if(element == nullptr || element->type != REDIS_REPLY_STRING || element->str == nullptr)
        {
            continue;
        }
        try
        {
            const auto event_id = static_cast<uint64_t>(std::stoull(element->str));
            if(!descending || event_id > after_event_id)
            {
                out_ids->push_back(event_id);
            }
        }
        catch(...)
        {
        }
    }
    freeReplyObject(reply);
    return true;
}

bool RedisMudEventStore::latest_event_id(redisContext* context, uint64_t* latest_id, std::string* error)
{
    if(latest_id == nullptr)
    {
        return false;
    }
    *latest_id = 0;

    auto* reply = static_cast<redisReply*>(redisCommand(context, "GET %s", make_seq_key().c_str()));
    if(reply == nullptr)
    {
        if(error != nullptr)
        {
            *error = "redis_get_latest_event_id_failed";
        }
        return false;
    }
    if(reply->type == REDIS_REPLY_STRING && reply->str != nullptr)
    {
        try
        {
            *latest_id = static_cast<uint64_t>(std::stoull(reply->str));
        }
        catch(...)
        {
            *latest_id = 0;
        }
    }
    freeReplyObject(reply);
    return true;
}

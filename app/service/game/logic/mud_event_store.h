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

#pragma once

#include "application_config.h"
#include "coromanager.h"
#include "mud_types.h"

#include <hiredis/hiredis.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>

enum class MudEventStoreOpType
{
    append_events,
    list_recent,
    list_after,
    check_rate_limit,
};

class MudEventStoreOpResult : public CoroResult
{
public:
    MudEventStoreOpResult() = default;
    ~MudEventStoreOpResult() override = default;

    void init(MudEventStoreOpType op,
              std::string account,
              uint64_t after_event_id,
              int limit,
              std::vector<MudEventEnvelope> events,
              std::string bucket,
              int rate_limit,
              int window_sec,
              std::function<void(MudEventStoreOpResult*)> worker_fn)
    {
        op_type = op;
        request_account = std::move(account);
        request_after_event_id = after_event_id;
        request_limit = limit;
        request_events = std::move(events);
        request_bucket = std::move(bucket);
        request_rate_limit = rate_limit;
        request_window_sec = window_sec;
        success = false;
        error.clear();
        events.clear();
        latest_event_id = 0;
        allowed = false;
        current_count = 0;
        m_worker_fn = std::move(worker_fn);
    }

    void worker() override
    {
        if(m_worker_fn)
        {
            m_worker_fn(this);
            return;
        }
        success = false;
        error = "missing mud event store worker";
    }

    void clear() override
    {
        request_account.clear();
        request_after_event_id = 0;
        request_limit = 0;
        request_events.clear();
        request_bucket.clear();
        request_rate_limit = 0;
        request_window_sec = 0;
        success = false;
        error.clear();
        events.clear();
        latest_event_id = 0;
        allowed = false;
        current_count = 0;
        m_worker_fn = nullptr;
    }

public:
    MudEventStoreOpType op_type = MudEventStoreOpType::append_events;
    std::string request_account;
    uint64_t request_after_event_id = 0;
    int request_limit = 0;
    std::vector<MudEventEnvelope> request_events;
    std::string request_bucket;
    int request_rate_limit = 0;
    int request_window_sec = 0;

    bool success = false;
    std::string error;
    std::vector<MudEventEnvelope> events;
    uint64_t latest_event_id = 0;
    bool allowed = false;
    int current_count = 0;

private:
    std::function<void(MudEventStoreOpResult*)> m_worker_fn;
};

class IMudEventStore
{
public:
    virtual ~IMudEventStore() = default;

public:
    virtual bool ready() const = 0;
    virtual void poll() = 0;
    virtual CoroAwaitable append_events(const std::vector<MudEventEnvelope>& events) = 0;
    virtual CoroAwaitable list_recent(const std::string& account, int limit) = 0;
    virtual CoroAwaitable list_after(const std::string& account, uint64_t after_event_id, int limit) = 0;
    virtual CoroAwaitable check_rate_limit(const std::string& bucket, int limit, int window_sec) = 0;
};

bool wait_mud_event_store_result(IMudEventStore* store,
                                 CoroAwaitable awaitable,
                                 MudEventStoreOpResult* out_result,
                                 int timeout_ms);

class RedisMudEventStore final : public IMudEventStore
{
public:
    RedisMudEventStore(const RedisConfig& redis_config, const MudConfig& mud_config);
    ~RedisMudEventStore() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable append_events(const std::vector<MudEventEnvelope>& events) override;
    CoroAwaitable list_recent(const std::string& account, int limit) override;
    CoroAwaitable list_after(const std::string& account, uint64_t after_event_id, int limit) override;
    CoroAwaitable check_rate_limit(const std::string& bucket, int limit, int window_sec) override;

private:
    class RedisMudEventManager;

private:
    MudEventStoreOpResult* alloc_result();
    void execute_operation(MudEventStoreOpResult* result);
    bool ensure_connected(redisContext** context, std::string* error = nullptr);
    redisContext* ensure_worker_connection(std::string* error = nullptr);
    std::string make_seq_key() const;
    std::string make_global_index_key() const;
    std::string make_account_index_key(const std::string& account) const;
    std::string make_event_data_key(uint64_t event_id) const;
    std::string make_rate_limit_key(const std::string& bucket) const;
    uint64_t next_event_id(redisContext* context, std::string* error);
    bool load_event(redisContext* context, uint64_t event_id, MudEventEnvelope* out_event, std::string* error);
    bool append_event(redisContext* context, MudEventEnvelope* event, std::string* error);
    void trim_index(redisContext* context, const std::string& key);
    bool collect_event_ids(redisContext* context,
                           const std::string& key,
                           uint64_t after_event_id,
                           int limit,
                           bool descending,
                           std::vector<uint64_t>* out_ids,
                           std::string* error);
    bool latest_event_id(redisContext* context, uint64_t* latest_id, std::string* error);

private:
    RedisConfig m_redis_config;
    MudConfig m_mud_config;
    redisContext* m_context = nullptr;
    bool m_ready = false;
    std::unique_ptr<RedisMudEventManager> m_manager;
    mutable std::mutex m_mutex;
};

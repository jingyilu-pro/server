//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "account_cache_store.h"
#include "application_config.h"

#include <hiredis/hiredis.h>

#include <memory>
#include <mutex>

class RedisAccountCacheManager : public CoroManager
{
public:
    explicit RedisAccountCacheManager(int worker_count);
    ~RedisAccountCacheManager() override;

public:
    CoroResult* alloc() override;
};

class RedisAccountCacheStore final : public IAccountCacheStore
{
public:
    explicit RedisAccountCacheStore(const RedisConfig& config);
    ~RedisAccountCacheStore() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable get_account(const std::string& account) override;
    CoroAwaitable put_account(const AccountRecord& record, int ttl_sec) override;
    CoroAwaitable erase_account(const std::string& account) override;

private:
    AccountCacheOpResult* alloc_result();
    void execute_operation(AccountCacheOpResult* result);
    bool ensure_connected(redisContext** context, std::string* error = nullptr);
    redisContext* ensure_worker_connection(std::string* error = nullptr);
    std::string make_account_key(const std::string& account) const;

private:
    RedisConfig m_config;
    redisContext* m_context = nullptr;
    bool m_ready = false;
    std::unique_ptr<RedisAccountCacheManager> m_manager;
    mutable std::mutex m_mutex;
};

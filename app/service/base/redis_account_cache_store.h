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

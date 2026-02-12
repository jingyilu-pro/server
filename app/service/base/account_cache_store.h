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

#include "account_repository.h"

#include <functional>
#include <optional>
#include <string>

enum class AccountCacheOpType
{
    get_account,
    put_account,
    erase_account,
};

class AccountCacheOpResult : public CoroResult
{
public:
    AccountCacheOpResult() = default;
    ~AccountCacheOpResult() override = default;

    void init(AccountCacheOpType op,
              std::string account,
              std::optional<AccountRecord> record,
              int ttl_sec,
              std::function<void(AccountCacheOpResult*)> worker_fn)
    {
        op_type = op;
        request_account = std::move(account);
        request_record = std::move(record);
        request_ttl_sec = ttl_sec;
        success = false;
        hit = false;
        put_ok = false;
        erase_ok = false;
        error.clear();
        record.reset();
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
        error = "missing account cache worker";
    }

    void clear() override
    {
        request_account.clear();
        request_record.reset();
        request_ttl_sec = 0;
        success = false;
        hit = false;
        put_ok = false;
        erase_ok = false;
        error.clear();
        record.reset();
        m_worker_fn = nullptr;
    }

public:
    AccountCacheOpType op_type = AccountCacheOpType::get_account;
    std::string request_account;
    std::optional<AccountRecord> request_record;
    int request_ttl_sec = 0;

    bool success = false;
    bool hit = false;
    bool put_ok = false;
    bool erase_ok = false;
    std::string error;
    std::optional<AccountRecord> record;

private:
    std::function<void(AccountCacheOpResult*)> m_worker_fn;
};

class IAccountCacheStore;
bool wait_account_cache_store_result(IAccountCacheStore* store,
                                     CoroAwaitable awaitable,
                                     AccountCacheOpResult* out_result,
                                     int timeout_ms);

class IAccountCacheStore
{
public:
    virtual ~IAccountCacheStore() = default;

public:
    virtual bool ready() const = 0;
    virtual void poll() = 0;
    virtual CoroAwaitable get_account(const std::string& account) = 0;
    virtual CoroAwaitable put_account(const AccountRecord& record, int ttl_sec) = 0;
    virtual CoroAwaitable erase_account(const std::string& account) = 0;
};


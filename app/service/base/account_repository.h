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

#include "coromanager.h"

#include <functional>
#include <optional>
#include <string>

struct AccountRecord
{
    std::string account;
    std::string password_hash;
    std::string salt;
};

enum class AccountRepositoryOpType
{
    find_account,
    verify_password,
    create_account
};

class AccountRepositoryOpResult : public CoroResult
{
public:
    AccountRepositoryOpResult() = default;
    ~AccountRepositoryOpResult() override = default;

    void init(AccountRepositoryOpType op,
              std::string account,
              std::string password,
              std::function<void(AccountRepositoryOpResult*)> worker_fn)
    {
        op_type = op;
        request_account = std::move(account);
        request_password = std::move(password);
        success = false;
        error.clear();
        record.reset();
        password_ok = false;
        create_ok = false;
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
        error = "missing account repository worker";
    }

    void clear() override
    {
        request_account.clear();
        request_password.clear();
        success = false;
        error.clear();
        record.reset();
        password_ok = false;
        create_ok = false;
        m_worker_fn = nullptr;
    }

public:
    AccountRepositoryOpType op_type = AccountRepositoryOpType::find_account;
    std::string request_account;
    std::string request_password;
    bool success = false;
    std::string error;
    std::optional<AccountRecord> record;
    bool password_ok = false;
    bool create_ok = false;

private:
    std::function<void(AccountRepositoryOpResult*)> m_worker_fn;
};

class IAccountRepository;
bool wait_account_repository_result(IAccountRepository* repository,
                                    CoroAwaitable awaitable,
                                    AccountRepositoryOpResult* out_result,
                                    int timeout_ms);

class IAccountRepository
{
public:
    virtual ~IAccountRepository() = default;

public:
    virtual bool ready() const = 0;
    virtual void poll() = 0;
    virtual CoroAwaitable find_account(const std::string& account) = 0;
    virtual CoroAwaitable verify_password(const std::string& account, const std::string& password) = 0;
    virtual CoroAwaitable create_account(const std::string& account, const std::string& password) = 0;
};

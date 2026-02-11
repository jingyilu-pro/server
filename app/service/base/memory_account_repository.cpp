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

#include "memory_account_repository.h"

MemoryAccountRepository::MemoryAccountRepository()
{
    create_account("user_0001", "pressure_password");
    create_account("user_0002", "pressure_password");
    create_account("user_0003", "pressure_password");
    create_account("user_0004", "pressure_password");
}

std::optional<AccountRecord> MemoryAccountRepository::find_account(const std::string& account)
{
    std::lock_guard lock(m_mutex);
    auto iter = m_accounts.find(account);
    if(iter == m_accounts.end())
    {
        return std::nullopt;
    }
    return iter->second;
}

bool MemoryAccountRepository::verify_password(const std::string& account, const std::string& password)
{
    std::lock_guard lock(m_mutex);
    auto iter = m_accounts.find(account);
    if(iter == m_accounts.end())
    {
        return false;
    }
    return iter->second.password_hash == password;
}

bool MemoryAccountRepository::create_account(const std::string& account, const std::string& password)
{
    if(account.empty())
    {
        return false;
    }

    std::lock_guard lock(m_mutex);
    if(m_accounts.find(account) != m_accounts.end())
    {
        return false;
    }

    AccountRecord record;
    record.account = account;
    record.password_hash = password;
    record.salt = "";
    m_accounts[account] = record;
    return true;
}


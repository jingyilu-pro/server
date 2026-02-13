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

#include "cached_account_repository.h"

CachedAccountRepository::CachedAccountRepository(std::shared_ptr<IAccountRepository> inner,
                                                 std::shared_ptr<IAccountCacheStore> cache_store,
                                                 int /*cache_ttl_sec*/,
                                                 int /*coro_workers*/,
                                                 int /*password_hash_iterations*/)
    : m_inner(std::move(inner)),
      m_cache_store(std::move(cache_store))
{
}

CachedAccountRepository::~CachedAccountRepository() = default;

bool CachedAccountRepository::ready() const
{
    return m_inner != nullptr && m_inner->ready();
}

void CachedAccountRepository::poll()
{
    if(m_inner != nullptr)
    {
        m_inner->poll();
    }
    if(m_cache_store != nullptr)
    {
        m_cache_store->poll();
    }
}

CoroAwaitable CachedAccountRepository::find_account(const std::string& account)
{
    if(m_inner == nullptr)
    {
        return CoroAwaitable{nullptr, nullptr};
    }
    return m_inner->find_account(account);
}

CoroAwaitable CachedAccountRepository::verify_password(const std::string& account, const std::string& password)
{
    if(m_inner == nullptr)
    {
        return CoroAwaitable{nullptr, nullptr};
    }
    return m_inner->verify_password(account, password);
}

CoroAwaitable CachedAccountRepository::create_account(const std::string& account, const std::string& password)
{
    if(m_inner == nullptr)
    {
        return CoroAwaitable{nullptr, nullptr};
    }
    return m_inner->create_account(account, password);
}


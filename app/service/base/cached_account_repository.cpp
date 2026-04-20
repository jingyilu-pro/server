//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
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


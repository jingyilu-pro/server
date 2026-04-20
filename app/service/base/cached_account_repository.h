//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "account_cache_store.h"
#include "account_repository.h"

#include <memory>

class CachedAccountRepository final : public IAccountRepository
{
public:
    CachedAccountRepository(std::shared_ptr<IAccountRepository> inner,
                            std::shared_ptr<IAccountCacheStore> cache_store,
                            int cache_ttl_sec,
                            int coro_workers,
                            int password_hash_iterations);
    ~CachedAccountRepository() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable find_account(const std::string& account) override;
    CoroAwaitable verify_password(const std::string& account, const std::string& password) override;
    CoroAwaitable create_account(const std::string& account, const std::string& password) override;

private:
    std::shared_ptr<IAccountRepository> m_inner;
    std::shared_ptr<IAccountCacheStore> m_cache_store;
};

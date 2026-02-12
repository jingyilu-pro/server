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
#include "account_repository.h"

#include <memory>
#include <mutex>

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
    class CachedAccountRepoManager;

private:
    std::shared_ptr<IAccountRepository> m_inner;
    std::shared_ptr<IAccountCacheStore> m_cache_store;
    int m_cache_ttl_sec = 300;
    int m_coro_workers = 1;
    int m_password_hash_iterations = 120000;
    mutable std::mutex m_wait_mutex;
    std::unique_ptr<CachedAccountRepoManager> m_manager;
};

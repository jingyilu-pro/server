//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "account_cache_store.h"

#include <memory>

class NoopAccountCacheStore final : public IAccountCacheStore
{
public:
    NoopAccountCacheStore();
    ~NoopAccountCacheStore() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable get_account(const std::string& account) override;
    CoroAwaitable put_account(const AccountRecord& record, int ttl_sec) override;
    CoroAwaitable erase_account(const std::string& account) override;

private:
    class NoopAccountCacheManager;

private:
    std::unique_ptr<NoopAccountCacheManager> m_manager;
};

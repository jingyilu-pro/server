//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "noop_account_cache_store.h"

namespace
{

class NoopAccountCacheOpManager : public CoroManager
{
public:
    NoopAccountCacheOpManager()
        : CoroManager(1)
    {
        init();
    }
    ~NoopAccountCacheOpManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<AccountCacheOpResult>();
        return inner_alloc();
    }
};

} // namespace

class NoopAccountCacheStore::NoopAccountCacheManager : public NoopAccountCacheOpManager
{
};

NoopAccountCacheStore::NoopAccountCacheStore()
{
    m_manager = std::make_unique<NoopAccountCacheManager>();
}

NoopAccountCacheStore::~NoopAccountCacheStore() = default;

bool NoopAccountCacheStore::ready() const
{
    return true;
}

void NoopAccountCacheStore::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

CoroAwaitable NoopAccountCacheStore::get_account(const std::string& account)
{
    auto* result = dynamic_cast<AccountCacheOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountCacheOpType::get_account,
                 account,
                 std::nullopt,
                 0,
                 [](AccountCacheOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->hit = false;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopAccountCacheStore::put_account(const AccountRecord& record, int ttl_sec)
{
    auto* result = dynamic_cast<AccountCacheOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountCacheOpType::put_account,
                 record.account,
                 record,
                 ttl_sec,
                 [](AccountCacheOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->put_ok = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopAccountCacheStore::erase_account(const std::string& account)
{
    auto* result = dynamic_cast<AccountCacheOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountCacheOpType::erase_account,
                 account,
                 std::nullopt,
                 0,
                 [](AccountCacheOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->erase_ok = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

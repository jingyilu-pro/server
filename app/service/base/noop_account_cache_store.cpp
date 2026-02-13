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

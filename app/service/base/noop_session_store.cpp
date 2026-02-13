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

#include "noop_session_store.h"

namespace
{

class NoopSessionOpManager : public CoroManager
{
public:
    NoopSessionOpManager()
        : CoroManager(1)
    {
        init();
    }
    ~NoopSessionOpManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<SessionStoreOpResult>();
        return inner_alloc();
    }
};

} // namespace

class NoopSessionStore::NoopSessionManager : public NoopSessionOpManager
{
};

NoopSessionStore::NoopSessionStore()
{
    m_manager = std::make_unique<NoopSessionManager>();
}

NoopSessionStore::~NoopSessionStore() = default;

bool NoopSessionStore::ready() const
{
    return true;
}

void NoopSessionStore::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

CoroAwaitable NoopSessionStore::get_session(const std::string& account)
{
    auto* result = dynamic_cast<SessionStoreOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::get_session,
                 account,
                 std::nullopt,
                 0,
                 [](SessionStoreOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->hit = false;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopSessionStore::upsert_session(const SessionRecord& session, int ttl_sec)
{
    auto* result = dynamic_cast<SessionStoreOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::upsert_session,
                 session.account,
                 session,
                 ttl_sec,
                 [](SessionStoreOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->upsert_ok = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopSessionStore::touch_session(const std::string& account, int ttl_sec)
{
    auto* result = dynamic_cast<SessionStoreOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::touch_session,
                 account,
                 std::nullopt,
                 ttl_sec,
                 [](SessionStoreOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->touch_ok = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopSessionStore::remove_session(const std::string& account)
{
    auto* result = dynamic_cast<SessionStoreOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(SessionStoreOpType::remove_session,
                 account,
                 std::nullopt,
                 0,
                 [](SessionStoreOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->remove_ok = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

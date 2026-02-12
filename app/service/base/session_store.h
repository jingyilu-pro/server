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

struct SessionRecord
{
    std::string account;
    std::string token_digest;
    int64_t expire_at = 0;
};

enum class SessionStoreOpType
{
    get_session,
    upsert_session,
    touch_session,
    remove_session,
};

class SessionStoreOpResult : public CoroResult
{
public:
    SessionStoreOpResult() = default;
    ~SessionStoreOpResult() override = default;

    void init(SessionStoreOpType op,
              std::string account,
              std::optional<SessionRecord> session,
              int ttl_sec,
              std::function<void(SessionStoreOpResult*)> worker_fn)
    {
        op_type = op;
        request_account = std::move(account);
        request_session = std::move(session);
        request_ttl_sec = ttl_sec;
        success = false;
        hit = false;
        touch_ok = false;
        upsert_ok = false;
        remove_ok = false;
        error.clear();
        session.reset();
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
        error = "missing session store worker";
    }

    void clear() override
    {
        request_account.clear();
        request_session.reset();
        request_ttl_sec = 0;
        success = false;
        hit = false;
        touch_ok = false;
        upsert_ok = false;
        remove_ok = false;
        error.clear();
        session.reset();
        m_worker_fn = nullptr;
    }

public:
    SessionStoreOpType op_type = SessionStoreOpType::get_session;
    std::string request_account;
    std::optional<SessionRecord> request_session;
    int request_ttl_sec = 0;

    bool success = false;
    bool hit = false;
    bool touch_ok = false;
    bool upsert_ok = false;
    bool remove_ok = false;
    std::string error;
    std::optional<SessionRecord> session;

private:
    std::function<void(SessionStoreOpResult*)> m_worker_fn;
};

class ISessionStore;
bool wait_session_store_result(ISessionStore* store,
                               CoroAwaitable awaitable,
                               SessionStoreOpResult* out_result,
                               int timeout_ms);

class ISessionStore
{
public:
    virtual ~ISessionStore() = default;

public:
    virtual bool ready() const = 0;
    virtual void poll() = 0;
    virtual CoroAwaitable get_session(const std::string& account) = 0;
    virtual CoroAwaitable upsert_session(const SessionRecord& session, int ttl_sec) = 0;
    virtual CoroAwaitable touch_session(const std::string& account, int ttl_sec) = 0;
    virtual CoroAwaitable remove_session(const std::string& account) = 0;
};


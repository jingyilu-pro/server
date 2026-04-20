//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "session_store.h"

#include <memory>

class NoopSessionStore final : public ISessionStore
{
public:
    NoopSessionStore();
    ~NoopSessionStore() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable get_session(const std::string& account) override;
    CoroAwaitable upsert_session(const SessionRecord& session, int ttl_sec) override;
    CoroAwaitable touch_session(const std::string& account, int ttl_sec) override;
    CoroAwaitable remove_session(const std::string& account) override;

private:
    class NoopSessionManager;

private:
    std::unique_ptr<NoopSessionManager> m_manager;
};

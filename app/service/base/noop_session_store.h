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


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


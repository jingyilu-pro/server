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

#include "application_config.h"
#include "token_provider.h"

#include <jwt.h>

#include <mutex>

class JwtTokenProvider : public ITokenProvider
{
public:
    explicit JwtTokenProvider(const JwtConfig& config);
    ~JwtTokenProvider() override;

public:
    std::string issue(const std::string& subject, int expire_sec) override;
    std::optional<VerifiedToken> verify(const std::string& token) override;

private:
    bool ensure_signing_key_loaded();

private:
    JwtConfig m_config;
    std::string m_hs256_jwk_json;
    jwk_set_t* m_signing_jwk_set = nullptr;
    const jwk_item_t* m_signing_item = nullptr;
    std::mutex m_mutex;
};


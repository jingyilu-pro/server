//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
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


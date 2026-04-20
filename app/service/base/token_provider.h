//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include <optional>
#include <string>

struct VerifiedToken
{
    std::string subject;
    std::string issuer;
    int64_t issued_at = 0;
    int64_t expire_at = 0;
};

class ITokenProvider
{
public:
    virtual ~ITokenProvider() = default;

public:
    virtual std::string issue(const std::string& subject, int expire_sec) = 0;
    virtual std::optional<VerifiedToken> verify(const std::string& token) = 0;
};


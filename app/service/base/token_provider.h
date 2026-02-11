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


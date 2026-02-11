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

#include "jwt_token_provider.h"

#include "log/glogger.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <string>

namespace
{

constexpr const char* kMockTokenPrefix = "mock.jwt.token.";

std::string base64url_encode(const std::string& input)
{
    static constexpr char kBase64Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64;
    base64.reserve(((input.size() + 2) / 3) * 4);

    int val = 0;
    int valb = -6;
    for(unsigned char c : input)
    {
        val = (val << 8) + c;
        valb += 8;
        while(valb >= 0)
        {
            base64.push_back(kBase64Alphabet[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if(valb > -6)
    {
        base64.push_back(kBase64Alphabet[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while(base64.size() % 4)
    {
        base64.push_back('=');
    }

    for(char& ch : base64)
    {
        if(ch == '+')
        {
            ch = '-';
        }
        else if(ch == '/')
        {
            ch = '_';
        }
    }
    while(!base64.empty() && base64.back() == '=')
    {
        base64.pop_back();
    }

    return base64;
}

int jwt_build_claim_callback(jwt_t* jwt, jwt_config_t* config)
{
    if(jwt == nullptr || config == nullptr || config->ctx == nullptr)
    {
        return 1;
    }

    auto* claim_subject = static_cast<const std::string*>(config->ctx);
    jwt_value_t subject_claim;
    jwt_set_SET_STR(&subject_claim, "sub", claim_subject->c_str());
    if(jwt_claim_set(jwt, &subject_claim) != JWT_VALUE_ERR_NONE)
    {
        return 1;
    }

    return 0;
}

int jwt_read_claim_callback(jwt_t* jwt, jwt_config_t* config)
{
    if(jwt == nullptr || config == nullptr || config->ctx == nullptr)
    {
        return 1;
    }

    auto* verified = static_cast<VerifiedToken*>(config->ctx);
    jwt_value_t subject_claim;
    jwt_set_GET_STR(&subject_claim, "sub");
    auto subject_result = jwt_claim_get(jwt, &subject_claim);
    if(subject_result == JWT_VALUE_ERR_NONE && subject_claim.str_val != nullptr)
    {
        verified->subject = subject_claim.str_val;
    }

    jwt_value_t issuer_claim;
    jwt_set_GET_STR(&issuer_claim, "iss");
    auto issuer_result = jwt_claim_get(jwt, &issuer_claim);
    if(issuer_result == JWT_VALUE_ERR_NONE && issuer_claim.str_val != nullptr)
    {
        verified->issuer = issuer_claim.str_val;
    }

    return 0;
}

} // namespace

JwtTokenProvider::JwtTokenProvider(const JwtConfig& config)
    : m_config(config)
{
    if(m_config.secret.empty())
    {
        spdlog::warn("jwt secret is empty, token provider will fallback to insecure mock token");
    }
}

JwtTokenProvider::~JwtTokenProvider()
{
    std::lock_guard lock(m_mutex);
    if(m_signing_jwk_set != nullptr)
    {
        jwks_free(m_signing_jwk_set);
        m_signing_jwk_set = nullptr;
        m_signing_item = nullptr;
    }
}

std::string JwtTokenProvider::issue(const std::string& subject, int expire_sec)
{
    if(subject.empty())
    {
        return {};
    }

    std::lock_guard lock(m_mutex);
    if(!ensure_signing_key_loaded())
    {
        return std::string(kMockTokenPrefix) + subject;
    }

    jwt_builder_t* builder = jwt_builder_new();
    if(builder == nullptr)
    {
        return {};
    }

    auto free_builder = [&builder]() {
        if(builder != nullptr)
        {
            jwt_builder_free(builder);
            builder = nullptr;
        }
    };

    if(jwt_builder_setkey(builder, JWT_ALG_HS256, m_signing_item) != 0)
    {
        spdlog::error("jwt_builder_setkey failed: {}", jwt_builder_error_msg(builder));
        free_builder();
        return {};
    }

    if(!m_config.issuer.empty())
    {
        jwt_value_t issuer_claim;
        jwt_set_SET_STR(&issuer_claim, "iss", m_config.issuer.c_str());
        issuer_claim.replace = 1;
        if(jwt_builder_claim_set(builder, &issuer_claim) != JWT_VALUE_ERR_NONE)
        {
            spdlog::error("set iss claim failed: {}", jwt_builder_error_msg(builder));
            free_builder();
            return {};
        }
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    jwt_value_t iat_claim;
    jwt_set_SET_INT(&iat_claim, "iat", static_cast<jwt_long_t>(now));
    iat_claim.replace = 1;
    if(jwt_builder_claim_set(builder, &iat_claim) != JWT_VALUE_ERR_NONE)
    {
        spdlog::error("set iat claim failed: {}", jwt_builder_error_msg(builder));
        free_builder();
        return {};
    }

    const int effective_expire = std::max(1, expire_sec);
    jwt_value_t exp_claim;
    jwt_set_SET_INT(&exp_claim, "exp", static_cast<jwt_long_t>(now + effective_expire));
    exp_claim.replace = 1;
    if(jwt_builder_claim_set(builder, &exp_claim) != JWT_VALUE_ERR_NONE)
    {
        spdlog::error("set exp claim failed: {}", jwt_builder_error_msg(builder));
        free_builder();
        return {};
    }

    std::string subject_copy = subject;
    if(jwt_builder_setcb(builder, &jwt_build_claim_callback, &subject_copy) != 0)
    {
        spdlog::error("jwt_builder_setcb failed: {}", jwt_builder_error_msg(builder));
        free_builder();
        return {};
    }

    char* encoded = jwt_builder_generate(builder);
    if(encoded == nullptr)
    {
        spdlog::error("jwt_builder_generate failed: {}", jwt_builder_error_msg(builder));
        free_builder();
        return {};
    }

    std::string token = encoded;
    free(encoded);
    free_builder();
    return token;
}

std::optional<VerifiedToken> JwtTokenProvider::verify(const std::string& token)
{
    if(token.empty())
    {
        return std::nullopt;
    }

    std::lock_guard lock(m_mutex);
    if(!ensure_signing_key_loaded())
    {
        if(token.rfind(kMockTokenPrefix, 0) == 0)
        {
            VerifiedToken verified;
            verified.subject = token.substr(std::string(kMockTokenPrefix).size());
            verified.issuer = m_config.issuer;
            return verified;
        }
        return std::nullopt;
    }

    jwt_checker_t* checker = jwt_checker_new();
    if(checker == nullptr)
    {
        return std::nullopt;
    }

    auto free_checker = [&checker]() {
        if(checker != nullptr)
        {
            jwt_checker_free(checker);
            checker = nullptr;
        }
    };

    if(jwt_checker_setkey(checker, JWT_ALG_HS256, m_signing_item) != 0)
    {
        spdlog::error("jwt_checker_setkey failed: {}", jwt_checker_error_msg(checker));
        free_checker();
        return std::nullopt;
    }

    if(!m_config.issuer.empty())
    {
        if(jwt_checker_claim_set(checker, JWT_CLAIM_ISS, m_config.issuer.c_str()) != 0)
        {
            spdlog::error("jwt_checker_claim_set(iss) failed: {}", jwt_checker_error_msg(checker));
            free_checker();
            return std::nullopt;
        }
    }

    VerifiedToken verified;
    if(jwt_checker_setcb(checker, &jwt_read_claim_callback, &verified) != 0)
    {
        spdlog::error("jwt_checker_setcb failed: {}", jwt_checker_error_msg(checker));
        free_checker();
        return std::nullopt;
    }

    if(jwt_checker_verify(checker, token.c_str()) != 0)
    {
        spdlog::warn("jwt verify failed: {}", jwt_checker_error_msg(checker));
        free_checker();
        return std::nullopt;
    }

    if(verified.subject.empty())
    {
        const char* subject_claim = jwt_checker_claim_get(checker, JWT_CLAIM_SUB);
        if(subject_claim != nullptr)
        {
            verified.subject = subject_claim;
        }
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    const char* exp_claim = jwt_checker_claim_get(checker, JWT_CLAIM_EXP);
    if(exp_claim != nullptr)
    {
        try
        {
            verified.expire_at = std::stoll(exp_claim);
        }
        catch(...)
        {
            verified.expire_at = 0;
        }
    }

    const char* iat_claim = jwt_checker_claim_get(checker, JWT_CLAIM_IAT);
    if(iat_claim != nullptr)
    {
        try
        {
            verified.issued_at = std::stoll(iat_claim);
        }
        catch(...)
        {
            verified.issued_at = 0;
        }
    }

    if(verified.expire_at > 0 && verified.expire_at < now)
    {
        free_checker();
        return std::nullopt;
    }

    free_checker();
    return verified;
}

bool JwtTokenProvider::ensure_signing_key_loaded()
{
    if(m_signing_item != nullptr && m_signing_jwk_set != nullptr)
    {
        return true;
    }

    if(m_config.secret.empty())
    {
        return false;
    }

    const auto secret_base64url = base64url_encode(m_config.secret);
    m_hs256_jwk_json = "{\"keys\":[{\"kty\":\"oct\",\"alg\":\"HS256\",\"k\":\"" +
                       secret_base64url +
                       "\"}]}";

    if(m_signing_jwk_set != nullptr)
    {
        jwks_free(m_signing_jwk_set);
        m_signing_jwk_set = nullptr;
        m_signing_item = nullptr;
    }

    m_signing_jwk_set = jwks_create(m_hs256_jwk_json.c_str());
    if(m_signing_jwk_set == nullptr || jwks_error(m_signing_jwk_set) != 0)
    {
        spdlog::error("jwks_create failed for hs256 key: {}",
                      m_signing_jwk_set == nullptr ? "null key set" : jwks_error_msg(m_signing_jwk_set));
        if(m_signing_jwk_set != nullptr)
        {
            jwks_free(m_signing_jwk_set);
            m_signing_jwk_set = nullptr;
        }
        return false;
    }

    m_signing_item = jwks_item_get(m_signing_jwk_set, 0);
    if(m_signing_item == nullptr)
    {
        spdlog::error("jwks_item_get failed for signing key");
        jwks_free(m_signing_jwk_set);
        m_signing_jwk_set = nullptr;
        return false;
    }

    return true;
}

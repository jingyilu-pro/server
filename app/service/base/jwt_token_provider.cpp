//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "jwt_token_provider.h"

#include "log/glogger.h"

#include <array>
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

std::optional<std::string> base64url_decode(const std::string& input)
{
    static constexpr std::array<int, 256> kDecodeTable = []() {
        std::array<int, 256> table{};
        table.fill(-1);
        for(int index = 0; index < 26; ++index)
        {
            table[static_cast<std::size_t>('A' + index)] = index;
            table[static_cast<std::size_t>('a' + index)] = 26 + index;
        }
        for(int index = 0; index < 10; ++index)
        {
            table[static_cast<std::size_t>('0' + index)] = 52 + index;
        }
        table[static_cast<std::size_t>('+')] = 62;
        table[static_cast<std::size_t>('/')] = 63;
        table[static_cast<std::size_t>('-')] = 62;
        table[static_cast<std::size_t>('_')] = 63;
        table[static_cast<std::size_t>('=')] = 0;
        return table;
    }();

    std::string normalized = input;
    for(char& ch : normalized)
    {
        if(ch == '-')
        {
            ch = '+';
        }
        else if(ch == '_')
        {
            ch = '/';
        }
        else if(kDecodeTable[static_cast<unsigned char>(ch)] < 0)
        {
            return std::nullopt;
        }
    }

    while(normalized.size() % 4 != 0)
    {
        normalized.push_back('=');
    }

    std::string output;
    output.reserve((normalized.size() / 4) * 3);

    int value = 0;
    int value_bits = -8;
    for(unsigned char ch : normalized)
    {
        if(ch == '=')
        {
            break;
        }
        const int decoded = kDecodeTable[ch];
        if(decoded < 0)
        {
            return std::nullopt;
        }
        value = (value << 6) + decoded;
        value_bits += 6;
        if(value_bits >= 0)
        {
            output.push_back(static_cast<char>((value >> value_bits) & 0xFF));
            value_bits -= 8;
        }
    }

    return output;
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
        return std::string(kMockTokenPrefix) + base64url_encode(subject);
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
            const std::string encoded_subject = token.substr(std::string(kMockTokenPrefix).size());
            if(auto decoded_subject = base64url_decode(encoded_subject); decoded_subject.has_value())
            {
                verified.subject = *decoded_subject;
            }
            else
            {
                // Backward-compatible fallback for older dev mock tokens that appended raw subject text.
                verified.subject = encoded_subject;
            }
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

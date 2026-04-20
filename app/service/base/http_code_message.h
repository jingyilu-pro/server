//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include <string>

namespace http_code_message
{

namespace transport
{

namespace status
{

inline constexpr int kOk = 200;
inline constexpr int kBadRequest = 400;
inline constexpr int kNotFound = 404;
inline constexpr int kMethodNotAllowed = 405;
inline constexpr int kUnsupportedMediaType = 415;
inline constexpr int kInternalServerError = 500;

} // namespace status

namespace message
{

inline constexpr const char* kInternalError = "internal error";
inline constexpr const char* kMethodNotAllowed = "method not allowed";
inline constexpr const char* kUnsupportedMediaType = "unsupported media type";
inline constexpr const char* kInvalidUri = "invalid uri";
inline constexpr const char* kNotFound = "not found";
inline constexpr const char* kEmptyProtobufBody = "empty protobuf body";
inline constexpr const char* kInvalidProtobuf = "invalid protobuf";

} // namespace message

} // namespace transport

namespace gateway
{

namespace code
{

inline constexpr int kSuccess = 0;

inline constexpr int kAccountAlreadyExistsOrInvalidInput = 40001;
inline constexpr int kCharacterAlreadyExistsOrInvalidInput = 40002;
inline constexpr int kInvalidMudCommand = 40003;
inline constexpr int kCharacterNotFound = 40401;

inline constexpr int kInvalidAccountOrPassword = 40101;
inline constexpr int kMissingJwt = 40101;
inline constexpr int kInvalidOrExpiredJwt = 40102;
inline constexpr int kJwtSubjectMismatch = 40103;

inline constexpr int kAccountRepositoryUnavailable = 50001;
inline constexpr int kTokenProviderUnavailable = 50002;
inline constexpr int kTokenIssueFailed = 50003;
inline constexpr int kMudWorldUnavailable = 50004;
inline constexpr int kMudPlayerRepositoryUnavailable = 50005;
inline constexpr int kLoginServiceUnavailable = 50011;
inline constexpr int kGameServiceUnavailable = 50012;
inline constexpr int kLoginAndGameServiceUnavailable = 50013;

} // namespace code

namespace message
{

inline constexpr const char* kOk = "ok";
inline constexpr const char* kRegistered = "registered";
inline constexpr const char* kAccountRepositoryUnavailable = "account repository unavailable";
inline constexpr const char* kAccountAlreadyExistsOrInvalidInput = "account already exists or invalid input";
inline constexpr const char* kCharacterAlreadyExistsOrInvalidInput = "character already exists or invalid input";
inline constexpr const char* kInvalidMudCommand = "invalid mud command";
inline constexpr const char* kCharacterNotFound = "character not found";
inline constexpr const char* kInvalidAccountOrPassword = "invalid account or password";
inline constexpr const char* kTokenProviderUnavailable = "token provider unavailable";
inline constexpr const char* kTokenIssueFailed = "token issue failed";
inline constexpr const char* kMudWorldUnavailable = "mud world unavailable";
inline constexpr const char* kMudPlayerRepositoryUnavailable = "mud player repository unavailable";
inline constexpr const char* kLoginServiceUnavailable = "login service unavailable";
inline constexpr const char* kGameServiceUnavailable = "game service unavailable";
inline constexpr const char* kLoginAndGameServiceUnavailable = "login and game service unavailable";
inline constexpr const char* kMissingJwt = "missing jwt";
inline constexpr const char* kInvalidOrExpiredJwt = "invalid or expired jwt";
inline constexpr const char* kJwtSubjectMismatch = "jwt subject mismatch";

} // namespace message

template <typename TResponse>
inline void set_code_message(TResponse* response, int code, const char* message)
{
    if(response == nullptr)
    {
        return;
    }

    response->set_code(code);
    response->set_message(message == nullptr ? "" : message);
}

template <typename TResponse>
inline void set_code_message(TResponse* response, int code, const std::string& message)
{
    if(response == nullptr)
    {
        return;
    }

    response->set_code(code);
    response->set_message(message);
}

} // namespace gateway

} // namespace http_code_message

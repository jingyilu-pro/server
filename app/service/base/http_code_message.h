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

inline constexpr int kInvalidAccountOrPassword = 40101;
inline constexpr int kMissingJwt = 40101;
inline constexpr int kInvalidOrExpiredJwt = 40102;
inline constexpr int kJwtSubjectMismatch = 40103;

inline constexpr int kAccountRepositoryUnavailable = 50001;
inline constexpr int kTokenProviderUnavailable = 50002;
inline constexpr int kTokenIssueFailed = 50003;
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
inline constexpr const char* kInvalidAccountOrPassword = "invalid account or password";
inline constexpr const char* kTokenProviderUnavailable = "token provider unavailable";
inline constexpr const char* kTokenIssueFailed = "token issue failed";
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

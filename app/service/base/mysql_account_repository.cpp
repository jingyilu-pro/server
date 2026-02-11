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

#include "mysql_account_repository.h"

#include "log/glogger.h"

#include <algorithm>

namespace
{

constexpr const char* kAccountTableSql =
    "CREATE TABLE IF NOT EXISTS account ("
    "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
    "account VARCHAR(128) NOT NULL UNIQUE,"
    "password_hash VARCHAR(256) NOT NULL,"
    "salt VARCHAR(128) NOT NULL DEFAULT '',"
    "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
    "updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

} // namespace

MySqlAccountRepository::MySqlAccountRepository(const MySqlConfig& config)
    : m_config(config)
{
    m_ready = ensure_connected() && ensure_table();
}

MySqlAccountRepository::~MySqlAccountRepository()
{
    std::lock_guard lock(m_mutex);
    if(m_mysql != nullptr)
    {
        mysql_close(m_mysql);
        m_mysql = nullptr;
    }
}

bool MySqlAccountRepository::ready() const
{
    return m_ready;
}

std::optional<AccountRecord> MySqlAccountRepository::find_account(const std::string& account)
{
    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return std::nullopt;
    }

    const auto escaped_account = escape_string(account);
    const auto sql = "SELECT account,password_hash,salt FROM account WHERE account='" + escaped_account + "' LIMIT 1";
    if(mysql_query(m_mysql, sql.c_str()) != 0)
    {
        spdlog::error("mysql query failed: {}", mysql_error(m_mysql));
        return std::nullopt;
    }

    MYSQL_RES* result = mysql_store_result(m_mysql);
    if(result == nullptr)
    {
        return std::nullopt;
    }

    auto* row = mysql_fetch_row(result);
    if(row == nullptr)
    {
        mysql_free_result(result);
        return std::nullopt;
    }

    AccountRecord record;
    record.account = row[0] == nullptr ? "" : row[0];
    record.password_hash = row[1] == nullptr ? "" : row[1];
    record.salt = row[2] == nullptr ? "" : row[2];

    mysql_free_result(result);
    return record;
}

bool MySqlAccountRepository::verify_password(const std::string& account, const std::string& password)
{
    auto record = find_account(account);
    if(!record)
    {
        return false;
    }
    return record->password_hash == password;
}

bool MySqlAccountRepository::create_account(const std::string& account, const std::string& password)
{
    if(account.empty())
    {
        return false;
    }

    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return false;
    }

    const auto escaped_account = escape_string(account);
    const auto escaped_password = escape_string(password);

    const auto sql = "INSERT INTO account(account,password_hash,salt) VALUES('" +
                     escaped_account +
                     "','" +
                     escaped_password +
                     "','')";
    if(mysql_query(m_mysql, sql.c_str()) != 0)
    {
        if(mysql_errno(m_mysql) == 1062)
        {
            return false;
        }
        spdlog::error("mysql insert failed: {}", mysql_error(m_mysql));
        return false;
    }

    return true;
}

bool MySqlAccountRepository::ensure_connected()
{
    if(m_mysql != nullptr)
    {
        return true;
    }

    m_mysql = mysql_init(nullptr);
    if(m_mysql == nullptr)
    {
        spdlog::error("mysql_init failed");
        return false;
    }

    const unsigned int timeout_sec = static_cast<unsigned int>(std::max(1, m_config.connect_timeout_ms / 1000));
    mysql_options(m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec);

    const my_bool ssl_verify_server_cert = 0;
    const my_bool ssl_enforce = 0;
    mysql_options(m_mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &ssl_verify_server_cert);
    mysql_options(m_mysql, MYSQL_OPT_SSL_ENFORCE, &ssl_enforce);

    if(mysql_real_connect(m_mysql,
                          m_config.host.c_str(),
                          m_config.user.c_str(),
                          m_config.password.c_str(),
                          m_config.database.c_str(),
                          m_config.port,
                          nullptr,
                          0) == nullptr)
    {
        spdlog::error("mysql_real_connect failed host={} port={} user={} db={} err={}",
                      m_config.host,
                      m_config.port,
                      m_config.user,
                      m_config.database,
                      mysql_error(m_mysql));
        mysql_close(m_mysql);
        m_mysql = nullptr;
        return false;
    }

    return true;
}

bool MySqlAccountRepository::ensure_table()
{
    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return false;
    }

    if(mysql_query(m_mysql, kAccountTableSql) != 0)
    {
        spdlog::error("mysql create table failed: {}", mysql_error(m_mysql));
        return false;
    }
    return true;
}

std::string MySqlAccountRepository::escape_string(const std::string& input)
{
    if(m_mysql == nullptr)
    {
        return input;
    }

    std::string out;
    out.resize(input.size() * 2 + 1);
    const auto escaped_length = mysql_real_escape_string(m_mysql,
                                                          out.data(),
                                                          input.data(),
                                                          static_cast<unsigned long>(input.size()));
    out.resize(escaped_length);
    return out;
}

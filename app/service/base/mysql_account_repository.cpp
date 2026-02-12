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

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

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

constexpr const char* kFindAccountSql =
    "SELECT account,password_hash,salt FROM account WHERE account=? LIMIT 1";

constexpr const char* kInsertAccountSql =
    "INSERT INTO account(account,password_hash,salt) VALUES(?,?,?)";

constexpr int kDefaultPasswordHashIterations = 120000;

std::string bytes_to_hex(const unsigned char* data, size_t len)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for(size_t i = 0; i < len; ++i)
    {
        output << std::setw(2) << static_cast<int>(data[i]);
    }
    return output.str();
}

std::string sha256_hex(const std::string& input)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(context == nullptr)
    {
        return {};
    }

    bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
              EVP_DigestUpdate(context, input.data(), input.size()) == 1 &&
              EVP_DigestFinal_ex(context, digest.data(), &digest_len) == 1;

    EVP_MD_CTX_free(context);
    if(!ok)
    {
        return {};
    }

    return bytes_to_hex(digest.data(), digest_len);
}

std::string generate_salt_hex()
{
    std::array<unsigned char, 16> salt_bytes{};
    std::random_device device;
    for(auto& value : salt_bytes)
    {
        value = static_cast<unsigned char>(device());
    }
    return bytes_to_hex(salt_bytes.data(), salt_bytes.size());
}

std::string make_password_hash(const std::string& password, const std::string& salt, int iterations)
{
    std::array<unsigned char, 32> digest{};
    const int rounds = std::max(1, iterations);
    const int rc = PKCS5_PBKDF2_HMAC(password.c_str(),
                                     static_cast<int>(password.size()),
                                     reinterpret_cast<const unsigned char*>(salt.data()),
                                     static_cast<int>(salt.size()),
                                     rounds,
                                     EVP_sha256(),
                                     static_cast<int>(digest.size()),
                                     digest.data());
    if(rc != 1)
    {
        return {};
    }
    return bytes_to_hex(digest.data(), digest.size());
}

std::string make_password_hash_legacy(const std::string& password, const std::string& salt)
{
    return sha256_hex(salt + ":" + password);
}

bool verify_password_value(const std::string& password, AccountRecord* record, int iterations)
{
    if(record == nullptr)
    {
        return false;
    }

    const int normalized_iterations = std::max(1, iterations);
    const auto expected_hash = make_password_hash(password, record->salt, normalized_iterations);
    if(expected_hash.empty())
    {
        return false;
    }

    if(record->password_hash == expected_hash)
    {
        return true;
    }

    if(normalized_iterations != 20000)
    {
        const auto fast_hash = make_password_hash(password, record->salt, 20000);
        if(record->password_hash == fast_hash)
        {
            record->password_hash = expected_hash;
            return true;
        }
    }

    if(normalized_iterations != kDefaultPasswordHashIterations)
    {
        const auto default_hash = make_password_hash(password, record->salt, kDefaultPasswordHashIterations);
        if(record->password_hash == default_hash)
        {
            record->password_hash = expected_hash;
            return true;
        }
    }

    const auto expected_legacy_hash = make_password_hash_legacy(password, record->salt);
    if(record->password_hash == expected_legacy_hash)
    {
        record->password_hash = expected_hash;
        return true;
    }

    if(record->salt.empty() && record->password_hash == password)
    {
        record->password_hash = expected_hash;
        return true;
    }

    return false;
}

} // namespace

MySqlAccountCoroManager::MySqlAccountCoroManager(int worker_count)
    : CoroManager(worker_count)
{
    CoroManager::init();
}

MySqlAccountCoroManager::~MySqlAccountCoroManager() = default;

CoroResult* MySqlAccountCoroManager::alloc()
{
    expand<AccountRepositoryOpResult>();
    return inner_alloc();
}

MySqlAccountRepository::MySqlAccountRepository(const MySqlConfig& config)
    : m_config(config)
{
    m_manager = std::make_unique<MySqlAccountCoroManager>(std::max(1, m_config.coro_workers));

    std::lock_guard lock(m_mutex);
    m_ready = ensure_connected() && ensure_table();
}

int MySqlAccountRepository::password_hash_iterations() const
{
    return std::max(1, m_config.password_hash_iterations);
}

MYSQL* MySqlAccountRepository::ensure_worker_connection(std::string* error)
{
    thread_local MYSQL* worker_mysql = nullptr;
    if(ensure_connected(&worker_mysql, error))
    {
        return worker_mysql;
    }
    return nullptr;
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

void MySqlAccountRepository::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

AccountRepositoryOpResult* MySqlAccountRepository::alloc_result()
{
    if(m_manager == nullptr)
    {
        return nullptr;
    }

    auto* result = dynamic_cast<AccountRepositoryOpResult*>(m_manager->alloc());
    return result;
}

CoroAwaitable MySqlAccountRepository::find_account(const std::string& account)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountRepositoryOpType::find_account,
                 account,
                 "",
                 [this](AccountRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable MySqlAccountRepository::verify_password(const std::string& account, const std::string& password)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountRepositoryOpType::verify_password,
                 account,
                 password,
                 [this](AccountRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable MySqlAccountRepository::create_account(const std::string& account, const std::string& password)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountRepositoryOpType::create_account,
                 account,
                 password,
                 [this](AccountRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

void MySqlAccountRepository::execute_operation(AccountRepositoryOpResult* result)
{
    if(result == nullptr)
    {
        return;
    }

    const std::string request_account = result->request_account;
    const std::string request_password = result->request_password;

    std::optional<AccountRecord> record;
    std::string error;

    MYSQL* worker_mysql = ensure_worker_connection(&error);
    if(worker_mysql == nullptr)
    {
        result->success = false;
        result->error = error.empty() ? "mysql_unavailable" : error;
        return;
    }

    switch(result->op_type)
    {
    case AccountRepositoryOpType::find_account:
    {
        const bool ok = query_account_record(worker_mysql, request_account, &record, &error);
        result->success = ok;
        result->record = record;
        if(!ok)
        {
            result->error = error.empty() ? "mysql_find_failed" : error;
        }
        break;
    }
    case AccountRepositoryOpType::verify_password:
    {
        const bool ok = query_account_record(worker_mysql, request_account, &record, &error);

        result->success = ok;
        if(!ok)
        {
            result->error = error.empty() ? "mysql_verify_query_failed" : error;
            break;
        }

        result->password_ok = false;
        if(record.has_value())
        {
            result->password_ok = verify_password_value(request_password,
                                                        &(*record),
                                                        password_hash_iterations());
        }
        result->record = record;
        break;
    }
    case AccountRepositoryOpType::create_account:
    {
        if(request_account.empty())
        {
            result->success = true;
            result->create_ok = false;
            break;
        }

        const std::string salt = generate_salt_hex();
        if(salt.empty())
        {
            result->success = false;
            result->error = "mysql_salt_generate_failed";
            break;
        }

        const std::string password_hash = make_password_hash(request_password,
                                                             salt,
                                                             password_hash_iterations());
        if(password_hash.empty())
        {
            result->success = false;
            result->error = "mysql_password_hash_failed";
            break;
        }

        result->create_ok = insert_account_record(worker_mysql, request_account, password_hash, salt, &error);
        result->success = error.empty();
        if(!error.empty())
        {
            result->error = error;
        }
        if(result->create_ok)
        {
            AccountRecord created_record;
            created_record.account = request_account;
            created_record.password_hash = password_hash;
            created_record.salt = salt;
            result->record = created_record;
        }
        break;
    }
    default:
        result->success = false;
        result->error = "mysql_unknown_operation";
        break;
    }
}

bool MySqlAccountRepository::ensure_connected(MYSQL** mysql_handle, std::string* error)
{
    if(mysql_handle == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_invalid_handle";
        }
        return false;
    }

    if(*mysql_handle != nullptr)
    {
        return true;
    }

    *mysql_handle = mysql_init(nullptr);
    if(*mysql_handle == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_init_failed";
        }
        spdlog::error("mysql_init failed");
        return false;
    }

    const unsigned int timeout_sec = static_cast<unsigned int>(std::max(1, m_config.connect_timeout_ms / 1000));
    mysql_options(*mysql_handle, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec);

    const my_bool ssl_verify_server_cert = 0;
    const my_bool ssl_enforce = 0;
    mysql_options(*mysql_handle, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &ssl_verify_server_cert);
    mysql_options(*mysql_handle, MYSQL_OPT_SSL_ENFORCE, &ssl_enforce);

    if(mysql_real_connect(*mysql_handle,
                          m_config.host.c_str(),
                          m_config.user.c_str(),
                          m_config.password.c_str(),
                          m_config.database.c_str(),
                          m_config.port,
                          nullptr,
                          0) == nullptr)
    {
        const char* mysql_error_text = mysql_error(*mysql_handle);
        if(error != nullptr)
        {
            *error = mysql_error_text == nullptr ? "mysql_connect_failed" : mysql_error_text;
        }
        spdlog::error("mysql_real_connect failed host={} port={} user={} db={} err={}",
                      m_config.host,
                      m_config.port,
                      m_config.user,
                      m_config.database,
                      mysql_error_text == nullptr ? "unknown" : mysql_error_text);
        mysql_close(*mysql_handle);
        *mysql_handle = nullptr;
        return false;
    }

    return true;
}

bool MySqlAccountRepository::ensure_connected()
{
    return ensure_connected(&m_mysql, nullptr);
}

bool MySqlAccountRepository::ensure_table()
{
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

bool MySqlAccountRepository::query_account_record(MYSQL* mysql_handle,
                                                  const std::string& account,
                                                  std::optional<AccountRecord>* out_record,
                                                  std::string* error)
{
    if(mysql_handle == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_unavailable";
        }
        return false;
    }

    if(out_record == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_invalid_output";
        }
        return false;
    }

    out_record->reset();

    MYSQL_STMT* stmt = mysql_stmt_init(mysql_handle);
    if(stmt == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_stmt_init_failed";
        }
        return false;
    }

    bool ok = false;
    do
    {
        if(mysql_stmt_prepare(stmt, kFindAccountSql, static_cast<unsigned long>(std::strlen(kFindAccountSql))) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        MYSQL_BIND bind_param[1]{};
        unsigned long account_len = static_cast<unsigned long>(account.size());
        bind_param[0].buffer_type = MYSQL_TYPE_STRING;
        bind_param[0].buffer = const_cast<char*>(account.data());
        bind_param[0].buffer_length = account_len;
        bind_param[0].length = &account_len;

        if(mysql_stmt_bind_param(stmt, bind_param) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        if(mysql_stmt_execute(stmt) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        std::array<char, 129> account_buf{};
        std::array<char, 257> password_buf{};
        std::array<char, 129> salt_buf{};
        unsigned long account_out_len = 0;
        unsigned long password_out_len = 0;
        unsigned long salt_out_len = 0;
        my_bool account_is_null = 0;
        my_bool password_is_null = 0;
        my_bool salt_is_null = 0;

        MYSQL_BIND bind_result[3]{};
        bind_result[0].buffer_type = MYSQL_TYPE_STRING;
        bind_result[0].buffer = account_buf.data();
        bind_result[0].buffer_length = static_cast<unsigned long>(account_buf.size());
        bind_result[0].is_null = &account_is_null;
        bind_result[0].length = &account_out_len;

        bind_result[1].buffer_type = MYSQL_TYPE_STRING;
        bind_result[1].buffer = password_buf.data();
        bind_result[1].buffer_length = static_cast<unsigned long>(password_buf.size());
        bind_result[1].is_null = &password_is_null;
        bind_result[1].length = &password_out_len;

        bind_result[2].buffer_type = MYSQL_TYPE_STRING;
        bind_result[2].buffer = salt_buf.data();
        bind_result[2].buffer_length = static_cast<unsigned long>(salt_buf.size());
        bind_result[2].is_null = &salt_is_null;
        bind_result[2].length = &salt_out_len;

        if(mysql_stmt_bind_result(stmt, bind_result) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        const int fetch_rc = mysql_stmt_fetch(stmt);
        if(fetch_rc == MYSQL_NO_DATA)
        {
            ok = true;
            break;
        }
        if(fetch_rc != 0 && fetch_rc != MYSQL_DATA_TRUNCATED)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        AccountRecord record;
        record.account.assign(account_buf.data(), std::min<unsigned long>(account_out_len, static_cast<unsigned long>(account_buf.size() - 1)));
        record.password_hash.assign(password_buf.data(), std::min<unsigned long>(password_out_len, static_cast<unsigned long>(password_buf.size() - 1)));
        record.salt.assign(salt_buf.data(), std::min<unsigned long>(salt_out_len, static_cast<unsigned long>(salt_buf.size() - 1)));
        *out_record = record;
        ok = true;
    } while(false);

    mysql_stmt_close(stmt);
    return ok;
}

bool MySqlAccountRepository::insert_account_record(MYSQL* mysql_handle,
                                                   const std::string& account,
                                                   const std::string& password_hash,
                                                   const std::string& salt,
                                                   std::string* error)
{
    if(mysql_handle == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_unavailable";
        }
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(mysql_handle);
    if(stmt == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_stmt_init_failed";
        }
        return false;
    }

    bool created = false;
    do
    {
        if(mysql_stmt_prepare(stmt, kInsertAccountSql, static_cast<unsigned long>(std::strlen(kInsertAccountSql))) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        if(password_hash.empty() || salt.empty())
        {
            if(error != nullptr)
            {
                *error = "mysql_invalid_hash_or_salt";
            }
            break;
        }

        unsigned long account_len = static_cast<unsigned long>(account.size());
        unsigned long password_len = static_cast<unsigned long>(password_hash.size());
        unsigned long salt_len = static_cast<unsigned long>(salt.size());

        MYSQL_BIND bind_param[3]{};
        bind_param[0].buffer_type = MYSQL_TYPE_STRING;
        bind_param[0].buffer = const_cast<char*>(account.data());
        bind_param[0].buffer_length = account_len;
        bind_param[0].length = &account_len;

        bind_param[1].buffer_type = MYSQL_TYPE_STRING;
        bind_param[1].buffer = const_cast<char*>(password_hash.data());
        bind_param[1].buffer_length = password_len;
        bind_param[1].length = &password_len;

        bind_param[2].buffer_type = MYSQL_TYPE_STRING;
        bind_param[2].buffer = const_cast<char*>(salt.data());
        bind_param[2].buffer_length = salt_len;
        bind_param[2].length = &salt_len;

        if(mysql_stmt_bind_param(stmt, bind_param) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        if(mysql_stmt_execute(stmt) != 0)
        {
            if(mysql_stmt_errno(stmt) == 1062)
            {
                created = false;
                break;
            }

            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        created = true;
    } while(false);

    mysql_stmt_close(stmt);
    return created;
}

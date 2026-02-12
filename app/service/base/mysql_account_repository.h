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

#include "account_repository.h"
#include "application_config.h"

#include <mariadb/mysql.h>

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

class MySqlAccountCoroManager : public CoroManager
{
public:
    explicit MySqlAccountCoroManager(int worker_count);
    ~MySqlAccountCoroManager() override;

public:
    CoroResult* alloc() override;
};

class MySqlAccountRepository : public IAccountRepository
{
public:
    explicit MySqlAccountRepository(const MySqlConfig& config);
    ~MySqlAccountRepository() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable find_account(const std::string& account) override;
    CoroAwaitable verify_password(const std::string& account, const std::string& password) override;
    CoroAwaitable create_account(const std::string& account, const std::string& password) override;

private:
    struct VerifiedPasswordCacheEntry
    {
        std::string password_digest;
        std::string password_hash;
        std::string salt;
    };

    int password_hash_iterations() const;
    AccountRepositoryOpResult* alloc_result();
    void execute_operation(AccountRepositoryOpResult* result);
    bool get_cached_account(const std::string& account, AccountRecord* out_record) const;
    void put_cached_account(const AccountRecord& record);
    bool is_verified_password_cached(const std::string& account,
                                     const std::string& password,
                                     const AccountRecord& record) const;
    void remember_verified_password(const std::string& account,
                                    const std::string& password,
                                    const AccountRecord& record);
    void invalidate_verified_password_cache(const std::string& account);
    bool ensure_connected(MYSQL** mysql_handle, std::string* error = nullptr);
    MYSQL* ensure_worker_connection(std::string* error);
    bool ensure_connected();
    bool ensure_table();
    bool query_account_record(MYSQL* mysql_handle,
                              const std::string& account,
                              std::optional<AccountRecord>* out_record,
                              std::string* error);
    bool insert_account_record(MYSQL* mysql_handle,
                               const std::string& account,
                               const std::string& password_hash,
                               const std::string& salt,
                               std::string* error);

private:
    MySqlConfig m_config;
    MYSQL* m_mysql = nullptr;
    bool m_ready = false;
    std::unique_ptr<MySqlAccountCoroManager> m_manager;
    mutable std::mutex m_mutex;
    mutable std::shared_mutex m_cache_mutex;
    std::unordered_map<std::string, AccountRecord> m_account_cache;
    std::unordered_map<std::string, VerifiedPasswordCacheEntry> m_verified_password_cache;
};

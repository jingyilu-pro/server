//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "account_repository.h"
#include "application_config.h"

#include <mariadb/mysql.h>

#include <memory>
#include <mutex>

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
    int password_hash_iterations() const;
    AccountRepositoryOpResult* alloc_result();
    void execute_operation(AccountRepositoryOpResult* result);
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
};

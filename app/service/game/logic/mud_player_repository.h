//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "coromanager.h"
#include "mud_types.h"

#include <mariadb/mysql.h>

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

enum class MudPlayerRepositoryOpType
{
    load_player,
    create_player,
    save_player,
    list_top_players,
    list_team_members,
};

class MudPlayerRepositoryOpResult : public CoroResult
{
public:
    MudPlayerRepositoryOpResult() = default;
    ~MudPlayerRepositoryOpResult() override = default;

    void init(MudPlayerRepositoryOpType op,
              std::string account,
              std::string group_id,
              std::optional<MudPlayerState> player_state,
              MudLeaderboardType leaderboard_type,
              int limit,
              std::function<void(MudPlayerRepositoryOpResult*)> worker_fn)
    {
        op_type = op;
        request_account = std::move(account);
        request_group_id = std::move(group_id);
        request_player = std::move(player_state);
        request_leaderboard_type = leaderboard_type;
        request_limit = limit;
        success = false;
        found = false;
        create_ok = false;
        save_ok = false;
        error.clear();
        player.reset();
        players.clear();
        m_worker_fn = std::move(worker_fn);
    }

    void worker() override
    {
        if(m_worker_fn)
        {
            m_worker_fn(this);
            return;
        }

        success = false;
        error = "missing mud player repository worker";
    }

    void clear() override
    {
        request_account.clear();
        request_group_id.clear();
        request_player.reset();
        success = false;
        found = false;
        create_ok = false;
        save_ok = false;
        request_leaderboard_type = MudLeaderboardType::realm;
        request_limit = 0;
        error.clear();
        player.reset();
        players.clear();
        m_worker_fn = nullptr;
    }

public:
    MudPlayerRepositoryOpType op_type = MudPlayerRepositoryOpType::load_player;
    std::string request_account;
    std::string request_group_id;
    std::optional<MudPlayerState> request_player;
    MudLeaderboardType request_leaderboard_type = MudLeaderboardType::realm;
    int request_limit = 0;
    bool success = false;
    bool found = false;
    bool create_ok = false;
    bool save_ok = false;
    std::string error;
    std::optional<MudPlayerState> player;
    std::vector<MudLeaderboardEntry> players;

private:
    std::function<void(MudPlayerRepositoryOpResult*)> m_worker_fn;
};

class IMudPlayerRepository
{
public:
    virtual ~IMudPlayerRepository() = default;

public:
    virtual bool ready() const = 0;
    virtual void poll() = 0;
    virtual CoroAwaitable load_player(const std::string& account) = 0;
    virtual CoroAwaitable create_player(const MudPlayerState& player) = 0;
    virtual CoroAwaitable save_player(const MudPlayerState& player) = 0;
    virtual CoroAwaitable list_top_players(MudLeaderboardType leaderboard_type, int limit) = 0;
    virtual CoroAwaitable list_team_members(const std::string& team_id) = 0;
};

bool wait_mud_player_repository_result(IMudPlayerRepository* repository,
                                       CoroAwaitable awaitable,
                                       MudPlayerRepositoryOpResult* out_result,
                                       int timeout_ms);

class MySqlMudPlayerRepository : public IMudPlayerRepository
{
public:
    explicit MySqlMudPlayerRepository(const MySqlConfig& config);
    ~MySqlMudPlayerRepository() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable load_player(const std::string& account) override;
    CoroAwaitable create_player(const MudPlayerState& player) override;
    CoroAwaitable save_player(const MudPlayerState& player) override;
    CoroAwaitable list_top_players(MudLeaderboardType leaderboard_type, int limit) override;
    CoroAwaitable list_team_members(const std::string& team_id) override;

private:
    class MySqlMudPlayerCoroManager;

private:
    MudPlayerRepositoryOpResult* alloc_result();
    void execute_operation(MudPlayerRepositoryOpResult* result);
    bool ensure_connected(MYSQL** mysql_handle, std::string* error = nullptr);
    MYSQL* ensure_worker_connection(std::string* error);
    bool ensure_connected();
    bool ensure_table();
    bool query_player_record(MYSQL* mysql_handle,
                             const std::string& account,
                             std::optional<MudPlayerState>* out_player,
                             std::string* error);
    bool insert_player_record(MYSQL* mysql_handle,
                              const MudPlayerState& player,
                              std::string* error);
    bool update_player_record(MYSQL* mysql_handle,
                              const MudPlayerState& player,
                              std::string* error);
    bool query_top_players(MYSQL* mysql_handle,
                           MudLeaderboardType leaderboard_type,
                           int limit,
                           std::vector<MudLeaderboardEntry>* out_players,
                           std::string* error);
    bool query_team_members(MYSQL* mysql_handle,
                            const std::string& team_id,
                            std::vector<MudPlayerState>* out_players,
                            std::string* error);

private:
    MySqlConfig m_config;
    MYSQL* m_mysql = nullptr;
    bool m_ready = false;
    std::unique_ptr<MySqlMudPlayerCoroManager> m_manager;
    mutable std::mutex m_mutex;
};

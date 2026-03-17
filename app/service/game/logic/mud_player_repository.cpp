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

#include "mud_player_repository.h"

#include "log/glogger.h"

#include <jansson.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace
{

constexpr const char* kMudCharacterTableSql =
    "CREATE TABLE IF NOT EXISTS mud_character ("
    "account VARCHAR(128) NOT NULL PRIMARY KEY,"
    "character_name VARCHAR(64) NOT NULL,"
    "level INT NOT NULL DEFAULT 1,"
    "hp INT NOT NULL DEFAULT 100,"
    "max_hp INT NOT NULL DEFAULT 100,"
    "attack_power INT NOT NULL DEFAULT 18,"
    "defense_power INT NOT NULL DEFAULT 10,"
    "spirit_stone BIGINT NOT NULL DEFAULT 0,"
    "title VARCHAR(128) NOT NULL DEFAULT '',"
    "location_scene_id VARCHAR(64) NOT NULL,"
    "realm_name VARCHAR(64) NOT NULL DEFAULT '',"
    "realm_stage INT NOT NULL DEFAULT 0,"
    "exp BIGINT NOT NULL DEFAULT 0,"
    "next_breakthrough_exp BIGINT NOT NULL DEFAULT 120,"
    "primary_skill VARCHAR(64) NOT NULL DEFAULT '',"
    "skill_level INT NOT NULL DEFAULT 1,"
    "sect_id VARCHAR(64) NOT NULL DEFAULT '',"
    "sect_name VARCHAR(64) NOT NULL DEFAULT '',"
    "sect_rank VARCHAR(64) NOT NULL DEFAULT '',"
    "inventory_json LONGTEXT NOT NULL,"
    "quest_json LONGTEXT NOT NULL,"
    "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
    "updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

constexpr const char* kLoadPlayerSql =
    "SELECT account,character_name,level,hp,max_hp,attack_power,defense_power,spirit_stone,"
    "title,location_scene_id,realm_name,realm_stage,exp,next_breakthrough_exp,primary_skill,"
    "skill_level,sect_id,sect_name,sect_rank,inventory_json,quest_json "
    "FROM mud_character WHERE account=? LIMIT 1";

constexpr const char* kInsertPlayerSql =
    "INSERT INTO mud_character(account,character_name,level,hp,max_hp,attack_power,defense_power,"
    "spirit_stone,title,location_scene_id,realm_name,realm_stage,exp,next_breakthrough_exp,"
    "primary_skill,skill_level,sect_id,sect_name,sect_rank,inventory_json,quest_json)"
    " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

constexpr const char* kUpdatePlayerSql =
    "UPDATE mud_character SET character_name=?,level=?,hp=?,max_hp=?,attack_power=?,defense_power=?,"
    "spirit_stone=?,title=?,location_scene_id=?,realm_name=?,realm_stage=?,exp=?,"
    "next_breakthrough_exp=?,primary_skill=?,skill_level=?,sect_id=?,sect_name=?,sect_rank=?,"
    "inventory_json=?,quest_json=? WHERE account=?";

std::string encode_inventory_json(const std::vector<MudInventoryItemState>& inventory)
{
    json_t* root = json_array();
    if(root == nullptr)
    {
        return "[]";
    }

    for(const auto& item : inventory)
    {
        json_t* node = json_object();
        if(node == nullptr)
        {
            continue;
        }
        json_object_set_new(node, "item_id", json_string(item.item_id.c_str()));
        json_object_set_new(node, "quantity", json_integer(item.quantity));
        json_object_set_new(node, "equipped", item.equipped ? json_true() : json_false());
        json_array_append_new(root, node);
    }

    char* dumped = json_dumps(root, JSON_COMPACT);
    std::string output = dumped == nullptr ? "[]" : dumped;
    if(dumped != nullptr)
    {
        free(dumped);
    }
    json_decref(root);
    return output;
}

std::string encode_quest_json(const std::vector<MudQuestState>& quests)
{
    json_t* root = json_array();
    if(root == nullptr)
    {
        return "[]";
    }

    for(const auto& quest : quests)
    {
        json_t* node = json_object();
        if(node == nullptr)
        {
            continue;
        }
        json_object_set_new(node, "quest_id", json_string(quest.quest_id.c_str()));
        json_object_set_new(node, "status", json_string(quest.status.c_str()));
        json_object_set_new(node, "progress", json_integer(quest.progress));
        json_array_append_new(root, node);
    }

    char* dumped = json_dumps(root, JSON_COMPACT);
    std::string output = dumped == nullptr ? "[]" : dumped;
    if(dumped != nullptr)
    {
        free(dumped);
    }
    json_decref(root);
    return output;
}

void decode_inventory_json(const std::string& json_text, std::vector<MudInventoryItemState>* inventory)
{
    if(inventory == nullptr)
    {
        return;
    }
    inventory->clear();
    if(json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_array(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    const size_t count = json_array_size(root);
    inventory->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(root, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudInventoryItemState state;
        if(auto* value = json_object_get(item, "item_id"); value != nullptr && json_is_string(value))
        {
            const char* text = json_string_value(value);
            state.item_id = text == nullptr ? "" : text;
        }
        if(auto* value = json_object_get(item, "quantity"); value != nullptr && json_is_integer(value))
        {
            state.quantity = static_cast<int>(json_integer_value(value));
        }
        if(auto* value = json_object_get(item, "equipped"); value != nullptr && json_is_boolean(value))
        {
            state.equipped = json_is_true(value);
        }
        if(!state.item_id.empty() && state.quantity > 0)
        {
            inventory->push_back(std::move(state));
        }
    }

    json_decref(root);
}

void decode_quest_json(const std::string& json_text, std::vector<MudQuestState>* quests)
{
    if(quests == nullptr)
    {
        return;
    }
    quests->clear();
    if(json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_array(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    const size_t count = json_array_size(root);
    quests->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(root, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudQuestState state;
        if(auto* value = json_object_get(item, "quest_id"); value != nullptr && json_is_string(value))
        {
            const char* text = json_string_value(value);
            state.quest_id = text == nullptr ? "" : text;
        }
        if(auto* value = json_object_get(item, "status"); value != nullptr && json_is_string(value))
        {
            const char* text = json_string_value(value);
            state.status = text == nullptr ? "" : text;
        }
        if(auto* value = json_object_get(item, "progress"); value != nullptr && json_is_integer(value))
        {
            state.progress = static_cast<int>(json_integer_value(value));
        }
        if(!state.quest_id.empty())
        {
            quests->push_back(std::move(state));
        }
    }

    json_decref(root);
}

class MySqlMudPlayerCoroManager : public CoroManager
{
public:
    explicit MySqlMudPlayerCoroManager(int worker_count)
        : CoroManager(worker_count)
    {
        CoroManager::init();
    }

    ~MySqlMudPlayerCoroManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<MudPlayerRepositoryOpResult>();
        return inner_alloc();
    }
};

} // namespace

class MySqlMudPlayerRepository::MySqlMudPlayerCoroManager : public ::MySqlMudPlayerCoroManager
{
public:
    explicit MySqlMudPlayerCoroManager(int worker_count)
        : ::MySqlMudPlayerCoroManager(worker_count)
    {
    }
};

MySqlMudPlayerRepository::MySqlMudPlayerRepository(const MySqlConfig& config)
    : m_config(config)
{
    m_manager = std::make_unique<MySqlMudPlayerCoroManager>(std::max(1, m_config.coro_workers));

    std::lock_guard lock(m_mutex);
    m_ready = ensure_connected() && ensure_table();
}

MySqlMudPlayerRepository::~MySqlMudPlayerRepository()
{
    std::lock_guard lock(m_mutex);
    if(m_mysql != nullptr)
    {
        mysql_close(m_mysql);
        m_mysql = nullptr;
    }
}

bool MySqlMudPlayerRepository::ready() const
{
    return m_ready;
}

void MySqlMudPlayerRepository::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

MudPlayerRepositoryOpResult* MySqlMudPlayerRepository::alloc_result()
{
    if(m_manager == nullptr)
    {
        return nullptr;
    }

    return dynamic_cast<MudPlayerRepositoryOpResult*>(m_manager->alloc());
}

CoroAwaitable MySqlMudPlayerRepository::load_player(const std::string& account)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudPlayerRepositoryOpType::load_player,
                 account,
                 std::nullopt,
                 MudLeaderboardType::realm,
                 0,
                 [this](MudPlayerRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable MySqlMudPlayerRepository::create_player(const MudPlayerState& player)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudPlayerRepositoryOpType::create_player,
                 player.account,
                 player,
                 MudLeaderboardType::realm,
                 0,
                 [this](MudPlayerRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable MySqlMudPlayerRepository::save_player(const MudPlayerState& player)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudPlayerRepositoryOpType::save_player,
                 player.account,
                 player,
                 MudLeaderboardType::realm,
                 0,
                 [this](MudPlayerRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable MySqlMudPlayerRepository::list_top_players(MudLeaderboardType leaderboard_type, int limit)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudPlayerRepositoryOpType::list_top_players,
                 "",
                 std::nullopt,
                 leaderboard_type,
                 limit,
                 [this](MudPlayerRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

void MySqlMudPlayerRepository::execute_operation(MudPlayerRepositoryOpResult* result)
{
    if(result == nullptr)
    {
        return;
    }

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
    case MudPlayerRepositoryOpType::load_player:
    {
        std::optional<MudPlayerState> player;
        const bool ok = query_player_record(worker_mysql,
                                            result->request_account,
                                            &player,
                                            &error);
        result->success = ok;
        result->found = player.has_value();
        result->player = player;
        if(!ok)
        {
            result->error = error.empty() ? "mysql_load_player_failed" : error;
        }
        break;
    }
    case MudPlayerRepositoryOpType::create_player:
    {
        if(!result->request_player.has_value())
        {
            result->success = false;
            result->error = "mysql_create_player_missing_state";
            break;
        }

        result->create_ok = insert_player_record(worker_mysql, *result->request_player, &error);
        result->success = error.empty();
        result->found = result->create_ok;
        if(result->create_ok)
        {
            result->player = result->request_player;
        }
        if(!error.empty())
        {
            result->error = error;
        }
        break;
    }
    case MudPlayerRepositoryOpType::save_player:
    {
        if(!result->request_player.has_value())
        {
            result->success = false;
            result->error = "mysql_save_player_missing_state";
            break;
        }

        result->save_ok = update_player_record(worker_mysql, *result->request_player, &error);
        result->success = error.empty();
        result->found = result->save_ok;
        if(result->save_ok)
        {
            result->player = result->request_player;
        }
        if(!error.empty())
        {
            result->error = error;
        }
        break;
    }
    case MudPlayerRepositoryOpType::list_top_players:
    {
        std::vector<MudLeaderboardEntry> players;
        const bool ok = query_top_players(worker_mysql,
                                          result->request_leaderboard_type,
                                          result->request_limit,
                                          &players,
                                          &error);
        result->success = ok;
        result->players = std::move(players);
        if(!ok)
        {
            result->error = error.empty() ? "mysql_list_top_players_failed" : error;
        }
        break;
    }
    default:
        result->success = false;
        result->error = "mysql_unknown_operation";
        break;
    }
}

bool MySqlMudPlayerRepository::ensure_connected(MYSQL** mysql_handle, std::string* error)
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
        spdlog::error("mysql mud connect failed host={} port={} user={} db={} err={}",
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

MYSQL* MySqlMudPlayerRepository::ensure_worker_connection(std::string* error)
{
    thread_local MYSQL* worker_mysql = nullptr;
    if(ensure_connected(&worker_mysql, error))
    {
        return worker_mysql;
    }
    return nullptr;
}

bool MySqlMudPlayerRepository::ensure_connected()
{
    return ensure_connected(&m_mysql, nullptr);
}

bool MySqlMudPlayerRepository::ensure_table()
{
    if(!ensure_connected())
    {
        return false;
    }

    if(mysql_query(m_mysql, kMudCharacterTableSql) != 0)
    {
        spdlog::error("mysql create mud_character failed: {}", mysql_error(m_mysql));
        return false;
    }
    return true;
}

bool MySqlMudPlayerRepository::query_player_record(MYSQL* mysql_handle,
                                                   const std::string& account,
                                                   std::optional<MudPlayerState>* out_player,
                                                   std::string* error)
{
    if(mysql_handle == nullptr || out_player == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_invalid_query_player_args";
        }
        return false;
    }

    out_player->reset();

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
        if(mysql_stmt_prepare(stmt, kLoadPlayerSql, static_cast<unsigned long>(std::strlen(kLoadPlayerSql))) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        unsigned long account_len = static_cast<unsigned long>(account.size());
        MYSQL_BIND bind_param[1]{};
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
        std::array<char, 65> character_name_buf{};
        std::array<char, 129> title_buf{};
        std::array<char, 65> location_scene_id_buf{};
        std::array<char, 65> realm_name_buf{};
        std::array<char, 65> primary_skill_buf{};
        std::array<char, 65> sect_id_buf{};
        std::array<char, 65> sect_name_buf{};
        std::array<char, 65> sect_rank_buf{};
        std::array<char, 32768> inventory_json_buf{};
        std::array<char, 32768> quest_json_buf{};

        unsigned long account_out_len = 0;
        unsigned long character_name_out_len = 0;
        unsigned long title_out_len = 0;
        unsigned long location_scene_id_out_len = 0;
        unsigned long realm_name_out_len = 0;
        unsigned long primary_skill_out_len = 0;
        unsigned long sect_id_out_len = 0;
        unsigned long sect_name_out_len = 0;
        unsigned long sect_rank_out_len = 0;
        unsigned long inventory_json_out_len = 0;
        unsigned long quest_json_out_len = 0;

        int level = 0;
        int hp = 0;
        int max_hp = 0;
        int attack_power = 0;
        int defense_power = 0;
        int realm_stage = 0;
        int skill_level = 0;
        long long spirit_stone = 0;
        long long exp = 0;
        long long next_breakthrough_exp = 0;

        MYSQL_BIND bind_result[21]{};
        bind_result[0].buffer_type = MYSQL_TYPE_STRING;
        bind_result[0].buffer = account_buf.data();
        bind_result[0].buffer_length = static_cast<unsigned long>(account_buf.size());
        bind_result[0].length = &account_out_len;

        bind_result[1].buffer_type = MYSQL_TYPE_STRING;
        bind_result[1].buffer = character_name_buf.data();
        bind_result[1].buffer_length = static_cast<unsigned long>(character_name_buf.size());
        bind_result[1].length = &character_name_out_len;

        bind_result[2].buffer_type = MYSQL_TYPE_LONG;
        bind_result[2].buffer = &level;
        bind_result[3].buffer_type = MYSQL_TYPE_LONG;
        bind_result[3].buffer = &hp;
        bind_result[4].buffer_type = MYSQL_TYPE_LONG;
        bind_result[4].buffer = &max_hp;
        bind_result[5].buffer_type = MYSQL_TYPE_LONG;
        bind_result[5].buffer = &attack_power;
        bind_result[6].buffer_type = MYSQL_TYPE_LONG;
        bind_result[6].buffer = &defense_power;
        bind_result[7].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_result[7].buffer = &spirit_stone;

        bind_result[8].buffer_type = MYSQL_TYPE_STRING;
        bind_result[8].buffer = title_buf.data();
        bind_result[8].buffer_length = static_cast<unsigned long>(title_buf.size());
        bind_result[8].length = &title_out_len;

        bind_result[9].buffer_type = MYSQL_TYPE_STRING;
        bind_result[9].buffer = location_scene_id_buf.data();
        bind_result[9].buffer_length = static_cast<unsigned long>(location_scene_id_buf.size());
        bind_result[9].length = &location_scene_id_out_len;

        bind_result[10].buffer_type = MYSQL_TYPE_STRING;
        bind_result[10].buffer = realm_name_buf.data();
        bind_result[10].buffer_length = static_cast<unsigned long>(realm_name_buf.size());
        bind_result[10].length = &realm_name_out_len;

        bind_result[11].buffer_type = MYSQL_TYPE_LONG;
        bind_result[11].buffer = &realm_stage;
        bind_result[12].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_result[12].buffer = &exp;
        bind_result[13].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_result[13].buffer = &next_breakthrough_exp;

        bind_result[14].buffer_type = MYSQL_TYPE_STRING;
        bind_result[14].buffer = primary_skill_buf.data();
        bind_result[14].buffer_length = static_cast<unsigned long>(primary_skill_buf.size());
        bind_result[14].length = &primary_skill_out_len;

        bind_result[15].buffer_type = MYSQL_TYPE_LONG;
        bind_result[15].buffer = &skill_level;

        bind_result[16].buffer_type = MYSQL_TYPE_STRING;
        bind_result[16].buffer = sect_id_buf.data();
        bind_result[16].buffer_length = static_cast<unsigned long>(sect_id_buf.size());
        bind_result[16].length = &sect_id_out_len;

        bind_result[17].buffer_type = MYSQL_TYPE_STRING;
        bind_result[17].buffer = sect_name_buf.data();
        bind_result[17].buffer_length = static_cast<unsigned long>(sect_name_buf.size());
        bind_result[17].length = &sect_name_out_len;

        bind_result[18].buffer_type = MYSQL_TYPE_STRING;
        bind_result[18].buffer = sect_rank_buf.data();
        bind_result[18].buffer_length = static_cast<unsigned long>(sect_rank_buf.size());
        bind_result[18].length = &sect_rank_out_len;

        bind_result[19].buffer_type = MYSQL_TYPE_STRING;
        bind_result[19].buffer = inventory_json_buf.data();
        bind_result[19].buffer_length = static_cast<unsigned long>(inventory_json_buf.size());
        bind_result[19].length = &inventory_json_out_len;

        bind_result[20].buffer_type = MYSQL_TYPE_STRING;
        bind_result[20].buffer = quest_json_buf.data();
        bind_result[20].buffer_length = static_cast<unsigned long>(quest_json_buf.size());
        bind_result[20].length = &quest_json_out_len;

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

        MudPlayerState player;
        player.account.assign(account_buf.data(), std::min<unsigned long>(account_out_len, static_cast<unsigned long>(account_buf.size() - 1)));
        player.character_name.assign(character_name_buf.data(), std::min<unsigned long>(character_name_out_len, static_cast<unsigned long>(character_name_buf.size() - 1)));
        player.level = level;
        player.hp = hp;
        player.max_hp = max_hp;
        player.attack_power = attack_power;
        player.defense_power = defense_power;
        player.spirit_stone = spirit_stone;
        player.title.assign(title_buf.data(), std::min<unsigned long>(title_out_len, static_cast<unsigned long>(title_buf.size() - 1)));
        player.location_scene_id.assign(location_scene_id_buf.data(), std::min<unsigned long>(location_scene_id_out_len, static_cast<unsigned long>(location_scene_id_buf.size() - 1)));
        player.realm_name.assign(realm_name_buf.data(), std::min<unsigned long>(realm_name_out_len, static_cast<unsigned long>(realm_name_buf.size() - 1)));
        player.realm_stage = realm_stage;
        player.exp = exp;
        player.next_breakthrough_exp = next_breakthrough_exp;
        player.primary_skill.assign(primary_skill_buf.data(), std::min<unsigned long>(primary_skill_out_len, static_cast<unsigned long>(primary_skill_buf.size() - 1)));
        player.skill_level = skill_level;
        player.sect_id.assign(sect_id_buf.data(), std::min<unsigned long>(sect_id_out_len, static_cast<unsigned long>(sect_id_buf.size() - 1)));
        player.sect_name.assign(sect_name_buf.data(), std::min<unsigned long>(sect_name_out_len, static_cast<unsigned long>(sect_name_buf.size() - 1)));
        player.sect_rank.assign(sect_rank_buf.data(), std::min<unsigned long>(sect_rank_out_len, static_cast<unsigned long>(sect_rank_buf.size() - 1)));
        decode_inventory_json(std::string(inventory_json_buf.data(), std::min<unsigned long>(inventory_json_out_len, static_cast<unsigned long>(inventory_json_buf.size() - 1))),
                              &player.inventory);
        decode_quest_json(std::string(quest_json_buf.data(), std::min<unsigned long>(quest_json_out_len, static_cast<unsigned long>(quest_json_buf.size() - 1))),
                          &player.quests);
        *out_player = std::move(player);
        ok = true;
    } while(false);

    mysql_stmt_close(stmt);
    return ok;
}

bool MySqlMudPlayerRepository::insert_player_record(MYSQL* mysql_handle,
                                                    const MudPlayerState& player,
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

    const std::string inventory_json = encode_inventory_json(player.inventory);
    const std::string quest_json = encode_quest_json(player.quests);

    bool created = false;
    do
    {
        if(mysql_stmt_prepare(stmt, kInsertPlayerSql, static_cast<unsigned long>(std::strlen(kInsertPlayerSql))) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        unsigned long account_len = static_cast<unsigned long>(player.account.size());
        unsigned long character_name_len = static_cast<unsigned long>(player.character_name.size());
        unsigned long title_len = static_cast<unsigned long>(player.title.size());
        unsigned long location_scene_id_len = static_cast<unsigned long>(player.location_scene_id.size());
        unsigned long realm_name_len = static_cast<unsigned long>(player.realm_name.size());
        unsigned long primary_skill_len = static_cast<unsigned long>(player.primary_skill.size());
        unsigned long sect_id_len = static_cast<unsigned long>(player.sect_id.size());
        unsigned long sect_name_len = static_cast<unsigned long>(player.sect_name.size());
        unsigned long sect_rank_len = static_cast<unsigned long>(player.sect_rank.size());
        unsigned long inventory_json_len = static_cast<unsigned long>(inventory_json.size());
        unsigned long quest_json_len = static_cast<unsigned long>(quest_json.size());

        int level = player.level;
        int hp = player.hp;
        int max_hp = player.max_hp;
        int attack_power = player.attack_power;
        int defense_power = player.defense_power;
        long long spirit_stone = static_cast<long long>(player.spirit_stone);
        int realm_stage = player.realm_stage;
        long long exp = static_cast<long long>(player.exp);
        long long next_breakthrough_exp = static_cast<long long>(player.next_breakthrough_exp);
        int skill_level = player.skill_level;

        MYSQL_BIND bind_param[21]{};
        bind_param[0].buffer_type = MYSQL_TYPE_STRING;
        bind_param[0].buffer = const_cast<char*>(player.account.data());
        bind_param[0].buffer_length = account_len;
        bind_param[0].length = &account_len;

        bind_param[1].buffer_type = MYSQL_TYPE_STRING;
        bind_param[1].buffer = const_cast<char*>(player.character_name.data());
        bind_param[1].buffer_length = character_name_len;
        bind_param[1].length = &character_name_len;

        bind_param[2].buffer_type = MYSQL_TYPE_LONG;
        bind_param[2].buffer = &level;
        bind_param[3].buffer_type = MYSQL_TYPE_LONG;
        bind_param[3].buffer = &hp;
        bind_param[4].buffer_type = MYSQL_TYPE_LONG;
        bind_param[4].buffer = &max_hp;
        bind_param[5].buffer_type = MYSQL_TYPE_LONG;
        bind_param[5].buffer = &attack_power;
        bind_param[6].buffer_type = MYSQL_TYPE_LONG;
        bind_param[6].buffer = &defense_power;
        bind_param[7].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_param[7].buffer = &spirit_stone;

        bind_param[8].buffer_type = MYSQL_TYPE_STRING;
        bind_param[8].buffer = const_cast<char*>(player.title.data());
        bind_param[8].buffer_length = title_len;
        bind_param[8].length = &title_len;

        bind_param[9].buffer_type = MYSQL_TYPE_STRING;
        bind_param[9].buffer = const_cast<char*>(player.location_scene_id.data());
        bind_param[9].buffer_length = location_scene_id_len;
        bind_param[9].length = &location_scene_id_len;

        bind_param[10].buffer_type = MYSQL_TYPE_STRING;
        bind_param[10].buffer = const_cast<char*>(player.realm_name.data());
        bind_param[10].buffer_length = realm_name_len;
        bind_param[10].length = &realm_name_len;

        bind_param[11].buffer_type = MYSQL_TYPE_LONG;
        bind_param[11].buffer = &realm_stage;
        bind_param[12].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_param[12].buffer = &exp;
        bind_param[13].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_param[13].buffer = &next_breakthrough_exp;

        bind_param[14].buffer_type = MYSQL_TYPE_STRING;
        bind_param[14].buffer = const_cast<char*>(player.primary_skill.data());
        bind_param[14].buffer_length = primary_skill_len;
        bind_param[14].length = &primary_skill_len;

        bind_param[15].buffer_type = MYSQL_TYPE_LONG;
        bind_param[15].buffer = &skill_level;

        bind_param[16].buffer_type = MYSQL_TYPE_STRING;
        bind_param[16].buffer = const_cast<char*>(player.sect_id.data());
        bind_param[16].buffer_length = sect_id_len;
        bind_param[16].length = &sect_id_len;

        bind_param[17].buffer_type = MYSQL_TYPE_STRING;
        bind_param[17].buffer = const_cast<char*>(player.sect_name.data());
        bind_param[17].buffer_length = sect_name_len;
        bind_param[17].length = &sect_name_len;

        bind_param[18].buffer_type = MYSQL_TYPE_STRING;
        bind_param[18].buffer = const_cast<char*>(player.sect_rank.data());
        bind_param[18].buffer_length = sect_rank_len;
        bind_param[18].length = &sect_rank_len;

        bind_param[19].buffer_type = MYSQL_TYPE_STRING;
        bind_param[19].buffer = const_cast<char*>(inventory_json.data());
        bind_param[19].buffer_length = inventory_json_len;
        bind_param[19].length = &inventory_json_len;

        bind_param[20].buffer_type = MYSQL_TYPE_STRING;
        bind_param[20].buffer = const_cast<char*>(quest_json.data());
        bind_param[20].buffer_length = quest_json_len;
        bind_param[20].length = &quest_json_len;

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

bool MySqlMudPlayerRepository::update_player_record(MYSQL* mysql_handle,
                                                    const MudPlayerState& player,
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

    const std::string inventory_json = encode_inventory_json(player.inventory);
    const std::string quest_json = encode_quest_json(player.quests);

    bool updated = false;
    do
    {
        if(mysql_stmt_prepare(stmt, kUpdatePlayerSql, static_cast<unsigned long>(std::strlen(kUpdatePlayerSql))) != 0)
        {
            if(error != nullptr)
            {
                *error = mysql_stmt_error(stmt);
            }
            break;
        }

        unsigned long character_name_len = static_cast<unsigned long>(player.character_name.size());
        unsigned long title_len = static_cast<unsigned long>(player.title.size());
        unsigned long location_scene_id_len = static_cast<unsigned long>(player.location_scene_id.size());
        unsigned long realm_name_len = static_cast<unsigned long>(player.realm_name.size());
        unsigned long primary_skill_len = static_cast<unsigned long>(player.primary_skill.size());
        unsigned long sect_id_len = static_cast<unsigned long>(player.sect_id.size());
        unsigned long sect_name_len = static_cast<unsigned long>(player.sect_name.size());
        unsigned long sect_rank_len = static_cast<unsigned long>(player.sect_rank.size());
        unsigned long inventory_json_len = static_cast<unsigned long>(inventory_json.size());
        unsigned long quest_json_len = static_cast<unsigned long>(quest_json.size());
        unsigned long account_len = static_cast<unsigned long>(player.account.size());

        int level = player.level;
        int hp = player.hp;
        int max_hp = player.max_hp;
        int attack_power = player.attack_power;
        int defense_power = player.defense_power;
        long long spirit_stone = static_cast<long long>(player.spirit_stone);
        int realm_stage = player.realm_stage;
        long long exp = static_cast<long long>(player.exp);
        long long next_breakthrough_exp = static_cast<long long>(player.next_breakthrough_exp);
        int skill_level = player.skill_level;

        MYSQL_BIND bind_param[21]{};
        bind_param[0].buffer_type = MYSQL_TYPE_STRING;
        bind_param[0].buffer = const_cast<char*>(player.character_name.data());
        bind_param[0].buffer_length = character_name_len;
        bind_param[0].length = &character_name_len;

        bind_param[1].buffer_type = MYSQL_TYPE_LONG;
        bind_param[1].buffer = &level;
        bind_param[2].buffer_type = MYSQL_TYPE_LONG;
        bind_param[2].buffer = &hp;
        bind_param[3].buffer_type = MYSQL_TYPE_LONG;
        bind_param[3].buffer = &max_hp;
        bind_param[4].buffer_type = MYSQL_TYPE_LONG;
        bind_param[4].buffer = &attack_power;
        bind_param[5].buffer_type = MYSQL_TYPE_LONG;
        bind_param[5].buffer = &defense_power;
        bind_param[6].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_param[6].buffer = &spirit_stone;

        bind_param[7].buffer_type = MYSQL_TYPE_STRING;
        bind_param[7].buffer = const_cast<char*>(player.title.data());
        bind_param[7].buffer_length = title_len;
        bind_param[7].length = &title_len;

        bind_param[8].buffer_type = MYSQL_TYPE_STRING;
        bind_param[8].buffer = const_cast<char*>(player.location_scene_id.data());
        bind_param[8].buffer_length = location_scene_id_len;
        bind_param[8].length = &location_scene_id_len;

        bind_param[9].buffer_type = MYSQL_TYPE_STRING;
        bind_param[9].buffer = const_cast<char*>(player.realm_name.data());
        bind_param[9].buffer_length = realm_name_len;
        bind_param[9].length = &realm_name_len;

        bind_param[10].buffer_type = MYSQL_TYPE_LONG;
        bind_param[10].buffer = &realm_stage;
        bind_param[11].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_param[11].buffer = &exp;
        bind_param[12].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_param[12].buffer = &next_breakthrough_exp;

        bind_param[13].buffer_type = MYSQL_TYPE_STRING;
        bind_param[13].buffer = const_cast<char*>(player.primary_skill.data());
        bind_param[13].buffer_length = primary_skill_len;
        bind_param[13].length = &primary_skill_len;

        bind_param[14].buffer_type = MYSQL_TYPE_LONG;
        bind_param[14].buffer = &skill_level;

        bind_param[15].buffer_type = MYSQL_TYPE_STRING;
        bind_param[15].buffer = const_cast<char*>(player.sect_id.data());
        bind_param[15].buffer_length = sect_id_len;
        bind_param[15].length = &sect_id_len;

        bind_param[16].buffer_type = MYSQL_TYPE_STRING;
        bind_param[16].buffer = const_cast<char*>(player.sect_name.data());
        bind_param[16].buffer_length = sect_name_len;
        bind_param[16].length = &sect_name_len;

        bind_param[17].buffer_type = MYSQL_TYPE_STRING;
        bind_param[17].buffer = const_cast<char*>(player.sect_rank.data());
        bind_param[17].buffer_length = sect_rank_len;
        bind_param[17].length = &sect_rank_len;

        bind_param[18].buffer_type = MYSQL_TYPE_STRING;
        bind_param[18].buffer = const_cast<char*>(inventory_json.data());
        bind_param[18].buffer_length = inventory_json_len;
        bind_param[18].length = &inventory_json_len;

        bind_param[19].buffer_type = MYSQL_TYPE_STRING;
        bind_param[19].buffer = const_cast<char*>(quest_json.data());
        bind_param[19].buffer_length = quest_json_len;
        bind_param[19].length = &quest_json_len;

        bind_param[20].buffer_type = MYSQL_TYPE_STRING;
        bind_param[20].buffer = const_cast<char*>(player.account.data());
        bind_param[20].buffer_length = account_len;
        bind_param[20].length = &account_len;

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

        updated = mysql_stmt_affected_rows(stmt) >= 0;
    } while(false);

    mysql_stmt_close(stmt);
    return updated;
}

bool MySqlMudPlayerRepository::query_top_players(MYSQL* mysql_handle,
                                                 MudLeaderboardType leaderboard_type,
                                                 int limit,
                                                 std::vector<MudLeaderboardEntry>* out_players,
                                                 std::string* error)
{
    if(mysql_handle == nullptr || out_players == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_invalid_query_top_players_args";
        }
        return false;
    }

    out_players->clear();

    std::string order_by = "realm_stage DESC, exp DESC, level DESC";
    if(leaderboard_type == MudLeaderboardType::wealth)
    {
        order_by = "spirit_stone DESC, exp DESC";
    }
    else if(leaderboard_type == MudLeaderboardType::combat)
    {
        order_by = "(attack_power + defense_power + level * 10) DESC, exp DESC";
    }

    const int normalized_limit = std::clamp(limit <= 0 ? 10 : limit, 1, 50);
    const std::string sql =
        "SELECT account,character_name,level,hp,max_hp,attack_power,defense_power,spirit_stone,"
        "title,location_scene_id,realm_name,realm_stage,exp,next_breakthrough_exp,primary_skill,"
        "skill_level,sect_id,sect_name,sect_rank,inventory_json,quest_json "
        "FROM mud_character ORDER BY " + order_by + " LIMIT " + std::to_string(normalized_limit);

    if(mysql_query(mysql_handle, sql.c_str()) != 0)
    {
        if(error != nullptr)
        {
            *error = mysql_error(mysql_handle);
        }
        return false;
    }

    MYSQL_RES* result = mysql_store_result(mysql_handle);
    if(result == nullptr)
    {
        if(error != nullptr)
        {
            *error = mysql_error(mysql_handle);
        }
        return false;
    }

    int rank = 1;
    while(MYSQL_ROW row = mysql_fetch_row(result))
    {
        unsigned long* lengths = mysql_fetch_lengths(result);
        if(lengths == nullptr)
        {
            continue;
        }

        auto field_text = [&](int index) -> std::string {
            if(row[index] == nullptr)
            {
                return {};
            }
            return std::string(row[index], lengths[index]);
        };

        auto field_int = [&](int index) -> int {
            auto text = field_text(index);
            return text.empty() ? 0 : std::stoi(text);
        };

        auto field_int64 = [&](int index) -> int64_t {
            auto text = field_text(index);
            return text.empty() ? 0 : std::stoll(text);
        };

        MudPlayerState player;
        player.account = field_text(0);
        player.character_name = field_text(1);
        player.level = field_int(2);
        player.hp = field_int(3);
        player.max_hp = field_int(4);
        player.attack_power = field_int(5);
        player.defense_power = field_int(6);
        player.spirit_stone = field_int64(7);
        player.title = field_text(8);
        player.location_scene_id = field_text(9);
        player.realm_name = field_text(10);
        player.realm_stage = field_int(11);
        player.exp = field_int64(12);
        player.next_breakthrough_exp = field_int64(13);
        player.primary_skill = field_text(14);
        player.skill_level = field_int(15);
        player.sect_id = field_text(16);
        player.sect_name = field_text(17);
        player.sect_rank = field_text(18);
        decode_inventory_json(field_text(19), &player.inventory);
        decode_quest_json(field_text(20), &player.quests);

        MudLeaderboardEntry entry;
        entry.rank = rank++;
        entry.player = std::move(player);
        out_players->push_back(std::move(entry));
    }

    mysql_free_result(result);
    return true;
}

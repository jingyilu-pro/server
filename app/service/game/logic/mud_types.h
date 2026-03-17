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

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

enum class MudLeaderboardType
{
    wealth,
    realm,
    combat,
};

struct MudStarterInventoryItem
{
    std::string item_id;
    int quantity = 0;
    bool equipped = false;
};

struct MudWorldDefaults
{
    std::string starting_scene_id;
    std::string starter_title;
    std::string starter_skill;
    std::string starter_realm_name;
    int starter_realm_stage = 0;
    int starter_hp = 100;
    int starter_attack = 18;
    int starter_defense = 10;
    int starter_spirit_stone = 80;
    int64_t starter_next_breakthrough_exp = 120;
    std::vector<MudStarterInventoryItem> starter_inventory;
    std::vector<std::string> realm_names;
};

struct MudItemConfig
{
    std::string item_id;
    std::string name;
    std::string item_type;
    std::string description;
    int price = 0;
    int hp_restore = 0;
    int64_t exp_gain = 0;
    int skill_level_gain = 0;
    int attack_bonus = 0;
    int defense_bonus = 0;
    bool consumable = false;
    bool equipable = false;
};

struct MudQuestConfig
{
    std::string quest_id;
    std::string title;
    std::string description;
    std::string issuer_npc_id;
    std::string submit_npc_id;
    std::string required_item_id;
    int required_item_count = 0;
    int reward_spirit_stone = 0;
    int64_t reward_exp = 0;
    std::string reward_item_id;
    int reward_item_count = 0;
    std::string reward_sect_id;
};

struct MudNpcConfig
{
    std::string npc_id;
    std::string name;
    std::string scene_id;
    std::string hint;
    std::string dialogue;
    std::vector<std::string> quest_ids;
    std::string sect_offer_id;
};

struct MudMonsterConfig
{
    std::string monster_id;
    std::string name;
    std::string scene_id;
    int hp = 0;
    int attack = 0;
    int defense = 0;
    int reward_spirit_stone = 0;
    int64_t reward_exp = 0;
    std::string drop_item_id;
    int drop_item_count = 0;
};

struct MudSectConfig
{
    std::string sect_id;
    std::string name;
    std::string rank_title;
    std::string join_scene_id;
    std::string join_npc_id;
};

struct MudSceneConfig
{
    std::string scene_id;
    std::string name;
    std::string region_name;
    std::string description;
    std::unordered_map<std::string, std::string> exits;
    std::vector<std::string> npc_ids;
    std::vector<std::string> monster_ids;
    std::vector<std::string> shop_item_ids;
};

struct MudInventoryItemState
{
    std::string item_id;
    int quantity = 0;
    bool equipped = false;
};

struct MudQuestState
{
    std::string quest_id;
    std::string status;
    int progress = 0;
};

struct MudPlayerState
{
    std::string account;
    std::string character_name;
    int level = 1;
    int hp = 100;
    int max_hp = 100;
    int attack_power = 18;
    int defense_power = 10;
    int64_t spirit_stone = 80;
    std::string title;
    std::string location_scene_id;
    std::string realm_name;
    int realm_stage = 0;
    int64_t exp = 0;
    int64_t next_breakthrough_exp = 120;
    std::string primary_skill;
    int skill_level = 1;
    std::string sect_id;
    std::string sect_name;
    std::string sect_rank;
    std::vector<MudInventoryItemState> inventory;
    std::vector<MudQuestState> quests;
};

struct MudEventEnvelope
{
    uint64_t event_id = 0;
    std::string target_account;
    std::string type;
    std::string title;
    std::string content;
    int64_t server_time_ms = 0;
};

struct MudCommandParseResult
{
    std::string verb;
    std::vector<std::string> args;
    std::string raw_args;
};

struct MudCommandExecution
{
    bool success = false;
    std::string title;
    std::string summary;
    std::vector<std::string> hints;
    int recommended_poll_interval_ms = 1500;
    std::vector<MudEventEnvelope> events;
};

struct MudLeaderboardEntry
{
    int rank = 0;
    MudPlayerState player;
};

struct MudTeamMemberState
{
    std::string account;
    std::string display_name;
    bool leader = false;
};

struct MudTeamState
{
    std::string team_id;
    std::string team_name;
    std::string leader_account;
    std::vector<MudTeamMemberState> members;
};

int64_t mud_now_ms();
std::string mud_to_lower_ascii(std::string value);
std::string mud_trim(const std::string& input);
MudCommandParseResult parse_mud_command(const std::string& command);
MudLeaderboardType mud_parse_leaderboard_type(const std::string& value);
std::string mud_leaderboard_name(MudLeaderboardType type);

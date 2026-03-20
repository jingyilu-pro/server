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
    alchemy,
    travel,
    bounty,
    chief,
};

struct MudUnlockRule
{
    std::string trigger;
    std::string target_id;
};

struct MudBaseAttributeState
{
    int spi = 0;
    int gin = 0;
    int str = 0;
    int per = 0;
    int int_attr = 0;
    int cha = 0;
    int luc = 0;
};

struct MudStatusAttributeState
{
    int kee = 0;
    int sen = 0;
    int sta = 0;
    int mana = 0;
};

struct MudCombatAttributeState
{
    int phys_hit = 0;
    int phys_crit = 0;
    int phys_damage = 0;
    int phys_haste = 0;
    int spell_hit = 0;
    int spell_crit = 0;
    int spell_damage = 0;
    int spell_haste = 0;
    int dodge = 0;
    int block = 0;
    int shield = 0;
    int parry = 0;
    int armor = 0;
    int resist_fire = 0;
    int resist_ice = 0;
    int resist_thunder = 0;
    int resist_wind = 0;
    int resist_corrosion = 0;
    int resist_poison = 0;
    int resist_pierce = 0;
    int resist_slash = 0;
    int resist_blunt = 0;
};

struct MudSummaryEntry
{
    std::string entry_id;
    std::string title;
    std::string summary;
    std::string status;
    std::string category;
    std::string command;
    std::string location_hint;
    std::string reward_summary;
};

struct MudStructuredPanelState
{
    std::string panel_id;
    std::string title;
    std::string summary;
    std::vector<MudSummaryEntry> entries;
};

struct MudRouteSummaryState
{
    std::string route_id;
    std::string title;
    std::string status;
    std::string summary;
    std::string next_step;
};

struct MudWeeklyEventSummaryState
{
    std::string event_id;
    std::string title;
    std::string summary;
    std::string risk_level;
    std::string location_hint;
    std::string command_hint;
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
    std::vector<std::string> starter_spell_ids;
    std::vector<std::string> starter_recipe_ids;
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

struct MudOriginConfig
{
    std::string origin_id;
    std::string name;
    std::string race_name;
    std::string homeland;
    std::string description;
    MudBaseAttributeState base_attributes;
    std::string starter_skill_id;
    std::vector<std::string> starter_spell_ids;
};

struct MudBackgroundConfig
{
    std::string background_id;
    std::string name;
    std::string description;
    std::string starter_title;
    std::string focus_label;
    MudBaseAttributeState attribute_bonus;
    std::vector<MudStarterInventoryItem> starter_inventory;
};

struct MudItemConfig
{
    std::string item_id;
    std::string name;
    std::string item_type;
    std::string description;
    int price = 0;
    int hp_restore = 0;
    int mana_restore = 0;
    int sen_restore = 0;
    int sta_restore = 0;
    int64_t exp_gain = 0;
    int skill_level_gain = 0;
    int attack_bonus = 0;
    int defense_bonus = 0;
    int spell_damage_bonus = 0;
    int spell_haste_bonus = 0;
    bool consumable = false;
    bool equipable = false;
    std::vector<std::string> tags;
    std::string codex_entry_id;
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
    std::vector<MudUnlockRule> unlock_rules;
    std::string chapter;
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
    std::string role;
    std::string description;
    std::string codex_entry_id;
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
    std::string description;
    std::string kind;
    std::string element;
    std::string codex_entry_id;
};

struct MudSectConfig
{
    std::string sect_id;
    std::string name;
    std::string rank_title;
    std::vector<std::string> rank_titles;
    std::string chief_title;
    std::string join_scene_id;
    std::string join_npc_id;
    std::string description;
    std::string codex_entry_id;
    bool joinable = false;
};

struct MudSkillConfig
{
    std::string skill_id;
    std::string name;
    std::string category;
    std::string description;
    std::string governing_attribute;
    bool starter = false;
    std::string chapter;
    std::string codex_entry_id;
};

struct MudSpellConfig
{
    std::string spell_id;
    std::string name;
    std::string element;
    std::string description;
    int mana_cost = 0;
    int power = 0;
    int required_realm_stage = 0;
    std::string granted_by_item_id;
    std::string chapter;
    std::string codex_entry_id;
};

struct MudRecipeIngredient
{
    std::string item_id;
    int quantity = 0;
};

struct MudRecipeConfig
{
    std::string recipe_id;
    std::string name;
    std::string description;
    std::string result_item_id;
    int result_quantity = 0;
    std::vector<MudRecipeIngredient> ingredient_items;
    std::string station_scene_id;
    std::string npc_id;
    double success_rate = 0.8;
    std::string required_skill_id;
    std::string chapter;
    std::string codex_entry_id;
};

struct MudTreasureConfig
{
    std::string treasure_id;
    std::string name;
    std::string description;
    std::string effect_summary;
};

struct MudFormationConfig
{
    std::string formation_id;
    std::string name;
    std::string description;
    std::string scene_id;
    std::string effect_summary;
    std::string codex_entry_id;
};

struct MudResourceNodeConfig
{
    std::string node_id;
    std::string name;
    std::string scene_id;
    std::string description;
    std::string drop_item_id;
    int drop_item_count = 0;
    int cooldown_ms = 0;
    std::string required_skill_id;
    std::string codex_entry_id;
};

struct MudGroundLootConfig
{
    std::string loot_id;
    std::string scene_id;
    std::string item_id;
    int quantity = 0;
    std::string description;
    bool one_time = true;
    std::string codex_entry_id;
};

struct MudHazardConfig
{
    std::string hazard_id;
    std::string scene_id;
    std::string name;
    std::string description;
    int hp_cost = 0;
    int mana_cost = 0;
    int sta_cost = 0;
    int sen_cost = 0;
    std::string resist_key;
    std::string codex_entry_id;
};

struct MudCodexEntryConfig
{
    std::string entry_id;
    std::string category;
    std::string title;
    std::string summary;
    std::string content;
    std::vector<std::string> related_scene_ids;
    std::vector<std::string> related_npc_ids;
    std::vector<std::string> related_monster_ids;
    std::vector<std::string> related_item_ids;
    std::vector<std::string> related_sect_ids;
    std::vector<MudUnlockRule> unlock_rules;
};

struct MudSceneConfig
{
    std::string scene_id;
    std::string name;
    std::string region_name;
    std::string description;
    std::string room_type;
    std::string risk_level;
    std::string landmark;
    std::string room_layer;
    bool pvp_enabled = false;
    std::vector<std::string> rumors;
    std::vector<std::string> loop_tags;
    std::unordered_map<std::string, std::string> exits;
    std::vector<std::string> npc_ids;
    std::vector<std::string> monster_ids;
    std::vector<std::string> shop_item_ids;
    std::vector<std::string> resource_node_ids;
    std::vector<std::string> ground_loot_ids;
    std::vector<std::string> hazard_ids;
    std::vector<std::string> codex_entry_ids;
    int map_x = 0;
    int map_y = 0;
    std::string chapter;
    std::string codex_entry_id;
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

struct MudSkillState
{
    std::string skill_id;
    int level = 1;
    int64_t proficiency = 0;
};

struct MudSpellState
{
    std::string spell_id;
    int level = 1;
    int64_t proficiency = 0;
    bool unlocked = true;
};

struct MudRecipeState
{
    std::string recipe_id;
    int level = 1;
    int64_t proficiency = 0;
    bool unlocked = false;
};

struct MudProfessionState
{
    int alchemy_level = 1;
    int exploration_level = 1;
    int formation_level = 1;
    int forging_level = 1;
};

struct MudCodexState
{
    std::string entry_id;
    bool unread = true;
    int64_t unlocked_at_ms = 0;
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
    std::string team_id;
    std::string team_name;
    std::string team_leader_account;
    std::string origin_id;
    std::string origin_name;
    std::string race_name;
    std::string homeland;
    std::string background_id;
    std::string background_name;
    std::vector<MudInventoryItemState> inventory;
    std::vector<MudQuestState> quests;
    MudBaseAttributeState base_attributes;
    MudStatusAttributeState status_attributes;
    MudCombatAttributeState combat_attributes;
    std::vector<MudSkillState> skills;
    std::vector<MudSpellState> spells;
    std::vector<MudRecipeState> recipes;
    std::vector<MudCodexState> codex_entries;
    MudProfessionState profession;
    std::unordered_map<std::string, std::string> flags;
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
    std::vector<MudPlayerState> extra_players_to_save;
    std::vector<std::string> unlocked_codex_entry_ids;
    std::string spell_summary;
    std::string brew_summary;
    std::string hazard_feedback;
    std::vector<MudStructuredPanelState> panels;
};

struct MudLeaderboardEntry
{
    int rank = 0;
    MudPlayerState player;
    int64_t score = 0;
    std::string extra;
};

struct MudTeamMemberState
{
    std::string account;
    std::string display_name;
    bool leader = false;
    MudPlayerState player_state;
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

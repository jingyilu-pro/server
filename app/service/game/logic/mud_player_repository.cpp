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
    "team_id VARCHAR(128) NOT NULL DEFAULT '',"
    "team_name VARCHAR(128) NOT NULL DEFAULT '',"
    "team_leader_account VARCHAR(128) NOT NULL DEFAULT '',"
    "origin_id VARCHAR(64) NOT NULL DEFAULT '',"
    "inventory_json LONGTEXT NOT NULL,"
    "quest_json LONGTEXT NOT NULL,"
    "attributes_json LONGTEXT NULL,"
    "status_json LONGTEXT NULL,"
    "combat_json LONGTEXT NULL,"
    "skills_json LONGTEXT NULL,"
    "spells_json LONGTEXT NULL,"
    "recipes_json LONGTEXT NULL,"
    "codex_json LONGTEXT NULL,"
    "profession_json LONGTEXT NULL,"
    "flags_json LONGTEXT NULL,"
    "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
    "updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

constexpr std::array<const char*, 10> kMudCharacterAlterSqls = {
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS origin_id VARCHAR(64) NOT NULL DEFAULT ''",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS attributes_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS status_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS combat_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS skills_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS spells_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS recipes_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS codex_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS profession_json LONGTEXT NULL",
    "ALTER TABLE mud_character ADD COLUMN IF NOT EXISTS flags_json LONGTEXT NULL",
};

constexpr const char* kPlayerSelectColumns =
    "account,character_name,level,hp,max_hp,attack_power,defense_power,spirit_stone,"
    "title,location_scene_id,realm_name,realm_stage,exp,next_breakthrough_exp,primary_skill,"
    "skill_level,sect_id,sect_name,sect_rank,team_id,team_name,team_leader_account,origin_id,"
    "inventory_json,quest_json,attributes_json,status_json,combat_json,skills_json,spells_json,"
    "recipes_json,codex_json,profession_json,flags_json";

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

std::string dump_json_or_fallback(json_t* root, const char* fallback)
{
    if(root == nullptr)
    {
        return fallback == nullptr ? "{}" : fallback;
    }

    char* dumped = json_dumps(root, JSON_COMPACT);
    std::string output = dumped == nullptr ? (fallback == nullptr ? "{}" : fallback) : dumped;
    if(dumped != nullptr)
    {
        free(dumped);
    }
    json_decref(root);
    return output;
}

std::string encode_base_attributes_json(const MudBaseAttributeState& attributes)
{
    json_t* root = json_object();
    if(root == nullptr)
    {
        return "{}";
    }

    json_object_set_new(root, "spi", json_integer(attributes.spi));
    json_object_set_new(root, "gin", json_integer(attributes.gin));
    json_object_set_new(root, "str", json_integer(attributes.str));
    json_object_set_new(root, "per", json_integer(attributes.per));
    json_object_set_new(root, "int_attr", json_integer(attributes.int_attr));
    json_object_set_new(root, "cha", json_integer(attributes.cha));
    json_object_set_new(root, "luc", json_integer(attributes.luc));
    return dump_json_or_fallback(root, "{}");
}

std::string encode_status_attributes_json(const MudStatusAttributeState& attributes)
{
    json_t* root = json_object();
    if(root == nullptr)
    {
        return "{}";
    }

    json_object_set_new(root, "kee", json_integer(attributes.kee));
    json_object_set_new(root, "sen", json_integer(attributes.sen));
    json_object_set_new(root, "sta", json_integer(attributes.sta));
    json_object_set_new(root, "mana", json_integer(attributes.mana));
    return dump_json_or_fallback(root, "{}");
}

std::string encode_combat_attributes_json(const MudCombatAttributeState& attributes)
{
    json_t* root = json_object();
    if(root == nullptr)
    {
        return "{}";
    }

    json_object_set_new(root, "phys_hit", json_integer(attributes.phys_hit));
    json_object_set_new(root, "phys_crit", json_integer(attributes.phys_crit));
    json_object_set_new(root, "phys_damage", json_integer(attributes.phys_damage));
    json_object_set_new(root, "phys_haste", json_integer(attributes.phys_haste));
    json_object_set_new(root, "spell_hit", json_integer(attributes.spell_hit));
    json_object_set_new(root, "spell_crit", json_integer(attributes.spell_crit));
    json_object_set_new(root, "spell_damage", json_integer(attributes.spell_damage));
    json_object_set_new(root, "spell_haste", json_integer(attributes.spell_haste));
    json_object_set_new(root, "dodge", json_integer(attributes.dodge));
    json_object_set_new(root, "block", json_integer(attributes.block));
    json_object_set_new(root, "shield", json_integer(attributes.shield));
    json_object_set_new(root, "parry", json_integer(attributes.parry));
    json_object_set_new(root, "armor", json_integer(attributes.armor));
    json_object_set_new(root, "resist_fire", json_integer(attributes.resist_fire));
    json_object_set_new(root, "resist_ice", json_integer(attributes.resist_ice));
    json_object_set_new(root, "resist_thunder", json_integer(attributes.resist_thunder));
    json_object_set_new(root, "resist_wind", json_integer(attributes.resist_wind));
    json_object_set_new(root, "resist_corrosion", json_integer(attributes.resist_corrosion));
    json_object_set_new(root, "resist_poison", json_integer(attributes.resist_poison));
    json_object_set_new(root, "resist_pierce", json_integer(attributes.resist_pierce));
    json_object_set_new(root, "resist_slash", json_integer(attributes.resist_slash));
    json_object_set_new(root, "resist_blunt", json_integer(attributes.resist_blunt));
    return dump_json_or_fallback(root, "{}");
}

std::string encode_skills_json(const std::vector<MudSkillState>& skills)
{
    json_t* root = json_array();
    if(root == nullptr)
    {
        return "[]";
    }

    for(const auto& skill : skills)
    {
        json_t* node = json_object();
        if(node == nullptr)
        {
            continue;
        }
        json_object_set_new(node, "skill_id", json_string(skill.skill_id.c_str()));
        json_object_set_new(node, "level", json_integer(skill.level));
        json_object_set_new(node, "proficiency", json_integer(skill.proficiency));
        json_array_append_new(root, node);
    }
    return dump_json_or_fallback(root, "[]");
}

std::string encode_spells_json(const std::vector<MudSpellState>& spells)
{
    json_t* root = json_array();
    if(root == nullptr)
    {
        return "[]";
    }

    for(const auto& spell : spells)
    {
        json_t* node = json_object();
        if(node == nullptr)
        {
            continue;
        }
        json_object_set_new(node, "spell_id", json_string(spell.spell_id.c_str()));
        json_object_set_new(node, "level", json_integer(spell.level));
        json_object_set_new(node, "proficiency", json_integer(spell.proficiency));
        json_object_set_new(node, "unlocked", spell.unlocked ? json_true() : json_false());
        json_array_append_new(root, node);
    }
    return dump_json_or_fallback(root, "[]");
}

std::string encode_recipes_json(const std::vector<MudRecipeState>& recipes)
{
    json_t* root = json_array();
    if(root == nullptr)
    {
        return "[]";
    }

    for(const auto& recipe : recipes)
    {
        json_t* node = json_object();
        if(node == nullptr)
        {
            continue;
        }
        json_object_set_new(node, "recipe_id", json_string(recipe.recipe_id.c_str()));
        json_object_set_new(node, "level", json_integer(recipe.level));
        json_object_set_new(node, "proficiency", json_integer(recipe.proficiency));
        json_object_set_new(node, "unlocked", recipe.unlocked ? json_true() : json_false());
        json_array_append_new(root, node);
    }
    return dump_json_or_fallback(root, "[]");
}

std::string encode_codex_json(const std::vector<MudCodexState>& codex_entries)
{
    json_t* root = json_array();
    if(root == nullptr)
    {
        return "[]";
    }

    for(const auto& entry : codex_entries)
    {
        json_t* node = json_object();
        if(node == nullptr)
        {
            continue;
        }
        json_object_set_new(node, "entry_id", json_string(entry.entry_id.c_str()));
        json_object_set_new(node, "unread", entry.unread ? json_true() : json_false());
        json_object_set_new(node, "unlocked_at_ms", json_integer(entry.unlocked_at_ms));
        json_array_append_new(root, node);
    }
    return dump_json_or_fallback(root, "[]");
}

std::string encode_profession_json(const MudProfessionState& profession)
{
    json_t* root = json_object();
    if(root == nullptr)
    {
        return "{}";
    }

    json_object_set_new(root, "alchemy_level", json_integer(profession.alchemy_level));
    json_object_set_new(root, "exploration_level", json_integer(profession.exploration_level));
    json_object_set_new(root, "formation_level", json_integer(profession.formation_level));
    json_object_set_new(root, "forging_level", json_integer(profession.forging_level));
    return dump_json_or_fallback(root, "{}");
}

std::string encode_flags_json(const std::unordered_map<std::string, std::string>& flags)
{
    json_t* root = json_object();
    if(root == nullptr)
    {
        return "{}";
    }

    for(const auto& [key, value] : flags)
    {
        json_object_set_new(root, key.c_str(), json_string(value.c_str()));
    }
    return dump_json_or_fallback(root, "{}");
}

void decode_base_attributes_json(const std::string& json_text, MudBaseAttributeState* attributes)
{
    if(attributes == nullptr || json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_object(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    auto read_int = [&](const char* key) -> int {
        auto* value = json_object_get(root, key);
        return value != nullptr && json_is_integer(value) ? static_cast<int>(json_integer_value(value)) : 0;
    };

    attributes->spi = read_int("spi");
    attributes->gin = read_int("gin");
    attributes->str = read_int("str");
    attributes->per = read_int("per");
    attributes->int_attr = read_int("int_attr");
    attributes->cha = read_int("cha");
    attributes->luc = read_int("luc");
    json_decref(root);
}

void decode_status_attributes_json(const std::string& json_text, MudStatusAttributeState* attributes)
{
    if(attributes == nullptr || json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_object(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    auto read_int = [&](const char* key) -> int {
        auto* value = json_object_get(root, key);
        return value != nullptr && json_is_integer(value) ? static_cast<int>(json_integer_value(value)) : 0;
    };

    attributes->kee = read_int("kee");
    attributes->sen = read_int("sen");
    attributes->sta = read_int("sta");
    attributes->mana = read_int("mana");
    json_decref(root);
}

void decode_combat_attributes_json(const std::string& json_text, MudCombatAttributeState* attributes)
{
    if(attributes == nullptr || json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_object(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    auto read_int = [&](const char* key) -> int {
        auto* value = json_object_get(root, key);
        return value != nullptr && json_is_integer(value) ? static_cast<int>(json_integer_value(value)) : 0;
    };

    attributes->phys_hit = read_int("phys_hit");
    attributes->phys_crit = read_int("phys_crit");
    attributes->phys_damage = read_int("phys_damage");
    attributes->phys_haste = read_int("phys_haste");
    attributes->spell_hit = read_int("spell_hit");
    attributes->spell_crit = read_int("spell_crit");
    attributes->spell_damage = read_int("spell_damage");
    attributes->spell_haste = read_int("spell_haste");
    attributes->dodge = read_int("dodge");
    attributes->block = read_int("block");
    attributes->shield = read_int("shield");
    attributes->parry = read_int("parry");
    attributes->armor = read_int("armor");
    attributes->resist_fire = read_int("resist_fire");
    attributes->resist_ice = read_int("resist_ice");
    attributes->resist_thunder = read_int("resist_thunder");
    attributes->resist_wind = read_int("resist_wind");
    attributes->resist_corrosion = read_int("resist_corrosion");
    attributes->resist_poison = read_int("resist_poison");
    attributes->resist_pierce = read_int("resist_pierce");
    attributes->resist_slash = read_int("resist_slash");
    attributes->resist_blunt = read_int("resist_blunt");
    json_decref(root);
}

void decode_skills_json(const std::string& json_text, std::vector<MudSkillState>* skills)
{
    if(skills == nullptr)
    {
        return;
    }
    skills->clear();
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
    skills->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(root, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudSkillState skill;
        if(auto* value = json_object_get(item, "skill_id"); value != nullptr && json_is_string(value))
        {
            skill.skill_id = json_string_value(value);
        }
        if(auto* value = json_object_get(item, "level"); value != nullptr && json_is_integer(value))
        {
            skill.level = static_cast<int>(json_integer_value(value));
        }
        if(auto* value = json_object_get(item, "proficiency"); value != nullptr && json_is_integer(value))
        {
            skill.proficiency = static_cast<int64_t>(json_integer_value(value));
        }
        if(!skill.skill_id.empty())
        {
            skills->push_back(std::move(skill));
        }
    }
    json_decref(root);
}

void decode_spells_json(const std::string& json_text, std::vector<MudSpellState>* spells)
{
    if(spells == nullptr)
    {
        return;
    }
    spells->clear();
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
    spells->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(root, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudSpellState spell;
        if(auto* value = json_object_get(item, "spell_id"); value != nullptr && json_is_string(value))
        {
            spell.spell_id = json_string_value(value);
        }
        if(auto* value = json_object_get(item, "level"); value != nullptr && json_is_integer(value))
        {
            spell.level = static_cast<int>(json_integer_value(value));
        }
        if(auto* value = json_object_get(item, "proficiency"); value != nullptr && json_is_integer(value))
        {
            spell.proficiency = static_cast<int64_t>(json_integer_value(value));
        }
        if(auto* value = json_object_get(item, "unlocked"); value != nullptr && json_is_boolean(value))
        {
            spell.unlocked = json_is_true(value);
        }
        if(!spell.spell_id.empty())
        {
            spells->push_back(std::move(spell));
        }
    }
    json_decref(root);
}

void decode_recipes_json(const std::string& json_text, std::vector<MudRecipeState>* recipes)
{
    if(recipes == nullptr)
    {
        return;
    }
    recipes->clear();
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
    recipes->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(root, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudRecipeState recipe;
        if(auto* value = json_object_get(item, "recipe_id"); value != nullptr && json_is_string(value))
        {
            recipe.recipe_id = json_string_value(value);
        }
        if(auto* value = json_object_get(item, "level"); value != nullptr && json_is_integer(value))
        {
            recipe.level = static_cast<int>(json_integer_value(value));
        }
        if(auto* value = json_object_get(item, "proficiency"); value != nullptr && json_is_integer(value))
        {
            recipe.proficiency = static_cast<int64_t>(json_integer_value(value));
        }
        if(auto* value = json_object_get(item, "unlocked"); value != nullptr && json_is_boolean(value))
        {
            recipe.unlocked = json_is_true(value);
        }
        if(!recipe.recipe_id.empty())
        {
            recipes->push_back(std::move(recipe));
        }
    }
    json_decref(root);
}

void decode_codex_json(const std::string& json_text, std::vector<MudCodexState>* codex_entries)
{
    if(codex_entries == nullptr)
    {
        return;
    }
    codex_entries->clear();
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
    codex_entries->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(root, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudCodexState state;
        if(auto* value = json_object_get(item, "entry_id"); value != nullptr && json_is_string(value))
        {
            state.entry_id = json_string_value(value);
        }
        if(auto* value = json_object_get(item, "unread"); value != nullptr && json_is_boolean(value))
        {
            state.unread = json_is_true(value);
        }
        if(auto* value = json_object_get(item, "unlocked_at_ms"); value != nullptr && json_is_integer(value))
        {
            state.unlocked_at_ms = static_cast<int64_t>(json_integer_value(value));
        }
        if(!state.entry_id.empty())
        {
            codex_entries->push_back(std::move(state));
        }
    }
    json_decref(root);
}

void decode_profession_json(const std::string& json_text, MudProfessionState* profession)
{
    if(profession == nullptr || json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_object(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    auto read_int = [&](const char* key) -> int {
        auto* value = json_object_get(root, key);
        return value != nullptr && json_is_integer(value) ? static_cast<int>(json_integer_value(value)) : 0;
    };

    profession->alchemy_level = read_int("alchemy_level");
    profession->exploration_level = read_int("exploration_level");
    profession->formation_level = read_int("formation_level");
    profession->forging_level = read_int("forging_level");
    json_decref(root);
}

void decode_flags_json(const std::string& json_text, std::unordered_map<std::string, std::string>* flags)
{
    if(flags == nullptr)
    {
        return;
    }
    flags->clear();
    if(json_text.empty())
    {
        return;
    }

    json_error_t error{};
    json_t* root = json_loads(json_text.c_str(), 0, &error);
    if(root == nullptr || !json_is_object(root))
    {
        if(root != nullptr)
        {
            json_decref(root);
        }
        return;
    }

    const char* key = nullptr;
    json_t* value = nullptr;
    json_object_foreach(root, key, value)
    {
        if(key == nullptr || value == nullptr || !json_is_string(value))
        {
            continue;
        }

        const char* text = json_string_value(value);
        flags->insert_or_assign(key, text == nullptr ? "" : text);
    }
    json_decref(root);
}

std::string escape_mysql_string(MYSQL* mysql_handle, const std::string& value)
{
    if(mysql_handle == nullptr)
    {
        return value;
    }

    std::string escaped(value.size() * 2 + 1, '\0');
    const auto escaped_len = mysql_real_escape_string(mysql_handle,
                                                      escaped.data(),
                                                      value.c_str(),
                                                      static_cast<unsigned long>(value.size()));
    escaped.resize(escaped_len);
    return escaped;
}

bool decode_player_row(MYSQL_ROW row, unsigned long* lengths, MudPlayerState* player)
{
    if(row == nullptr || lengths == nullptr || player == nullptr)
    {
        return false;
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

    player->account = field_text(0);
    player->character_name = field_text(1);
    player->level = field_int(2);
    player->hp = field_int(3);
    player->max_hp = field_int(4);
    player->attack_power = field_int(5);
    player->defense_power = field_int(6);
    player->spirit_stone = field_int64(7);
    player->title = field_text(8);
    player->location_scene_id = field_text(9);
    player->realm_name = field_text(10);
    player->realm_stage = field_int(11);
    player->exp = field_int64(12);
    player->next_breakthrough_exp = field_int64(13);
    player->primary_skill = field_text(14);
    player->skill_level = field_int(15);
    player->sect_id = field_text(16);
    player->sect_name = field_text(17);
    player->sect_rank = field_text(18);
    player->team_id = field_text(19);
    player->team_name = field_text(20);
    player->team_leader_account = field_text(21);
    player->origin_id = field_text(22);
    decode_inventory_json(field_text(23), &player->inventory);
    decode_quest_json(field_text(24), &player->quests);
    decode_base_attributes_json(field_text(25), &player->base_attributes);
    decode_status_attributes_json(field_text(26), &player->status_attributes);
    decode_combat_attributes_json(field_text(27), &player->combat_attributes);
    decode_skills_json(field_text(28), &player->skills);
    decode_spells_json(field_text(29), &player->spells);
    decode_recipes_json(field_text(30), &player->recipes);
    decode_codex_json(field_text(31), &player->codex_entries);
    decode_profession_json(field_text(32), &player->profession);
    decode_flags_json(field_text(33), &player->flags);
    return true;
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
                 "",
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
                 "",
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
                 "",
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
                 "",
                 std::nullopt,
                 leaderboard_type,
                 limit,
                 [this](MudPlayerRepositoryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable MySqlMudPlayerRepository::list_team_members(const std::string& team_id)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(MudPlayerRepositoryOpType::list_team_members,
                 "",
                 team_id,
                 std::nullopt,
                 MudLeaderboardType::realm,
                 0,
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
    case MudPlayerRepositoryOpType::list_team_members:
    {
        std::vector<MudPlayerState> players;
        const bool ok = query_team_members(worker_mysql,
                                           result->request_group_id,
                                           &players,
                                           &error);
        result->success = ok;
        result->players.clear();
        result->players.reserve(players.size());
        int rank = 1;
        for(auto& player : players)
        {
            MudLeaderboardEntry entry;
            entry.rank = rank++;
            entry.player = std::move(player);
            result->players.push_back(std::move(entry));
        }
        if(!ok)
        {
            result->error = error.empty() ? "mysql_list_team_members_failed" : error;
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

    for(const auto* sql : kMudCharacterAlterSqls)
    {
        if(sql == nullptr)
        {
            continue;
        }
        if(mysql_query(m_mysql, sql) != 0)
        {
            spdlog::error("mysql alter mud_character failed sql={} err={}", sql, mysql_error(m_mysql));
            return false;
        }
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
    const std::string sql = "SELECT " + std::string(kPlayerSelectColumns) + " FROM mud_character WHERE account='" +
                            escape_mysql_string(mysql_handle, account) + "' LIMIT 1";

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

    bool ok = true;
    if(MYSQL_ROW row = mysql_fetch_row(result))
    {
        unsigned long* lengths = mysql_fetch_lengths(result);
        if(lengths == nullptr)
        {
            ok = false;
            if(error != nullptr)
            {
                *error = "mysql_fetch_lengths_failed";
            }
        }
        else
        {
            MudPlayerState player;
            decode_player_row(row, lengths, &player);
            *out_player = std::move(player);
        }
    }

    mysql_free_result(result);
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

    const std::string inventory_json = encode_inventory_json(player.inventory);
    const std::string quest_json = encode_quest_json(player.quests);
    const std::string attributes_json = encode_base_attributes_json(player.base_attributes);
    const std::string status_json = encode_status_attributes_json(player.status_attributes);
    const std::string combat_json = encode_combat_attributes_json(player.combat_attributes);
    const std::string skills_json = encode_skills_json(player.skills);
    const std::string spells_json = encode_spells_json(player.spells);
    const std::string recipes_json = encode_recipes_json(player.recipes);
    const std::string codex_json = encode_codex_json(player.codex_entries);
    const std::string profession_json = encode_profession_json(player.profession);
    const std::string flags_json = encode_flags_json(player.flags);

    auto quoted = [&](const std::string& value) {
        return "'" + escape_mysql_string(mysql_handle, value) + "'";
    };

    const std::string sql =
        "INSERT INTO mud_character(account,character_name,level,hp,max_hp,attack_power,defense_power,"
        "spirit_stone,title,location_scene_id,realm_name,realm_stage,exp,next_breakthrough_exp,"
        "primary_skill,skill_level,sect_id,sect_name,sect_rank,team_id,team_name,team_leader_account,"
        "origin_id,inventory_json,quest_json,attributes_json,status_json,combat_json,skills_json,"
        "spells_json,recipes_json,codex_json,profession_json,flags_json) VALUES(" +
        quoted(player.account) + "," + quoted(player.character_name) + "," + std::to_string(player.level) + "," +
        std::to_string(player.hp) + "," + std::to_string(player.max_hp) + "," + std::to_string(player.attack_power) +
        "," + std::to_string(player.defense_power) + "," + std::to_string(player.spirit_stone) + "," +
        quoted(player.title) + "," + quoted(player.location_scene_id) + "," + quoted(player.realm_name) + "," +
        std::to_string(player.realm_stage) + "," + std::to_string(player.exp) + "," +
        std::to_string(player.next_breakthrough_exp) + "," + quoted(player.primary_skill) + "," +
        std::to_string(player.skill_level) + "," + quoted(player.sect_id) + "," + quoted(player.sect_name) + "," +
        quoted(player.sect_rank) + "," + quoted(player.team_id) + "," + quoted(player.team_name) + "," +
        quoted(player.team_leader_account) + "," + quoted(player.origin_id) + "," + quoted(inventory_json) + "," +
        quoted(quest_json) + "," + quoted(attributes_json) + "," + quoted(status_json) + "," +
        quoted(combat_json) + "," + quoted(skills_json) + "," + quoted(spells_json) + "," +
        quoted(recipes_json) + "," + quoted(codex_json) + "," + quoted(profession_json) + "," +
        quoted(flags_json) + ")";

    if(mysql_query(mysql_handle, sql.c_str()) != 0)
    {
        if(error != nullptr)
        {
            *error = mysql_error(mysql_handle);
        }
        return false;
    }

    return true;
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

    const std::string inventory_json = encode_inventory_json(player.inventory);
    const std::string quest_json = encode_quest_json(player.quests);
    const std::string attributes_json = encode_base_attributes_json(player.base_attributes);
    const std::string status_json = encode_status_attributes_json(player.status_attributes);
    const std::string combat_json = encode_combat_attributes_json(player.combat_attributes);
    const std::string skills_json = encode_skills_json(player.skills);
    const std::string spells_json = encode_spells_json(player.spells);
    const std::string recipes_json = encode_recipes_json(player.recipes);
    const std::string codex_json = encode_codex_json(player.codex_entries);
    const std::string profession_json = encode_profession_json(player.profession);
    const std::string flags_json = encode_flags_json(player.flags);

    auto quoted = [&](const std::string& value) {
        return "'" + escape_mysql_string(mysql_handle, value) + "'";
    };

    const std::string sql =
        "UPDATE mud_character SET character_name=" + quoted(player.character_name) + ",level=" +
        std::to_string(player.level) + ",hp=" + std::to_string(player.hp) + ",max_hp=" +
        std::to_string(player.max_hp) + ",attack_power=" + std::to_string(player.attack_power) +
        ",defense_power=" + std::to_string(player.defense_power) + ",spirit_stone=" +
        std::to_string(player.spirit_stone) + ",title=" + quoted(player.title) + ",location_scene_id=" +
        quoted(player.location_scene_id) + ",realm_name=" + quoted(player.realm_name) + ",realm_stage=" +
        std::to_string(player.realm_stage) + ",exp=" + std::to_string(player.exp) +
        ",next_breakthrough_exp=" + std::to_string(player.next_breakthrough_exp) + ",primary_skill=" +
        quoted(player.primary_skill) + ",skill_level=" + std::to_string(player.skill_level) + ",sect_id=" +
        quoted(player.sect_id) + ",sect_name=" + quoted(player.sect_name) + ",sect_rank=" +
        quoted(player.sect_rank) + ",team_id=" + quoted(player.team_id) + ",team_name=" +
        quoted(player.team_name) + ",team_leader_account=" + quoted(player.team_leader_account) + ",origin_id=" +
        quoted(player.origin_id) + ",inventory_json=" + quoted(inventory_json) + ",quest_json=" +
        quoted(quest_json) + ",attributes_json=" + quoted(attributes_json) + ",status_json=" +
        quoted(status_json) + ",combat_json=" + quoted(combat_json) + ",skills_json=" + quoted(skills_json) +
        ",spells_json=" + quoted(spells_json) + ",recipes_json=" + quoted(recipes_json) + ",codex_json=" +
        quoted(codex_json) + ",profession_json=" + quoted(profession_json) + ",flags_json=" +
        quoted(flags_json) + " WHERE account=" + quoted(player.account);

    if(mysql_query(mysql_handle, sql.c_str()) != 0)
    {
        if(error != nullptr)
        {
            *error = mysql_error(mysql_handle);
        }
        return false;
    }

    return mysql_affected_rows(mysql_handle) >= 0;
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
        "SELECT " + std::string(kPlayerSelectColumns) + " FROM mud_character ORDER BY " + order_by + " LIMIT " +
        std::to_string(normalized_limit);

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

        MudPlayerState player;
        decode_player_row(row, lengths, &player);

        MudLeaderboardEntry entry;
        entry.rank = rank++;
        entry.player = std::move(player);
        out_players->push_back(std::move(entry));
    }

    mysql_free_result(result);
    return true;
}

bool MySqlMudPlayerRepository::query_team_members(MYSQL* mysql_handle,
                                                  const std::string& team_id,
                                                  std::vector<MudPlayerState>* out_players,
                                                  std::string* error)
{
    if(mysql_handle == nullptr || out_players == nullptr)
    {
        if(error != nullptr)
        {
            *error = "mysql_invalid_query_team_members_args";
        }
        return false;
    }

    out_players->clear();
    if(team_id.empty())
    {
        return true;
    }

    std::string escaped_team_id(team_id.size() * 2 + 1, '\0');
    const auto escaped_len = mysql_real_escape_string(mysql_handle,
                                                      escaped_team_id.data(),
                                                      team_id.c_str(),
                                                      static_cast<unsigned long>(team_id.size()));
    escaped_team_id.resize(escaped_len);

    const std::string sql =
        "SELECT " + std::string(kPlayerSelectColumns) + " FROM mud_character WHERE team_id='" + escaped_team_id +
        "' ORDER BY (account = team_leader_account) DESC, account ASC";

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

    while(MYSQL_ROW row = mysql_fetch_row(result))
    {
        unsigned long* lengths = mysql_fetch_lengths(result);
        if(lengths == nullptr)
        {
            continue;
        }

        MudPlayerState player;
        decode_player_row(row, lengths, &player);
        out_players->push_back(std::move(player));
    }

    mysql_free_result(result);
    return true;
}

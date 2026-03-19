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

#include "mud_world.h"

#include <jansson.h>

#include <algorithm>

namespace
{

std::string json_string_field(json_t* object, const char* key)
{
    if(object == nullptr || key == nullptr)
    {
        return {};
    }

    auto* value = json_object_get(object, key);
    if(value == nullptr || !json_is_string(value))
    {
        return {};
    }

    const char* text = json_string_value(value);
    return text == nullptr ? std::string{} : std::string(text);
}

int json_int_value(json_t* object, const char* key, int default_value = 0)
{
    if(object == nullptr || key == nullptr)
    {
        return default_value;
    }

    auto* value = json_object_get(object, key);
    if(value == nullptr || !json_is_integer(value))
    {
        return default_value;
    }

    return static_cast<int>(json_integer_value(value));
}

int64_t json_int64_value(json_t* object, const char* key, int64_t default_value = 0)
{
    if(object == nullptr || key == nullptr)
    {
        return default_value;
    }

    auto* value = json_object_get(object, key);
    if(value == nullptr || !json_is_integer(value))
    {
        return default_value;
    }

    return static_cast<int64_t>(json_integer_value(value));
}

double json_double_value(json_t* object, const char* key, double default_value = 0.0)
{
    if(object == nullptr || key == nullptr)
    {
        return default_value;
    }

    auto* value = json_object_get(object, key);
    if(value == nullptr)
    {
        return default_value;
    }
    if(json_is_real(value))
    {
        return json_real_value(value);
    }
    if(json_is_integer(value))
    {
        return static_cast<double>(json_integer_value(value));
    }
    return default_value;
}

bool json_bool_value(json_t* object, const char* key, bool default_value = false)
{
    if(object == nullptr || key == nullptr)
    {
        return default_value;
    }

    auto* value = json_object_get(object, key);
    if(value == nullptr || !json_is_boolean(value))
    {
        return default_value;
    }

    return json_is_true(value);
}

void load_string_array(json_t* array, std::vector<std::string>* output)
{
    if(array == nullptr || output == nullptr || !json_is_array(array))
    {
        return;
    }

    output->clear();
    const size_t count = json_array_size(array);
    output->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* value = json_array_get(array, index);
        if(value != nullptr && json_is_string(value))
        {
            const char* text = json_string_value(value);
            if(text != nullptr)
            {
                output->emplace_back(text);
            }
        }
    }
}

void load_string_map(json_t* object, std::unordered_map<std::string, std::string>* output)
{
    if(object == nullptr || output == nullptr || !json_is_object(object))
    {
        return;
    }

    output->clear();
    const char* key = nullptr;
    json_t* value = nullptr;
    json_object_foreach(object, key, value)
    {
        if(key == nullptr || value == nullptr || !json_is_string(value))
        {
            continue;
        }

        const char* text = json_string_value(value);
        output->insert_or_assign(key, text == nullptr ? "" : text);
    }
}

void load_unlock_rules(json_t* array, std::vector<MudUnlockRule>* output)
{
    if(array == nullptr || output == nullptr || !json_is_array(array))
    {
        return;
    }

    output->clear();
    const size_t count = json_array_size(array);
    output->reserve(count);
    for(size_t index = 0; index < count; ++index)
    {
        auto* item = json_array_get(array, index);
        if(item == nullptr || !json_is_object(item))
        {
            continue;
        }

        MudUnlockRule rule;
        rule.trigger = json_string_field(item, "trigger");
        rule.target_id = json_string_field(item, "target_id");
        if(!rule.trigger.empty() && !rule.target_id.empty())
        {
            output->push_back(std::move(rule));
        }
    }
}

MudBaseAttributeState load_base_attributes(json_t* object)
{
    MudBaseAttributeState state;
    if(object == nullptr || !json_is_object(object))
    {
        return state;
    }

    state.spi = json_int_value(object, "spi", 0);
    state.gin = json_int_value(object, "gin", 0);
    state.str = json_int_value(object, "str", 0);
    state.per = json_int_value(object, "per", 0);
    state.int_attr = json_int_value(object, "int_attr", 0);
    state.cha = json_int_value(object, "cha", 0);
    state.luc = json_int_value(object, "luc", 0);
    return state;
}

MudStatusAttributeState load_status_attributes(json_t* object)
{
    MudStatusAttributeState state;
    if(object == nullptr || !json_is_object(object))
    {
        return state;
    }

    state.kee = json_int_value(object, "kee", 0);
    state.sen = json_int_value(object, "sen", 0);
    state.sta = json_int_value(object, "sta", 0);
    state.mana = json_int_value(object, "mana", 0);
    return state;
}

MudCombatAttributeState load_combat_attributes(json_t* object)
{
    MudCombatAttributeState state;
    if(object == nullptr || !json_is_object(object))
    {
        return state;
    }

    state.phys_hit = json_int_value(object, "phys_hit", 0);
    state.phys_crit = json_int_value(object, "phys_crit", 0);
    state.phys_damage = json_int_value(object, "phys_damage", 0);
    state.phys_haste = json_int_value(object, "phys_haste", 0);
    state.spell_hit = json_int_value(object, "spell_hit", 0);
    state.spell_crit = json_int_value(object, "spell_crit", 0);
    state.spell_damage = json_int_value(object, "spell_damage", 0);
    state.spell_haste = json_int_value(object, "spell_haste", 0);
    state.dodge = json_int_value(object, "dodge", 0);
    state.block = json_int_value(object, "block", 0);
    state.shield = json_int_value(object, "shield", 0);
    state.parry = json_int_value(object, "parry", 0);
    state.armor = json_int_value(object, "armor", 0);
    state.resist_fire = json_int_value(object, "resist_fire", 0);
    state.resist_ice = json_int_value(object, "resist_ice", 0);
    state.resist_thunder = json_int_value(object, "resist_thunder", 0);
    state.resist_wind = json_int_value(object, "resist_wind", 0);
    state.resist_corrosion = json_int_value(object, "resist_corrosion", 0);
    state.resist_poison = json_int_value(object, "resist_poison", 0);
    state.resist_pierce = json_int_value(object, "resist_pierce", 0);
    state.resist_slash = json_int_value(object, "resist_slash", 0);
    state.resist_blunt = json_int_value(object, "resist_blunt", 0);
    return state;
}

template <typename T>
const T* find_in_map(const std::unordered_map<std::string, T>& table, const std::string& id)
{
    if(auto iter = table.find(id); iter != table.end())
    {
        return &iter->second;
    }
    return nullptr;
}

} // namespace

bool MudWorld::load_from_file(const std::string& path, std::string* error_message)
{
    m_ready = false;
    m_defaults = MudWorldDefaults{};
    m_default_base_attributes = MudBaseAttributeState{};
    m_default_status_attributes = MudStatusAttributeState{};
    m_default_combat_attributes = MudCombatAttributeState{};
    m_origins.clear();
    m_scenes.clear();
    m_npcs.clear();
    m_quests.clear();
    m_monsters.clear();
    m_items.clear();
    m_sects.clear();
    m_skills.clear();
    m_spells.clear();
    m_recipes.clear();
    m_treasures.clear();
    m_formations.clear();
    m_resource_nodes.clear();
    m_ground_loots.clear();
    m_hazards.clear();
    m_codex_entries.clear();
    m_codex_unlock_index.clear();

    json_error_t error{};
    json_t* root = json_load_file(path.c_str(), 0, &error);
    if(root == nullptr)
    {
        if(error_message != nullptr)
        {
            *error_message = "load world json failed: " + std::string(error.text);
        }
        return false;
    }

    auto cleanup = [&]() {
        json_decref(root);
    };

    auto* defaults = json_object_get(root, "defaults");
    if(defaults == nullptr || !json_is_object(defaults))
    {
        cleanup();
        if(error_message != nullptr)
        {
            *error_message = "world defaults missing";
        }
        return false;
    }

    m_defaults.starting_scene_id = json_string_field(defaults, "starting_scene_id");
    m_defaults.starter_title = json_string_field(defaults, "starter_title");
    m_defaults.starter_skill = json_string_field(defaults, "starter_skill");
    m_defaults.starter_realm_name = json_string_field(defaults, "starter_realm_name");
    m_defaults.starter_realm_stage = json_int_value(defaults, "starter_realm_stage", 0);
    m_defaults.starter_hp = json_int_value(defaults, "starter_hp", 100);
    m_defaults.starter_attack = json_int_value(defaults, "starter_attack", 18);
    m_defaults.starter_defense = json_int_value(defaults, "starter_defense", 10);
    m_defaults.starter_spirit_stone = json_int_value(defaults, "starter_spirit_stone", 80);
    m_defaults.starter_next_breakthrough_exp =
        json_int64_value(defaults, "starter_next_breakthrough_exp", 120);
    load_string_array(json_object_get(defaults, "starter_spell_ids"), &m_defaults.starter_spell_ids);
    load_string_array(json_object_get(defaults, "starter_recipe_ids"), &m_defaults.starter_recipe_ids);
    load_string_array(json_object_get(defaults, "realm_names"), &m_defaults.realm_names);

    if(auto* starter_inventory = json_object_get(defaults, "starter_inventory");
       starter_inventory != nullptr && json_is_array(starter_inventory))
    {
        const size_t count = json_array_size(starter_inventory);
        m_defaults.starter_inventory.reserve(count);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(starter_inventory, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudStarterInventoryItem config_item;
            config_item.item_id = json_string_field(item, "item_id");
            config_item.quantity = json_int_value(item, "quantity", 1);
            config_item.equipped = json_bool_value(item, "equipped", false);
            if(!config_item.item_id.empty() && config_item.quantity > 0)
            {
                m_defaults.starter_inventory.push_back(std::move(config_item));
            }
        }
    }

    if(auto* attribute_defaults = json_object_get(root, "attribute_defaults");
       attribute_defaults != nullptr && json_is_object(attribute_defaults))
    {
        m_default_base_attributes = load_base_attributes(json_object_get(attribute_defaults, "base_attributes"));
        m_default_status_attributes =
            load_status_attributes(json_object_get(attribute_defaults, "status_attributes"));
        m_default_combat_attributes =
            load_combat_attributes(json_object_get(attribute_defaults, "combat_attributes"));
    }

    if(auto* origins = json_object_get(root, "origins"); origins != nullptr && json_is_array(origins))
    {
        const size_t count = json_array_size(origins);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(origins, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudOriginConfig origin;
            origin.origin_id = json_string_field(item, "origin_id");
            origin.name = json_string_field(item, "name");
            origin.race_name = json_string_field(item, "race_name");
            origin.homeland = json_string_field(item, "homeland");
            origin.description = json_string_field(item, "description");
            origin.base_attributes = load_base_attributes(json_object_get(item, "base_attributes"));
            origin.starter_skill_id = json_string_field(item, "starter_skill_id");
            load_string_array(json_object_get(item, "starter_spell_ids"), &origin.starter_spell_ids);
            if(!origin.origin_id.empty())
            {
                m_origins.insert_or_assign(origin.origin_id, std::move(origin));
            }
        }
    }

    if(auto* items = json_object_get(root, "items"); items != nullptr && json_is_array(items))
    {
        const size_t count = json_array_size(items);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(items, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudItemConfig config_item;
            config_item.item_id = json_string_field(item, "item_id");
            config_item.name = json_string_field(item, "name");
            config_item.item_type = json_string_field(item, "item_type");
            config_item.description = json_string_field(item, "description");
            config_item.price = json_int_value(item, "price", 0);
            config_item.hp_restore = json_int_value(item, "hp_restore", 0);
            config_item.mana_restore = json_int_value(item, "mana_restore", 0);
            config_item.sen_restore = json_int_value(item, "sen_restore", 0);
            config_item.sta_restore = json_int_value(item, "sta_restore", 0);
            config_item.exp_gain = json_int64_value(item, "exp_gain", 0);
            config_item.skill_level_gain = json_int_value(item, "skill_level_gain", 0);
            config_item.attack_bonus = json_int_value(item, "attack_bonus", 0);
            config_item.defense_bonus = json_int_value(item, "defense_bonus", 0);
            config_item.spell_damage_bonus = json_int_value(item, "spell_damage_bonus", 0);
            config_item.spell_haste_bonus = json_int_value(item, "spell_haste_bonus", 0);
            config_item.consumable = json_bool_value(item, "consumable", false);
            config_item.equipable = json_bool_value(item, "equipable", false);
            config_item.codex_entry_id = json_string_field(item, "codex_entry_id");
            load_string_array(json_object_get(item, "tags"), &config_item.tags);
            if(!config_item.item_id.empty())
            {
                m_items.insert_or_assign(config_item.item_id, std::move(config_item));
            }
        }
    }

    if(auto* sects = json_object_get(root, "sects"); sects != nullptr && json_is_array(sects))
    {
        const size_t count = json_array_size(sects);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(sects, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudSectConfig sect;
            sect.sect_id = json_string_field(item, "sect_id");
            sect.name = json_string_field(item, "name");
            sect.rank_title = json_string_field(item, "rank_title");
            sect.join_scene_id = json_string_field(item, "join_scene_id");
            sect.join_npc_id = json_string_field(item, "join_npc_id");
            sect.description = json_string_field(item, "description");
            sect.codex_entry_id = json_string_field(item, "codex_entry_id");
            sect.joinable = json_bool_value(item, "joinable", false);
            if(!sect.sect_id.empty())
            {
                m_sects.insert_or_assign(sect.sect_id, std::move(sect));
            }
        }
    }

    if(auto* quests = json_object_get(root, "quests"); quests != nullptr && json_is_array(quests))
    {
        const size_t count = json_array_size(quests);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(quests, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudQuestConfig quest;
            quest.quest_id = json_string_field(item, "quest_id");
            quest.title = json_string_field(item, "title");
            quest.description = json_string_field(item, "description");
            quest.issuer_npc_id = json_string_field(item, "issuer_npc_id");
            quest.submit_npc_id = json_string_field(item, "submit_npc_id");
            quest.required_item_id = json_string_field(item, "required_item_id");
            quest.required_item_count = json_int_value(item, "required_item_count", 0);
            quest.reward_spirit_stone = json_int_value(item, "reward_spirit_stone", 0);
            quest.reward_exp = json_int64_value(item, "reward_exp", 0);
            quest.reward_item_id = json_string_field(item, "reward_item_id");
            quest.reward_item_count = json_int_value(item, "reward_item_count", 0);
            quest.reward_sect_id = json_string_field(item, "reward_sect_id");
            quest.chapter = json_string_field(item, "chapter");
            load_unlock_rules(json_object_get(item, "unlock_rules"), &quest.unlock_rules);
            if(!quest.quest_id.empty())
            {
                m_quests.insert_or_assign(quest.quest_id, std::move(quest));
            }
        }
    }

    if(auto* npcs = json_object_get(root, "npcs"); npcs != nullptr && json_is_array(npcs))
    {
        const size_t count = json_array_size(npcs);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(npcs, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudNpcConfig npc;
            npc.npc_id = json_string_field(item, "npc_id");
            npc.name = json_string_field(item, "name");
            npc.scene_id = json_string_field(item, "scene_id");
            npc.hint = json_string_field(item, "hint");
            npc.dialogue = json_string_field(item, "dialogue");
            npc.sect_offer_id = json_string_field(item, "sect_offer_id");
            npc.role = json_string_field(item, "role");
            npc.description = json_string_field(item, "description");
            npc.codex_entry_id = json_string_field(item, "codex_entry_id");
            load_string_array(json_object_get(item, "quest_ids"), &npc.quest_ids);
            if(!npc.npc_id.empty())
            {
                m_npcs.insert_or_assign(npc.npc_id, std::move(npc));
            }
        }
    }

    if(auto* monsters = json_object_get(root, "monsters"); monsters != nullptr && json_is_array(monsters))
    {
        const size_t count = json_array_size(monsters);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(monsters, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudMonsterConfig monster;
            monster.monster_id = json_string_field(item, "monster_id");
            monster.name = json_string_field(item, "name");
            monster.scene_id = json_string_field(item, "scene_id");
            monster.hp = json_int_value(item, "hp", 0);
            monster.attack = json_int_value(item, "attack", 0);
            monster.defense = json_int_value(item, "defense", 0);
            monster.reward_spirit_stone = json_int_value(item, "reward_spirit_stone", 0);
            monster.reward_exp = json_int64_value(item, "reward_exp", 0);
            monster.drop_item_id = json_string_field(item, "drop_item_id");
            monster.drop_item_count = json_int_value(item, "drop_item_count", 0);
            monster.description = json_string_field(item, "description");
            monster.kind = json_string_field(item, "kind");
            monster.element = json_string_field(item, "element");
            monster.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!monster.monster_id.empty())
            {
                m_monsters.insert_or_assign(monster.monster_id, std::move(monster));
            }
        }
    }

    if(auto* scenes = json_object_get(root, "scenes"); scenes != nullptr && json_is_array(scenes))
    {
        const size_t count = json_array_size(scenes);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(scenes, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudSceneConfig scene;
            scene.scene_id = json_string_field(item, "scene_id");
            scene.name = json_string_field(item, "name");
            scene.region_name = json_string_field(item, "region_name");
            scene.description = json_string_field(item, "description");
            scene.map_x = json_int_value(item, "map_x", 0);
            scene.map_y = json_int_value(item, "map_y", 0);
            scene.chapter = json_string_field(item, "chapter");
            scene.codex_entry_id = json_string_field(item, "codex_entry_id");
            load_string_map(json_object_get(item, "exits"), &scene.exits);
            load_string_array(json_object_get(item, "npc_ids"), &scene.npc_ids);
            load_string_array(json_object_get(item, "monster_ids"), &scene.monster_ids);
            load_string_array(json_object_get(item, "shop_item_ids"), &scene.shop_item_ids);
            load_string_array(json_object_get(item, "resource_node_ids"), &scene.resource_node_ids);
            load_string_array(json_object_get(item, "ground_loot_ids"), &scene.ground_loot_ids);
            load_string_array(json_object_get(item, "hazard_ids"), &scene.hazard_ids);
            load_string_array(json_object_get(item, "codex_entry_ids"), &scene.codex_entry_ids);
            if(!scene.scene_id.empty())
            {
                m_scenes.insert_or_assign(scene.scene_id, std::move(scene));
            }
        }
    }

    if(auto* skills = json_object_get(root, "skills"); skills != nullptr && json_is_array(skills))
    {
        const size_t count = json_array_size(skills);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(skills, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudSkillConfig skill;
            skill.skill_id = json_string_field(item, "skill_id");
            skill.name = json_string_field(item, "name");
            skill.category = json_string_field(item, "category");
            skill.description = json_string_field(item, "description");
            skill.governing_attribute = json_string_field(item, "governing_attribute");
            skill.starter = json_bool_value(item, "starter", false);
            skill.chapter = json_string_field(item, "chapter");
            skill.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!skill.skill_id.empty())
            {
                m_skills.insert_or_assign(skill.skill_id, std::move(skill));
            }
        }
    }

    if(auto* spells = json_object_get(root, "spells"); spells != nullptr && json_is_array(spells))
    {
        const size_t count = json_array_size(spells);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(spells, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudSpellConfig spell;
            spell.spell_id = json_string_field(item, "spell_id");
            spell.name = json_string_field(item, "name");
            spell.element = json_string_field(item, "element");
            spell.description = json_string_field(item, "description");
            spell.mana_cost = json_int_value(item, "mana_cost", 0);
            spell.power = json_int_value(item, "power", 0);
            spell.required_realm_stage = json_int_value(item, "required_realm_stage", 0);
            spell.granted_by_item_id = json_string_field(item, "granted_by_item_id");
            spell.chapter = json_string_field(item, "chapter");
            spell.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!spell.spell_id.empty())
            {
                m_spells.insert_or_assign(spell.spell_id, std::move(spell));
            }
        }
    }

    if(auto* recipes = json_object_get(root, "recipes"); recipes != nullptr && json_is_array(recipes))
    {
        const size_t count = json_array_size(recipes);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(recipes, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudRecipeConfig recipe;
            recipe.recipe_id = json_string_field(item, "recipe_id");
            recipe.name = json_string_field(item, "name");
            recipe.description = json_string_field(item, "description");
            recipe.result_item_id = json_string_field(item, "result_item_id");
            recipe.result_quantity = json_int_value(item, "result_quantity", 0);
            recipe.station_scene_id = json_string_field(item, "station_scene_id");
            recipe.npc_id = json_string_field(item, "npc_id");
            recipe.success_rate = json_double_value(item, "success_rate", 0.8);
            recipe.required_skill_id = json_string_field(item, "required_skill_id");
            recipe.chapter = json_string_field(item, "chapter");
            recipe.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(auto* ingredients = json_object_get(item, "ingredient_items");
               ingredients != nullptr && json_is_array(ingredients))
            {
                const size_t ingredient_count = json_array_size(ingredients);
                recipe.ingredient_items.reserve(ingredient_count);
                for(size_t ingredient_index = 0; ingredient_index < ingredient_count; ++ingredient_index)
                {
                    auto* ingredient_item = json_array_get(ingredients, ingredient_index);
                    if(ingredient_item == nullptr || !json_is_object(ingredient_item))
                    {
                        continue;
                    }

                    MudRecipeIngredient ingredient;
                    ingredient.item_id = json_string_field(ingredient_item, "item_id");
                    ingredient.quantity = json_int_value(ingredient_item, "quantity", 0);
                    if(!ingredient.item_id.empty() && ingredient.quantity > 0)
                    {
                        recipe.ingredient_items.push_back(std::move(ingredient));
                    }
                }
            }
            if(!recipe.recipe_id.empty())
            {
                m_recipes.insert_or_assign(recipe.recipe_id, std::move(recipe));
            }
        }
    }

    if(auto* treasures = json_object_get(root, "treasures"); treasures != nullptr && json_is_array(treasures))
    {
        const size_t count = json_array_size(treasures);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(treasures, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudTreasureConfig treasure;
            treasure.treasure_id = json_string_field(item, "treasure_id");
            treasure.name = json_string_field(item, "name");
            treasure.description = json_string_field(item, "description");
            treasure.effect_summary = json_string_field(item, "effect_summary");
            if(!treasure.treasure_id.empty())
            {
                m_treasures.insert_or_assign(treasure.treasure_id, std::move(treasure));
            }
        }
    }

    if(auto* formations = json_object_get(root, "formations"); formations != nullptr && json_is_array(formations))
    {
        const size_t count = json_array_size(formations);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(formations, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudFormationConfig formation;
            formation.formation_id = json_string_field(item, "formation_id");
            formation.name = json_string_field(item, "name");
            formation.description = json_string_field(item, "description");
            formation.scene_id = json_string_field(item, "scene_id");
            formation.effect_summary = json_string_field(item, "effect_summary");
            formation.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!formation.formation_id.empty())
            {
                m_formations.insert_or_assign(formation.formation_id, std::move(formation));
            }
        }
    }

    if(auto* resource_nodes = json_object_get(root, "resource_nodes");
       resource_nodes != nullptr && json_is_array(resource_nodes))
    {
        const size_t count = json_array_size(resource_nodes);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(resource_nodes, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudResourceNodeConfig node;
            node.node_id = json_string_field(item, "node_id");
            node.name = json_string_field(item, "name");
            node.scene_id = json_string_field(item, "scene_id");
            node.description = json_string_field(item, "description");
            node.drop_item_id = json_string_field(item, "drop_item_id");
            node.drop_item_count = json_int_value(item, "drop_item_count", 0);
            node.cooldown_ms = json_int_value(item, "cooldown_ms", 0);
            node.required_skill_id = json_string_field(item, "required_skill_id");
            node.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!node.node_id.empty())
            {
                m_resource_nodes.insert_or_assign(node.node_id, std::move(node));
            }
        }
    }

    if(auto* ground_loots = json_object_get(root, "ground_loots");
       ground_loots != nullptr && json_is_array(ground_loots))
    {
        const size_t count = json_array_size(ground_loots);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(ground_loots, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudGroundLootConfig loot;
            loot.loot_id = json_string_field(item, "loot_id");
            loot.scene_id = json_string_field(item, "scene_id");
            loot.item_id = json_string_field(item, "item_id");
            loot.quantity = json_int_value(item, "quantity", 0);
            loot.description = json_string_field(item, "description");
            loot.one_time = json_bool_value(item, "one_time", true);
            loot.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!loot.loot_id.empty())
            {
                m_ground_loots.insert_or_assign(loot.loot_id, std::move(loot));
            }
        }
    }

    if(auto* hazards = json_object_get(root, "hazards"); hazards != nullptr && json_is_array(hazards))
    {
        const size_t count = json_array_size(hazards);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(hazards, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudHazardConfig hazard;
            hazard.hazard_id = json_string_field(item, "hazard_id");
            hazard.scene_id = json_string_field(item, "scene_id");
            hazard.name = json_string_field(item, "name");
            hazard.description = json_string_field(item, "description");
            hazard.hp_cost = json_int_value(item, "hp_cost", 0);
            hazard.mana_cost = json_int_value(item, "mana_cost", 0);
            hazard.sta_cost = json_int_value(item, "sta_cost", 0);
            hazard.sen_cost = json_int_value(item, "sen_cost", 0);
            hazard.resist_key = json_string_field(item, "resist_key");
            hazard.codex_entry_id = json_string_field(item, "codex_entry_id");
            if(!hazard.hazard_id.empty())
            {
                m_hazards.insert_or_assign(hazard.hazard_id, std::move(hazard));
            }
        }
    }

    if(auto* codex_entries = json_object_get(root, "codex_entries");
       codex_entries != nullptr && json_is_array(codex_entries))
    {
        const size_t count = json_array_size(codex_entries);
        for(size_t index = 0; index < count; ++index)
        {
            auto* item = json_array_get(codex_entries, index);
            if(item == nullptr || !json_is_object(item))
            {
                continue;
            }

            MudCodexEntryConfig entry;
            entry.entry_id = json_string_field(item, "entry_id");
            entry.category = json_string_field(item, "category");
            entry.title = json_string_field(item, "title");
            entry.summary = json_string_field(item, "summary");
            entry.content = json_string_field(item, "content");
            load_string_array(json_object_get(item, "related_scene_ids"), &entry.related_scene_ids);
            load_string_array(json_object_get(item, "related_npc_ids"), &entry.related_npc_ids);
            load_string_array(json_object_get(item, "related_monster_ids"), &entry.related_monster_ids);
            load_string_array(json_object_get(item, "related_item_ids"), &entry.related_item_ids);
            load_string_array(json_object_get(item, "related_sect_ids"), &entry.related_sect_ids);
            load_unlock_rules(json_object_get(item, "unlock_rules"), &entry.unlock_rules);
            if(!entry.entry_id.empty())
            {
                for(const auto& rule : entry.unlock_rules)
                {
                    m_codex_unlock_index[rule.trigger + ":" + rule.target_id].push_back(entry.entry_id);
                }
                m_codex_entries.insert_or_assign(entry.entry_id, std::move(entry));
            }
        }
    }

    cleanup();

    if(m_defaults.starting_scene_id.empty() || m_scenes.find(m_defaults.starting_scene_id) == m_scenes.end())
    {
        if(error_message != nullptr)
        {
            *error_message = "starting_scene_id missing or invalid";
        }
        return false;
    }

    m_ready = true;
    return true;
}

bool MudWorld::ready() const
{
    return m_ready;
}

const MudWorldDefaults& MudWorld::defaults() const
{
    return m_defaults;
}

const MudBaseAttributeState& MudWorld::default_base_attributes() const
{
    return m_default_base_attributes;
}

const MudStatusAttributeState& MudWorld::default_status_attributes() const
{
    return m_default_status_attributes;
}

const MudCombatAttributeState& MudWorld::default_combat_attributes() const
{
    return m_default_combat_attributes;
}

std::vector<MudOriginConfig> MudWorld::origins() const
{
    std::vector<MudOriginConfig> result;
    result.reserve(m_origins.size());
    for(const auto& [id, origin] : m_origins)
    {
        result.push_back(origin);
    }
    std::sort(result.begin(), result.end(), [](const MudOriginConfig& lhs, const MudOriginConfig& rhs) {
        return lhs.name < rhs.name;
    });
    return result;
}

std::vector<MudCodexEntryConfig> MudWorld::codex_entries_for_category(const std::string& category) const
{
    std::vector<MudCodexEntryConfig> result;
    result.reserve(m_codex_entries.size());
    for(const auto& [id, entry] : m_codex_entries)
    {
        if(category.empty() || entry.category == category)
        {
            result.push_back(entry);
        }
    }
    std::sort(result.begin(), result.end(), [](const MudCodexEntryConfig& lhs, const MudCodexEntryConfig& rhs) {
        if(lhs.category == rhs.category)
        {
            return lhs.title < rhs.title;
        }
        return lhs.category < rhs.category;
    });
    return result;
}

std::vector<const MudCodexEntryConfig*> MudWorld::codex_entries_for_unlock(const std::string& trigger,
                                                                           const std::string& target_id) const
{
    std::vector<const MudCodexEntryConfig*> result;
    const auto key = trigger + ":" + target_id;
    if(auto iter = m_codex_unlock_index.find(key); iter != m_codex_unlock_index.end())
    {
        result.reserve(iter->second.size());
        for(const auto& entry_id : iter->second)
        {
            if(const auto* entry = find_codex_entry(entry_id); entry != nullptr)
            {
                result.push_back(entry);
            }
        }
    }
    return result;
}

const MudOriginConfig* MudWorld::find_origin(const std::string& origin_id) const
{
    return find_in_map(m_origins, origin_id);
}

const MudSceneConfig* MudWorld::find_scene(const std::string& scene_id) const
{
    return find_in_map(m_scenes, scene_id);
}

const MudNpcConfig* MudWorld::find_npc(const std::string& npc_id) const
{
    return find_in_map(m_npcs, npc_id);
}

const MudQuestConfig* MudWorld::find_quest(const std::string& quest_id) const
{
    return find_in_map(m_quests, quest_id);
}

const MudMonsterConfig* MudWorld::find_monster(const std::string& monster_id) const
{
    return find_in_map(m_monsters, monster_id);
}

const MudItemConfig* MudWorld::find_item(const std::string& item_id) const
{
    return find_in_map(m_items, item_id);
}

const MudSectConfig* MudWorld::find_sect(const std::string& sect_id) const
{
    return find_in_map(m_sects, sect_id);
}

const MudSkillConfig* MudWorld::find_skill(const std::string& skill_id) const
{
    return find_in_map(m_skills, skill_id);
}

const MudSpellConfig* MudWorld::find_spell(const std::string& spell_id) const
{
    return find_in_map(m_spells, spell_id);
}

const MudRecipeConfig* MudWorld::find_recipe(const std::string& recipe_id) const
{
    return find_in_map(m_recipes, recipe_id);
}

const MudTreasureConfig* MudWorld::find_treasure(const std::string& treasure_id) const
{
    return find_in_map(m_treasures, treasure_id);
}

const MudFormationConfig* MudWorld::find_formation(const std::string& formation_id) const
{
    return find_in_map(m_formations, formation_id);
}

const MudResourceNodeConfig* MudWorld::find_resource_node(const std::string& node_id) const
{
    return find_in_map(m_resource_nodes, node_id);
}

const MudGroundLootConfig* MudWorld::find_ground_loot(const std::string& loot_id) const
{
    return find_in_map(m_ground_loots, loot_id);
}

const MudHazardConfig* MudWorld::find_hazard(const std::string& hazard_id) const
{
    return find_in_map(m_hazards, hazard_id);
}

const MudCodexEntryConfig* MudWorld::find_codex_entry(const std::string& entry_id) const
{
    return find_in_map(m_codex_entries, entry_id);
}

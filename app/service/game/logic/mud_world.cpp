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

} // namespace

bool MudWorld::load_from_file(const std::string& path, std::string* error_message)
{
    m_ready = false;
    m_defaults = MudWorldDefaults{};
    m_scenes.clear();
    m_npcs.clear();
    m_quests.clear();
    m_monsters.clear();
    m_items.clear();
    m_sects.clear();

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
            config_item.exp_gain = json_int64_value(item, "exp_gain", 0);
            config_item.skill_level_gain = json_int_value(item, "skill_level_gain", 0);
            config_item.attack_bonus = json_int_value(item, "attack_bonus", 0);
            config_item.defense_bonus = json_int_value(item, "defense_bonus", 0);
            config_item.consumable = json_bool_value(item, "consumable", false);
            config_item.equipable = json_bool_value(item, "equipable", false);
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
            load_string_map(json_object_get(item, "exits"), &scene.exits);
            load_string_array(json_object_get(item, "npc_ids"), &scene.npc_ids);
            load_string_array(json_object_get(item, "monster_ids"), &scene.monster_ids);
            load_string_array(json_object_get(item, "shop_item_ids"), &scene.shop_item_ids);
            if(!scene.scene_id.empty())
            {
                m_scenes.insert_or_assign(scene.scene_id, std::move(scene));
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

const MudSceneConfig* MudWorld::find_scene(const std::string& scene_id) const
{
    if(auto iter = m_scenes.find(scene_id); iter != m_scenes.end())
    {
        return &iter->second;
    }
    return nullptr;
}

const MudNpcConfig* MudWorld::find_npc(const std::string& npc_id) const
{
    if(auto iter = m_npcs.find(npc_id); iter != m_npcs.end())
    {
        return &iter->second;
    }
    return nullptr;
}

const MudQuestConfig* MudWorld::find_quest(const std::string& quest_id) const
{
    if(auto iter = m_quests.find(quest_id); iter != m_quests.end())
    {
        return &iter->second;
    }
    return nullptr;
}

const MudMonsterConfig* MudWorld::find_monster(const std::string& monster_id) const
{
    if(auto iter = m_monsters.find(monster_id); iter != m_monsters.end())
    {
        return &iter->second;
    }
    return nullptr;
}

const MudItemConfig* MudWorld::find_item(const std::string& item_id) const
{
    if(auto iter = m_items.find(item_id); iter != m_items.end())
    {
        return &iter->second;
    }
    return nullptr;
}

const MudSectConfig* MudWorld::find_sect(const std::string& sect_id) const
{
    if(auto iter = m_sects.find(sect_id); iter != m_sects.end())
    {
        return &iter->second;
    }
    return nullptr;
}

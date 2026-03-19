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

#include "mud_types.h"

#include <string>
#include <unordered_map>
#include <vector>

class MudWorld
{
public:
    bool load_from_file(const std::string& path, std::string* error_message = nullptr);
    bool ready() const;

    const MudWorldDefaults& defaults() const;
    const MudBaseAttributeState& default_base_attributes() const;
    const MudStatusAttributeState& default_status_attributes() const;
    const MudCombatAttributeState& default_combat_attributes() const;
    std::vector<MudOriginConfig> origins() const;
    std::vector<MudCodexEntryConfig> codex_entries_for_category(const std::string& category) const;
    std::vector<const MudCodexEntryConfig*> codex_entries_for_unlock(const std::string& trigger,
                                                                     const std::string& target_id) const;

    const MudOriginConfig* find_origin(const std::string& origin_id) const;
    const MudSceneConfig* find_scene(const std::string& scene_id) const;
    const MudNpcConfig* find_npc(const std::string& npc_id) const;
    const MudQuestConfig* find_quest(const std::string& quest_id) const;
    const MudMonsterConfig* find_monster(const std::string& monster_id) const;
    const MudItemConfig* find_item(const std::string& item_id) const;
    const MudSectConfig* find_sect(const std::string& sect_id) const;
    const MudSkillConfig* find_skill(const std::string& skill_id) const;
    const MudSpellConfig* find_spell(const std::string& spell_id) const;
    const MudRecipeConfig* find_recipe(const std::string& recipe_id) const;
    const MudTreasureConfig* find_treasure(const std::string& treasure_id) const;
    const MudFormationConfig* find_formation(const std::string& formation_id) const;
    const MudResourceNodeConfig* find_resource_node(const std::string& node_id) const;
    const MudGroundLootConfig* find_ground_loot(const std::string& loot_id) const;
    const MudHazardConfig* find_hazard(const std::string& hazard_id) const;
    const MudCodexEntryConfig* find_codex_entry(const std::string& entry_id) const;

private:
    MudWorldDefaults m_defaults;
    MudBaseAttributeState m_default_base_attributes;
    MudStatusAttributeState m_default_status_attributes;
    MudCombatAttributeState m_default_combat_attributes;
    std::unordered_map<std::string, MudOriginConfig> m_origins;
    std::unordered_map<std::string, MudSceneConfig> m_scenes;
    std::unordered_map<std::string, MudNpcConfig> m_npcs;
    std::unordered_map<std::string, MudQuestConfig> m_quests;
    std::unordered_map<std::string, MudMonsterConfig> m_monsters;
    std::unordered_map<std::string, MudItemConfig> m_items;
    std::unordered_map<std::string, MudSectConfig> m_sects;
    std::unordered_map<std::string, MudSkillConfig> m_skills;
    std::unordered_map<std::string, MudSpellConfig> m_spells;
    std::unordered_map<std::string, MudRecipeConfig> m_recipes;
    std::unordered_map<std::string, MudTreasureConfig> m_treasures;
    std::unordered_map<std::string, MudFormationConfig> m_formations;
    std::unordered_map<std::string, MudResourceNodeConfig> m_resource_nodes;
    std::unordered_map<std::string, MudGroundLootConfig> m_ground_loots;
    std::unordered_map<std::string, MudHazardConfig> m_hazards;
    std::unordered_map<std::string, MudCodexEntryConfig> m_codex_entries;
    std::unordered_map<std::string, std::vector<std::string>> m_codex_unlock_index;
    bool m_ready = false;
};

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

class MudWorld
{
public:
    bool load_from_file(const std::string& path, std::string* error_message = nullptr);
    bool ready() const;

    const MudWorldDefaults& defaults() const;
    const MudSceneConfig* find_scene(const std::string& scene_id) const;
    const MudNpcConfig* find_npc(const std::string& npc_id) const;
    const MudQuestConfig* find_quest(const std::string& quest_id) const;
    const MudMonsterConfig* find_monster(const std::string& monster_id) const;
    const MudItemConfig* find_item(const std::string& item_id) const;
    const MudSectConfig* find_sect(const std::string& sect_id) const;

private:
    MudWorldDefaults m_defaults;
    std::unordered_map<std::string, MudSceneConfig> m_scenes;
    std::unordered_map<std::string, MudNpcConfig> m_npcs;
    std::unordered_map<std::string, MudQuestConfig> m_quests;
    std::unordered_map<std::string, MudMonsterConfig> m_monsters;
    std::unordered_map<std::string, MudItemConfig> m_items;
    std::unordered_map<std::string, MudSectConfig> m_sects;
    bool m_ready = false;
};

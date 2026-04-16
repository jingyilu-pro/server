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

#include "mud_game_runtime.h"

#include "http_code_message.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace
{

constexpr int64_t kScenePresenceTtlMs = 5 * 60 * 1000;

std::string join_strings(const std::vector<std::string>& values, const char* separator = "、")
{
    std::ostringstream output;
    for(size_t index = 0; index < values.size(); ++index)
    {
        if(index > 0)
        {
            output << separator;
        }
        output << values[index];
    }
    return output.str();
}

std::pair<std::string, std::string> parse_ask_topic_entry(const std::string& raw_entry)
{
    const auto entry = mud_trim(raw_entry);
    if(entry.empty())
    {
        return {};
    }

    static const std::vector<std::string> kSeparators = {"=>", "｜", "|"};
    for(const auto& separator : kSeparators)
    {
        const auto position = entry.find(separator);
        if(position == std::string::npos)
        {
            continue;
        }

        return {
            mud_trim(entry.substr(0, position)),
            mud_trim(entry.substr(position + separator.size())),
        };
    }

    return {entry, ""};
}

std::string ask_topic_label(const std::string& raw_entry)
{
    return parse_ask_topic_entry(raw_entry).first;
}

bool ask_topic_matches(const std::string& raw_entry,
                       const std::string& raw_query)
{
    const auto label = ask_topic_label(raw_entry);
    const auto query = mud_trim(raw_query);
    if(label.empty() || query.empty())
    {
        return false;
    }

    const auto label_lower = mud_to_lower_ascii(label);
    const auto query_lower = mud_to_lower_ascii(query);
    return label == query || label.find(query) != std::string::npos || query.find(label) != std::string::npos ||
           label_lower == query_lower || label_lower.find(query_lower) != std::string::npos ||
           query_lower.find(label_lower) != std::string::npos;
}

std::string generic_ask_response(const MudNpcConfig& npc,
                                 const std::string& topic_label)
{
    const auto topic = mud_trim(topic_label);
    if(topic.empty())
    {
        return npc.name + "只是看了你一眼，像是在等你问得更明白些。";
    }
    if(topic.find("路") != std::string::npos || topic.find("去") != std::string::npos)
    {
        return npc.name + "低声提醒：修行路上先认清脚下去路，再谈更远的机缘。";
    }
    if(topic.find("差") != std::string::npos || topic.find("事") != std::string::npos || topic.find("任务") != std::string::npos)
    {
        return npc.name + "淡淡道：能先把眼前这件事办稳的人，往往才能走得更远。";
    }
    if(topic.find("宗") != std::string::npos || topic.find("门") != std::string::npos)
    {
        return npc.name + "提起各家势力时神色平静，只说修行路终究要看你自己站得住。";
    }
    if(topic.find("海") != std::string::npos || topic.find("风暴") != std::string::npos)
    {
        return npc.name + "望向远处，像是想起海上那些不肯轻易示人的风声。";
    }
    if(topic.find("血禁") != std::string::npos || topic.find("禁地") != std::string::npos)
    {
        return npc.name + "声音压得极低：禁地里的东西值钱，可更值钱的是你还能活着出来。";
    }
    if(topic.find("虚天") != std::string::npos)
    {
        return npc.name + "沉默片刻，只说虚天之事从来不是听几句传闻就能说透的。";
    }
    if(!npc.hint.empty())
    {
        return npc.hint;
    }
    if(!npc.dialogue.empty())
    {
        return npc.dialogue;
    }
    return npc.name + "没有再多说，只让你自己去看、去听、去走一遭。";
}

bool is_about_keyword(const std::string& raw_value)
{
    const auto trimmed = mud_trim(raw_value);
    const auto value = mud_to_lower_ascii(trimmed);
    return value == "about" || trimmed == "关于";
}

bool is_rumor_keyword(const std::string& raw_value)
{
    const auto trimmed = mud_trim(raw_value);
    const auto value = mud_to_lower_ascii(trimmed);
    return value == "rumor" || value == "rumours" || trimmed == "风声" || trimmed == "传闻" ||
           trimmed == "消息" || trimmed == "差事";
}

std::pair<std::string, std::string> parse_board_post_title(const std::string& title)
{
    const auto separator = title.find('|');
    if(separator == std::string::npos)
    {
        return {"", mud_trim(title)};
    }
    return {mud_trim(title.substr(0, separator)), mud_trim(title.substr(separator + 1))};
}

bool is_utf8_continuation_byte(unsigned char value)
{
    return (value & 0xC0U) == 0x80U;
}

std::string utf8_safe_prefix(const std::string& text,
                             std::size_t max_bytes)
{
    if(text.size() <= max_bytes)
    {
        return text;
    }
    std::size_t cut = std::min(max_bytes, text.size());
    while(cut > 0 && cut < text.size() && is_utf8_continuation_byte(static_cast<unsigned char>(text[cut])))
    {
        --cut;
    }
    return text.substr(0, cut);
}

std::string compact_preview(const std::string& text, std::size_t limit = 36)
{
    const auto value = mud_trim(text);
    if(value.size() <= limit)
    {
        return value;
    }
    if(limit <= 1)
    {
        return utf8_safe_prefix(value, limit);
    }
    return utf8_safe_prefix(value, limit - 1) + "…";
}

bool same_event_payload(const MudEventEnvelope& lhs, const MudEventEnvelope& rhs)
{
    return lhs.target_account == rhs.target_account && lhs.type == rhs.type && lhs.title == rhs.title &&
           lhs.content == rhs.content && lhs.server_time_ms == rhs.server_time_ms;
}

std::vector<std::string> split_multiline_text(const std::string& text)
{
    std::vector<std::string> lines;
    std::istringstream input(text);
    for(std::string line; std::getline(input, line);)
    {
        auto trimmed = mud_trim(line);
        if(!trimmed.empty())
        {
            lines.push_back(std::move(trimmed));
        }
    }
    if(lines.empty())
    {
        const auto trimmed = mud_trim(text);
        if(!trimmed.empty())
        {
            lines.push_back(trimmed);
        }
    }
    return lines;
}

bool quest_states_equal(const std::vector<MudQuestState>& lhs,
                        const std::vector<MudQuestState>& rhs)
{
    if(lhs.size() != rhs.size())
    {
        return false;
    }
    for(size_t index = 0; index < lhs.size(); ++index)
    {
        if(lhs[index].quest_id != rhs[index].quest_id || lhs[index].status != rhs[index].status ||
           lhs[index].progress != rhs[index].progress)
        {
            return false;
        }
    }
    return true;
}

bool status_attributes_equal(const MudStatusAttributeState& lhs,
                             const MudStatusAttributeState& rhs)
{
    return lhs.kee == rhs.kee && lhs.sen == rhs.sen && lhs.sta == rhs.sta && lhs.mana == rhs.mana;
}

bool combat_attributes_equal(const MudCombatAttributeState& lhs,
                             const MudCombatAttributeState& rhs)
{
    return lhs.phys_hit == rhs.phys_hit && lhs.phys_crit == rhs.phys_crit &&
           lhs.phys_damage == rhs.phys_damage && lhs.phys_haste == rhs.phys_haste &&
           lhs.spell_hit == rhs.spell_hit && lhs.spell_crit == rhs.spell_crit &&
           lhs.spell_damage == rhs.spell_damage && lhs.spell_haste == rhs.spell_haste &&
           lhs.dodge == rhs.dodge && lhs.block == rhs.block && lhs.shield == rhs.shield &&
           lhs.parry == rhs.parry && lhs.armor == rhs.armor && lhs.resist_fire == rhs.resist_fire &&
           lhs.resist_ice == rhs.resist_ice && lhs.resist_thunder == rhs.resist_thunder &&
           lhs.resist_wind == rhs.resist_wind && lhs.resist_corrosion == rhs.resist_corrosion &&
           lhs.resist_poison == rhs.resist_poison && lhs.resist_pierce == rhs.resist_pierce &&
           lhs.resist_slash == rhs.resist_slash && lhs.resist_blunt == rhs.resist_blunt;
}

std::string normalize_direction(std::string value)
{
    value = mud_to_lower_ascii(mud_trim(value));
    if(value == "n" || value == "north" || value == "北")
    {
        return "north";
    }
    if(value == "s" || value == "south" || value == "南")
    {
        return "south";
    }
    if(value == "e" || value == "east" || value == "东")
    {
        return "east";
    }
    if(value == "w" || value == "west" || value == "西")
    {
        return "west";
    }
    if(value == "up" || value == "u" || value == "上")
    {
        return "up";
    }
    if(value == "down" || value == "d" || value == "下")
    {
        return "down";
    }
    return value;
}

std::string direction_display_name(const std::string& value)
{
    const auto normalized = normalize_direction(value);
    if(normalized == "north")
    {
        return "北方";
    }
    if(normalized == "south")
    {
        return "南方";
    }
    if(normalized == "east")
    {
        return "东方";
    }
    if(normalized == "west")
    {
        return "西方";
    }
    if(normalized == "up")
    {
        return "上方";
    }
    if(normalized == "down")
    {
        return "下方";
    }
    return value.empty() ? std::string("未知方位") : value;
}

std::string item_display_name(const MudWorld* world,
                              const std::string& item_id,
                              const char* fallback = "未知物品")
{
    if(world != nullptr)
    {
        if(const auto* item = world->find_item(item_id); item != nullptr && !item->name.empty())
        {
            return item->name;
        }
    }
    return fallback == nullptr ? std::string() : std::string(fallback);
}

std::string item_with_count_label(const MudWorld* world,
                                  const std::string& item_id,
                                  int count,
                                  const char* fallback = "未知物品")
{
    return item_display_name(world, item_id, fallback) + " x" + std::to_string(count);
}

std::string quest_status_display_name(const std::string& status)
{
    const auto normalized = mud_to_lower_ascii(mud_trim(status));
    if(normalized == "active")
    {
        return "进行中";
    }
    if(normalized == "completed")
    {
        return "已完成";
    }
    if(normalized == "submitted")
    {
        return "已交付";
    }
    if(normalized == "ready")
    {
        return "可交付";
    }
    if(normalized == "available")
    {
        return "可接取";
    }
    return status.empty() ? std::string("未定") : status;
}

std::string item_type_display_name(const std::string& item_type)
{
    const auto normalized = mud_to_lower_ascii(mud_trim(item_type));
    if(normalized.empty())
    {
        return "杂物";
    }
    if(normalized == "consumable")
    {
        return "丹药";
    }
    if(normalized == "weapon")
    {
        return "兵刃";
    }
    if(normalized == "armor")
    {
        return "护具";
    }
    if(normalized == "accessory")
    {
        return "饰物";
    }
    if(normalized == "material")
    {
        return "材料";
    }
    if(normalized == "treasure")
    {
        return "宝物";
    }
    if(normalized == "book")
    {
        return "典籍";
    }
    if(normalized == "recipe")
    {
        return "配方";
    }
    if(normalized == "quest")
    {
        return "任务物";
    }
    if(normalized == "tool")
    {
        return "器具";
    }
    return item_type;
}

std::vector<std::string> split_dense_sentences(const std::string& text, size_t limit = 4)
{
    const auto normalized = mud_trim(text);
    if(normalized.empty())
    {
        return {};
    }

    std::vector<std::string> parts;
    std::string remaining = normalized;
    const std::vector<std::string> delimiters = {"。", "！", "？", "；"};
    while(!remaining.empty() && parts.size() < limit)
    {
        size_t best_pos = std::string::npos;
        size_t best_len = 0;
        for(const auto& delimiter : delimiters)
        {
            const auto pos = remaining.find(delimiter);
            if(pos != std::string::npos && (best_pos == std::string::npos || pos < best_pos))
            {
                best_pos = pos;
                best_len = delimiter.size();
            }
        }

        if(best_pos == std::string::npos)
        {
            parts.push_back(mud_trim(remaining));
            break;
        }

        parts.push_back(mud_trim(remaining.substr(0, best_pos + best_len)));
        remaining = mud_trim(remaining.substr(best_pos + best_len));
    }
    if(parts.empty())
    {
        parts.push_back(normalized);
    }
    return parts;
}

std::string first_dense_sentence(const std::string& text, const std::string& fallback = "")
{
    const auto parts = split_dense_sentences(text, 1);
    if(!parts.empty() && !parts.front().empty())
    {
        return parts.front();
    }
    return fallback;
}

std::string scene_palette_id(const MudSceneConfig& scene)
{
    if(!scene.palette_id.empty())
    {
        return scene.palette_id;
    }
    const auto region = mud_to_lower_ascii(scene.region_name + " " + scene.name);
    if(region.find("乱星") != std::string::npos || region.find("海") != std::string::npos || region.find("港") != std::string::npos)
    {
        return "sea_cold_ink";
    }
    if(region.find("血") != std::string::npos || region.find("禁") != std::string::npos)
    {
        return "forbidden_cinnabar";
    }
    if(region.find("虚天") != std::string::npos || region.find("残") != std::string::npos)
    {
        return "void_stone_blue";
    }
    if(region.find("黄枫") != std::string::npos)
    {
        return "autumn_olive";
    }
    return "warm_ink_scroll";
}

std::string scene_ambient_mood(const MudSceneConfig& scene)
{
    if(!scene.ambient_mood.empty())
    {
        return scene.ambient_mood;
    }
    if(scene.risk_level == "high")
    {
        return "杀机隐伏";
    }
    if(scene.risk_level == "mid")
    {
        return "风声微紧";
    }
    if(scene.room_type == "harbor" || scene.room_type == "sea")
    {
        return "潮声压耳";
    }
    return "灯影微温";
}

std::string event_channel_name(const MudEventEnvelope& event)
{
    const auto normalized = mud_to_lower_ascii(event.type);
    if(normalized == "chat")
    {
        const auto title = mud_to_lower_ascii(event.title);
        if(title.find("[team]") != std::string::npos)
        {
            return "team";
        }
        if(title.find("[tell") != std::string::npos)
        {
            return "tell";
        }
        if(title.find("[local]") != std::string::npos)
        {
            return "local";
        }
        return "world";
    }
    if(normalized == "combat" || normalized == "fight" || normalized == "spell")
    {
        return "combat";
    }
    if(normalized == "board_post")
    {
        return "board";
    }
    if(normalized == "quest" || normalized == "join")
    {
        return "quest";
    }
    if(normalized == "harvest" || normalized == "loot" || normalized == "brew" || normalized == "inventory")
    {
        return "system";
    }
    return "system";
}

std::string event_tone_name(const MudEventEnvelope& event)
{
    const auto channel = event_channel_name(event);
    if(channel == "world" || channel == "team" || channel == "tell" || channel == "local")
    {
        return "chat";
    }
    if(channel == "combat")
    {
        return "combat";
    }
    if(channel == "quest")
    {
        return "quest";
    }
    if(channel == "board")
    {
        return "hint";
    }
    if(event.type == "harvest" || event.type == "loot")
    {
        return "hint";
    }
    return "system";
}

std::string panel_render_mode_for_id(const std::string& panel_id)
{
    const auto normalized = mud_to_lower_ascii(mud_trim(panel_id));
    if(normalized == "map")
    {
        return "ascii_map";
    }
    if(normalized == "score" || normalized == "family" || normalized == "inspect")
    {
        return "dossier_block";
    }
    if(normalized == "help" || normalized == "commands" || normalized == "read")
    {
        return "notice_block";
    }
    if(normalized == "who")
    {
        return "roster_block";
    }
    return "board_block";
}

std::string panel_style_for_id(const std::string& panel_id)
{
    const auto normalized = mud_to_lower_ascii(mud_trim(panel_id));
    if(normalized == "map")
    {
        return "scroll-map";
    }
    if(normalized == "rank")
    {
        return "jianghu-board";
    }
    if(normalized == "help" || normalized == "commands" || normalized == "read")
    {
        return "mud-manual";
    }
    if(normalized == "journal" || normalized == "tasks")
    {
        return "travel-journal";
    }
    return "mud-tablet";
}

int flag_int_value(const MudPlayerState& player, const std::string& key, int default_value)
{
    if(auto iter = player.flags.find(key); iter != player.flags.end())
    {
        try
        {
            return std::stoi(iter->second);
        }
        catch(...)
        {
            return default_value;
        }
    }
    return default_value;
}

int64_t flag_int64_value(const MudPlayerState& player, const std::string& key, int64_t default_value)
{
    if(auto iter = player.flags.find(key); iter != player.flags.end())
    {
        try
        {
            return std::stoll(iter->second);
        }
        catch(...)
        {
            return default_value;
        }
    }
    return default_value;
}

void set_flag_int(MudPlayerState* player, const std::string& key, int value)
{
    if(player == nullptr || key.empty())
    {
        return;
    }
    player->flags[key] = std::to_string(value);
}

void set_flag_int64(MudPlayerState* player, const std::string& key, int64_t value)
{
    if(player == nullptr || key.empty())
    {
        return;
    }
    player->flags[key] = std::to_string(value);
}

void clear_flag(MudPlayerState* player, const std::string& key)
{
    if(player == nullptr || key.empty())
    {
        return;
    }
    player->flags.erase(key);
}

std::string monster_damage_flag_key(const MudPlayerState& player, const MudMonsterConfig& monster)
{
    return "monster_damage:" + player.location_scene_id + ":" + monster.monster_id;
}

bool scene_has_npc(const MudSceneConfig* scene, const std::string& npc_id)
{
    return scene != nullptr &&
           std::find(scene->npc_ids.begin(), scene->npc_ids.end(), npc_id) != scene->npc_ids.end();
}

bool quest_key_matches(const MudQuestConfig& quest, const std::string& normalized_key)
{
    return normalized_key.empty() || mud_to_lower_ascii(quest.quest_id) == normalized_key ||
           mud_to_lower_ascii(quest.title) == normalized_key;
}

bool has_loop_tag(const MudSceneConfig* scene, const std::string& tag)
{
    return scene != nullptr &&
           std::find(scene->loop_tags.begin(), scene->loop_tags.end(), tag) != scene->loop_tags.end();
}

} // namespace

MudGameRuntime::MudGameRuntime(std::shared_ptr<MudWorld> world,
                               std::shared_ptr<IMudPlayerRepository> repository)
    : m_world(std::move(world)),
      m_repository(std::move(repository))
{
    if(m_world == nullptr || !m_world->ready())
    {
        m_ready_error = "mud world not ready";
        return;
    }
    if(m_repository == nullptr || !m_repository->ready())
    {
        m_ready_error = "mud player repository not ready";
        return;
    }
}

bool MudGameRuntime::ready() const
{
    return m_ready_error.empty();
}

std::string MudGameRuntime::ready_error() const
{
    return m_ready_error;
}

void MudGameRuntime::poll()
{
    if(m_repository)
    {
        m_repository->poll();
    }
    prune_scene_presence();
    maybe_emit_world_event();
}

bool MudGameRuntime::verify_account_match(const std::string& jwt_account,
                                          const std::string& requested_account,
                                          std::string* resolved_account) const
{
    std::string account = requested_account.empty() ? jwt_account : requested_account;
    if(account.empty())
    {
        return false;
    }
    if(!jwt_account.empty() && !requested_account.empty() && jwt_account != requested_account)
    {
        return false;
    }
    if(resolved_account != nullptr)
    {
        *resolved_account = account;
    }
    return true;
}

MudPlayerState MudGameRuntime::build_default_player(const std::string& account,
                                                    const std::string& character_name,
                                                    const std::string& origin_id,
                                                    const std::string& background_id) const
{
    return make_default_player(account, character_name, origin_id, background_id);
}

MudPlayerState MudGameRuntime::make_default_player(const std::string& account,
                                                   const std::string& character_name,
                                                   const std::string& origin_id,
                                                   const std::string& background_id) const
{
    std::string resolved_background_id = background_id;
    if(resolved_background_id.empty())
    {
        const auto backgrounds = m_world->backgrounds();
        if(!backgrounds.empty())
        {
            resolved_background_id = backgrounds.front().background_id;
        }
    }

    MudPlayerState player;
    player.account = account;
    player.character_name = character_name;
    player.level = 1;
    player.hp = m_world->defaults().starter_hp;
    player.max_hp = m_world->defaults().starter_hp;
    player.attack_power = m_world->defaults().starter_attack;
    player.defense_power = m_world->defaults().starter_defense;
    player.spirit_stone = m_world->defaults().starter_spirit_stone;
    player.title = m_world->defaults().starter_title;
    player.location_scene_id = m_world->defaults().starting_scene_id;
    player.realm_name = m_world->defaults().starter_realm_name;
    player.realm_stage = m_world->defaults().starter_realm_stage;
    player.exp = 0;
    player.next_breakthrough_exp = m_world->defaults().starter_next_breakthrough_exp;
    player.primary_skill = m_world->defaults().starter_skill;
    player.skill_level = 1;
    player.origin_id = origin_id;
    player.background_id = resolved_background_id;
    for(const auto& starter_item : m_world->defaults().starter_inventory)
    {
        add_inventory_item(&player, starter_item.item_id, starter_item.quantity, starter_item.equipped);
    }
    if(const auto* background = m_world->find_background(resolved_background_id); background != nullptr)
    {
        for(const auto& starter_item : background->starter_inventory)
        {
            add_inventory_item(&player, starter_item.item_id, starter_item.quantity, starter_item.equipped);
        }
    }
    for(const auto& starter_spell_id : m_world->defaults().starter_spell_ids)
    {
        player.spells.push_back(MudSpellState{starter_spell_id, 1, 0, true});
    }
    for(const auto& starter_recipe_id : m_world->defaults().starter_recipe_ids)
    {
        player.recipes.push_back(MudRecipeState{starter_recipe_id, 1, 0, true});
    }
    set_flag_int64(&player, "created_at_s", mud_now_ms() / 1000);
    set_flag_int(&player, "foundation_qihai", 0);
    set_flag_int(&player, "loose_reputation", 0);
    set_flag_int64(&player, "pvp_opt_in_until_s", 0);
    normalize_player_state(&player);
    MudCommandExecution unlocks;
    unlock_codex_by_trigger(&player, "enter_scene", player.location_scene_id, &unlocks);
    refresh_quest_progress(&player);
    return player;
}

void MudGameRuntime::fill_origin_state(const MudPlayerState& player,
                                       mud::RaceState* state) const
{
    if(state == nullptr)
    {
        return;
    }
    state->set_origin_id(player.origin_id);
    state->set_origin_name(player.origin_name);
    state->set_race_name(player.race_name);
    state->set_homeland(player.homeland);
    if(const auto* origin = m_world->find_origin(player.origin_id); origin != nullptr)
    {
        state->set_description(origin->description);
    }
}

void MudGameRuntime::fill_background_state(const MudPlayerState& player,
                                           mud::BackgroundState* state) const
{
    if(state == nullptr)
    {
        return;
    }

    state->set_background_id(player.background_id);
    state->set_name(player.background_name);
    if(const auto* background = m_world->find_background(player.background_id); background != nullptr)
    {
        state->set_description(background->description);
        state->set_starter_title(background->starter_title);
        state->set_focus_label(background->focus_label);
    }
}

void MudGameRuntime::fill_base_attributes(const MudBaseAttributeState& state,
                                          mud::BaseAttributeState* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_spi(state.spi);
    output->set_gin(state.gin);
    output->set_str(state.str);
    output->set_per(state.per);
    output->set_int_attr(state.int_attr);
    output->set_cha(state.cha);
    output->set_luc(state.luc);
}

void MudGameRuntime::fill_status_attributes(const MudStatusAttributeState& state,
                                            mud::StatusAttributeState* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_kee(state.kee);
    output->set_sen(state.sen);
    output->set_sta(state.sta);
    output->set_mana(state.mana);
}

void MudGameRuntime::fill_combat_attributes(const MudCombatAttributeState& state,
                                            mud::CombatAttributeState* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_phys_hit(state.phys_hit);
    output->set_phys_crit(state.phys_crit);
    output->set_phys_damage(state.phys_damage);
    output->set_phys_haste(state.phys_haste);
    output->set_spell_hit(state.spell_hit);
    output->set_spell_crit(state.spell_crit);
    output->set_spell_damage(state.spell_damage);
    output->set_spell_haste(state.spell_haste);
    output->set_dodge(state.dodge);
    output->set_block(state.block);
    output->set_shield(state.shield);
    output->set_parry(state.parry);
    output->set_armor(state.armor);
    output->set_resist_fire(state.resist_fire);
    output->set_resist_ice(state.resist_ice);
    output->set_resist_thunder(state.resist_thunder);
    output->set_resist_wind(state.resist_wind);
    output->set_resist_corrosion(state.resist_corrosion);
    output->set_resist_poison(state.resist_poison);
    output->set_resist_pierce(state.resist_pierce);
    output->set_resist_slash(state.resist_slash);
    output->set_resist_blunt(state.resist_blunt);
}

void MudGameRuntime::fill_summary_entry(const MudSummaryEntry& entry,
                                        mud::SummaryEntry* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_entry_id(entry.entry_id);
    output->set_title(entry.title);
    output->set_summary(entry.summary);
    output->set_status(entry.status);
    output->set_category(entry.category);
    output->set_command(entry.command);
    output->set_location_hint(entry.location_hint);
    output->set_reward_summary(entry.reward_summary);
}

void MudGameRuntime::fill_structured_panel(const MudStructuredPanelState& panel,
                                           mud::StructuredPanel* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_panel_id(panel.panel_id);
    output->set_title(panel.title);
    output->set_summary(panel.summary);
    output->set_render_mode(panel.render_mode.empty() ? panel_render_mode_for_id(panel.panel_id) : panel.render_mode);
    output->set_style_id(panel.style_id.empty() ? panel_style_for_id(panel.panel_id) : panel.style_id);
    output->set_compact_title(panel.compact_title.empty() ? panel.title : panel.compact_title);
    output->set_document_id(panel.document_id);
    output->set_panel_kind(panel.panel_kind);
    output->clear_entries();
    output->clear_ascii_lines();
    output->clear_body_lines();
    output->clear_inline_commands();
    for(const auto& entry : panel.entries)
    {
        fill_summary_entry(entry, output->add_entries());
        if(panel.ascii_lines.empty())
        {
            std::string line = entry.title;
            if(!entry.status.empty())
            {
                line += "〔" + entry.status + "〕";
            }
            if(!entry.summary.empty())
            {
                line += " " + entry.summary;
            }
            output->add_ascii_lines(line);
        }
        if(panel.inline_commands.empty() && !entry.command.empty())
        {
            output->add_inline_commands(entry.command);
        }
    }
    for(const auto& line : panel.ascii_lines)
    {
        output->add_ascii_lines(line);
    }
    for(const auto& line : panel.body_lines)
    {
        output->add_body_lines(line);
    }
    for(const auto& command : panel.inline_commands)
    {
        output->add_inline_commands(command);
    }
}

void MudGameRuntime::fill_route_summary(const MudRouteSummaryState& route,
                                        mud::RouteSummary* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_route_id(route.route_id);
    output->set_title(route.title);
    output->set_status(route.status);
    output->set_summary(route.summary);
    output->set_next_step(route.next_step);
}

void MudGameRuntime::fill_weekly_event_summary(const MudWeeklyEventSummaryState& event,
                                               mud::WeeklyEventSummary* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_event_id(event.event_id);
    output->set_title(event.title);
    output->set_summary(event.summary);
    output->set_risk_level(event.risk_level);
    output->set_location_hint(event.location_hint);
    output->set_command_hint(event.command_hint);
}

void MudGameRuntime::sync_origin_from_world(MudPlayerState* player) const
{
    if(player == nullptr)
    {
        return;
    }

    const MudOriginConfig* origin = nullptr;
    if(!player->origin_id.empty())
    {
        origin = m_world->find_origin(player->origin_id);
    }
    if(origin == nullptr)
    {
        const auto origins = m_world->origins();
        if(!origins.empty())
        {
            origin = m_world->find_origin(origins.front().origin_id);
        }
    }
    if(origin == nullptr)
    {
        return;
    }

    player->origin_id = origin->origin_id;
    player->origin_name = origin->name;
    player->race_name = origin->race_name;
    player->homeland = origin->homeland;
    if(player->base_attributes.spi == 0 && player->base_attributes.gin == 0 && player->base_attributes.str == 0)
    {
        player->base_attributes = origin->base_attributes;
    }
    if(player->primary_skill.empty())
    {
        if(const auto* skill = m_world->find_skill(origin->starter_skill_id); skill != nullptr)
        {
            player->primary_skill = skill->name;
        }
    }
}

void MudGameRuntime::sync_background_from_world(MudPlayerState* player) const
{
    if(player == nullptr)
    {
        return;
    }

    const MudBackgroundConfig* background = nullptr;
    if(!player->background_id.empty())
    {
        background = m_world->find_background(player->background_id);
    }
    if(background == nullptr)
    {
        const auto backgrounds = m_world->backgrounds();
        if(!backgrounds.empty())
        {
            background = m_world->find_background(backgrounds.front().background_id);
        }
    }
    if(background == nullptr)
    {
        return;
    }

    player->background_id = background->background_id;
    player->background_name = background->name;
    if(player->title.empty() && !background->starter_title.empty())
    {
        player->title = background->starter_title;
    }
}

void MudGameRuntime::derive_player_combat_state(MudPlayerState* player) const
{
    if(player == nullptr)
    {
        return;
    }

    const auto& default_status = m_world->default_status_attributes();
    const auto& default_combat = m_world->default_combat_attributes();
    player->status_attributes.kee = std::max(default_status.kee,
                                             80 + player->base_attributes.str * 6 + player->base_attributes.gin * 4 +
                                                 player->realm_stage * 16);
    player->status_attributes.sen = std::max(default_status.sen,
                                             40 + player->base_attributes.spi * 6 + player->realm_stage * 8);
    player->status_attributes.sta = std::max(default_status.sta,
                                             50 + player->base_attributes.str * 5 + player->base_attributes.per * 3);
    player->status_attributes.mana = std::max(default_status.mana,
                                              30 + player->base_attributes.spi * 5 + player->base_attributes.int_attr * 4 +
                                                  player->realm_stage * 10);

    player->combat_attributes = default_combat;
    player->combat_attributes.phys_hit += player->base_attributes.per + player->skill_level;
    player->combat_attributes.phys_damage += player->base_attributes.str * 2 + player->realm_stage * 4;
    player->combat_attributes.phys_crit += player->base_attributes.luc / 2;
    player->combat_attributes.spell_hit += player->base_attributes.spi + player->realm_stage * 2;
    player->combat_attributes.spell_damage += player->base_attributes.int_attr * 2 + player->realm_stage * 5;
    player->combat_attributes.spell_crit += player->base_attributes.luc / 2;
    player->combat_attributes.dodge += player->base_attributes.per / 2;
    player->combat_attributes.armor += player->base_attributes.str + player->realm_stage * 3;

    player->max_hp = player->status_attributes.kee;
    player->hp = std::clamp(player->hp <= 0 ? player->max_hp : player->hp, 1, player->max_hp);
    player->attack_power = player->combat_attributes.phys_damage;
    player->defense_power = player->combat_attributes.armor;
}

bool MudGameRuntime::normalize_player_state(MudPlayerState* player) const
{
    if(player == nullptr)
    {
        return false;
    }

    bool dirty = false;
    const auto before_origin_id = player->origin_id;
    const auto before_background_id = player->background_id;
    const auto before_quests = player->quests;
    const auto before_status = player->status_attributes;
    const auto before_combat = player->combat_attributes;
    const auto before_hp = player->hp;
    const auto before_max_hp = player->max_hp;
    const auto before_attack_power = player->attack_power;
    const auto before_defense_power = player->defense_power;
    sync_origin_from_world(player);
    sync_background_from_world(player);
    dirty = dirty || before_origin_id != player->origin_id || before_background_id != player->background_id;

    if(player->base_attributes.spi == 0 && player->base_attributes.gin == 0 &&
       player->base_attributes.str == 0 && player->base_attributes.per == 0 &&
       player->base_attributes.int_attr == 0)
    {
        player->base_attributes = m_world->default_base_attributes();
        if(const auto* origin = m_world->find_origin(player->origin_id); origin != nullptr)
        {
            player->base_attributes = origin->base_attributes;
        }
        if(const auto* background = m_world->find_background(player->background_id); background != nullptr)
        {
            player->base_attributes.spi += background->attribute_bonus.spi;
            player->base_attributes.gin += background->attribute_bonus.gin;
            player->base_attributes.str += background->attribute_bonus.str;
            player->base_attributes.per += background->attribute_bonus.per;
            player->base_attributes.int_attr += background->attribute_bonus.int_attr;
            player->base_attributes.cha += background->attribute_bonus.cha;
            player->base_attributes.luc += background->attribute_bonus.luc;
        }
        dirty = true;
    }

    if(player->skills.empty())
    {
        MudSkillState skill_state;
        skill_state.skill_id = "long_spring_art";
        skill_state.level = std::max(1, player->skill_level);
        skill_state.proficiency = player->exp;
        player->skills.push_back(std::move(skill_state));
        dirty = true;
    }

    if(player->spells.empty())
    {
        for(const auto& starter_spell_id : m_world->defaults().starter_spell_ids)
        {
            player->spells.push_back(MudSpellState{starter_spell_id, 1, 0, true});
        }
        dirty = true;
    }

    if(player->recipes.empty())
    {
        for(const auto& starter_recipe_id : m_world->defaults().starter_recipe_ids)
        {
            player->recipes.push_back(MudRecipeState{starter_recipe_id, 1, 0, true});
        }
        dirty = true;
    }

    if(player->profession.alchemy_level <= 0)
    {
        player->profession.alchemy_level = 1;
        player->profession.exploration_level = 1;
        player->profession.formation_level = 1;
        player->profession.forging_level = 1;
        dirty = true;
    }

    derive_player_combat_state(player);
    refresh_quest_progress(player);
    dirty = dirty || !quest_states_equal(before_quests, player->quests) ||
            !status_attributes_equal(before_status, player->status_attributes) ||
            !combat_attributes_equal(before_combat, player->combat_attributes) || before_hp != player->hp ||
            before_max_hp != player->max_hp || before_attack_power != player->attack_power ||
            before_defense_power != player->defense_power;
    if(player->flags.find("current_mana") == player->flags.end())
    {
        set_flag_int(player, "current_mana", player->status_attributes.mana);
        dirty = true;
    }
    if(player->flags.find("current_sen") == player->flags.end())
    {
        set_flag_int(player, "current_sen", player->status_attributes.sen);
        dirty = true;
    }
    if(player->flags.find("current_sta") == player->flags.end())
    {
        set_flag_int(player, "current_sta", player->status_attributes.sta);
        dirty = true;
    }
    if(player->flags.find("created_at_s") == player->flags.end())
    {
        set_flag_int64(player, "created_at_s", 1);
        dirty = true;
    }
    if(player->flags.find("pvp_opt_in_until_s") == player->flags.end())
    {
        set_flag_int64(player, "pvp_opt_in_until_s", 0);
        dirty = true;
    }
    return dirty;
}

bool MudGameRuntime::is_codex_unlocked(const MudPlayerState& player, const std::string& entry_id) const
{
    return std::any_of(player.codex_entries.begin(), player.codex_entries.end(), [&](const MudCodexState& state) {
        return state.entry_id == entry_id;
    });
}

bool MudGameRuntime::unlock_codex_entry(MudPlayerState* player,
                                        const std::string& entry_id,
                                        MudCommandExecution* execution) const
{
    if(player == nullptr || entry_id.empty() || is_codex_unlocked(*player, entry_id))
    {
        return false;
    }

    MudCodexState state;
    state.entry_id = entry_id;
    state.unread = true;
    state.unlocked_at_ms = mud_now_ms();
    player->codex_entries.push_back(state);
    if(execution != nullptr)
    {
        execution->unlocked_codex_entry_ids.push_back(entry_id);
        if(const auto* entry = m_world->find_codex_entry(entry_id); entry != nullptr)
        {
            execution->hints.push_back("手册解锁：" + entry->title);
        }
    }
    return true;
}

void MudGameRuntime::unlock_codex_by_trigger(MudPlayerState* player,
                                             const std::string& trigger,
                                             const std::string& target_id,
                                             MudCommandExecution* execution) const
{
    if(player == nullptr)
    {
        return;
    }
    for(const auto* entry : m_world->codex_entries_for_unlock(trigger, target_id))
    {
        if(entry != nullptr)
        {
            unlock_codex_entry(player, entry->entry_id, execution);
        }
    }
}

void MudGameRuntime::fill_codex_summary(const MudPlayerState& player,
                                        const MudCodexEntryConfig& entry,
                                        mud::CodexSummary* output) const
{
    if(output == nullptr)
    {
        return;
    }
    output->set_entry_id(entry.entry_id);
    output->set_category(entry.category);
    output->set_title(entry.title);
    output->set_summary(entry.summary);
    output->set_unlocked(is_codex_unlocked(player, entry.entry_id));
    output->set_unread(false);
    for(const auto& state : player.codex_entries)
    {
        if(state.entry_id == entry.entry_id)
        {
            output->set_unread(state.unread);
            break;
        }
    }
}

void MudGameRuntime::fill_player_snapshot(const MudPlayerState& player,
                                          mud::PlayerSnapshot* snapshot) const
{
    if(snapshot == nullptr)
    {
        return;
    }

    snapshot->set_account(player.account);
    snapshot->set_character_name(player.character_name);
    snapshot->set_level(player.level);
    snapshot->set_hp(player.hp);
    snapshot->set_max_hp(player.max_hp);
    snapshot->set_attack_power(player.attack_power);
    snapshot->set_defense_power(player.defense_power);
    snapshot->set_spirit_stone(player.spirit_stone);
    snapshot->set_title(player.title);
    snapshot->set_location_scene_id(player.location_scene_id);

    auto* cultivation = snapshot->mutable_cultivation();
    cultivation->set_realm_name(player.realm_name);
    cultivation->set_realm_stage(player.realm_stage);
    cultivation->set_exp(player.exp);
    cultivation->set_next_breakthrough_exp(player.next_breakthrough_exp);
    cultivation->set_primary_skill(player.primary_skill);
    cultivation->set_skill_level(player.skill_level);

    auto* sect = snapshot->mutable_sect();
    sect->set_sect_id(player.sect_id);
    sect->set_sect_name(player.sect_name);
    sect->set_sect_rank(player.sect_rank);
    sect->set_joined(!player.sect_id.empty());

    fill_origin_state(player, snapshot->mutable_race());
    fill_background_state(player, snapshot->mutable_background());
    fill_base_attributes(player.base_attributes, snapshot->mutable_base_attributes());
    fill_status_attributes(player.status_attributes, snapshot->mutable_status_attributes());
    MudStatusAttributeState current_status = player.status_attributes;
    current_status.kee = player.hp;
    current_status.sen = flag_int_value(player, "current_sen", player.status_attributes.sen);
    current_status.sta = flag_int_value(player, "current_sta", player.status_attributes.sta);
    current_status.mana = flag_int_value(player, "current_mana", player.status_attributes.mana);
    fill_status_attributes(current_status, snapshot->mutable_current_status_attributes());
    fill_combat_attributes(player.combat_attributes, snapshot->mutable_combat_attributes());

    snapshot->clear_inventory();
    for(const auto& item_state : player.inventory)
    {
        const auto* item_config = m_world->find_item(item_state.item_id);
        auto* item = snapshot->add_inventory();
        item->set_item_id(item_state.item_id);
        item->set_name(item_config == nullptr ? item_state.item_id : item_config->name);
        item->set_quantity(item_state.quantity);
        item->set_equipped(item_state.equipped);
        item->set_description(item_config == nullptr ? "" : item_config->description);
        item->set_item_type(item_config == nullptr ? "" : item_config->item_type);
    }

    snapshot->clear_quests();
    for(const auto& quest_state : player.quests)
    {
        const auto* quest_config = m_world->find_quest(quest_state.quest_id);
        auto* quest = snapshot->add_quests();
        quest->set_quest_id(quest_state.quest_id);
        quest->set_title(quest_config == nullptr ? quest_state.quest_id : quest_config->title);
        quest->set_status(quest_state.status);
        quest->set_progress(quest_state.progress);
        quest->set_target(quest_config == nullptr ? 0 : quest_config->required_item_count);
        quest->set_description(quest_config == nullptr ? "" : quest_config->description);
        quest->set_quest_kind(quest_config == nullptr ? "" : quest_config->quest_kind);
        quest->set_repeatable(quest_config != nullptr && quest_config->repeatable);
        if(quest_config != nullptr)
        {
            if(const auto* issuer = m_world->find_npc(quest_config->issuer_npc_id); issuer != nullptr)
            {
                quest->set_issuer_hint(issuer->name);
            }
            else
            {
                quest->set_issuer_hint(quest_config->issuer_npc_id);
            }
        }
    }

    snapshot->clear_known_commands();
    snapshot->add_known_commands("look");
    snapshot->add_known_commands("help [topic]");
    snapshot->add_known_commands("commands");
    snapshot->add_known_commands("newbie");
    snapshot->add_known_commands("map");
    snapshot->add_known_commands("go <direction>");
    snapshot->add_known_commands("rumor");
    snapshot->add_known_commands("who");
    snapshot->add_known_commands("hp");
    snapshot->add_known_commands("score");
    snapshot->add_known_commands("rank [realm|wealth|combat|alchemy|travel|bounty|chief]");
    snapshot->add_known_commands("board");
    snapshot->add_known_commands("read <id>");
    snapshot->add_known_commands("post <题目>=<正文>");
    snapshot->add_known_commands("discard <id>");
    snapshot->add_known_commands("work");
    snapshot->add_known_commands("duty");
    snapshot->add_known_commands("wanted");
    snapshot->add_known_commands("travel");
    snapshot->add_known_commands("claim [id]");
    snapshot->add_known_commands("tasks");
    snapshot->add_known_commands("skills");
    snapshot->add_known_commands("spells");
    snapshot->add_known_commands("family");
    snapshot->add_known_commands("inspect <target>");
    snapshot->add_known_commands("talk <npc>");
    snapshot->add_known_commands("ask <npc> about rumor");
    snapshot->add_known_commands("ask <npc> <话题>");
    snapshot->add_known_commands("accept <quest>");
    snapshot->add_known_commands("submit <quest>");
    snapshot->add_known_commands("fight <target>");
    snapshot->add_known_commands("use <item>");
    snapshot->add_known_commands("loot <item>");
    snapshot->add_known_commands("harvest <node>");
    snapshot->add_known_commands("cast <spell> <target>");
    snapshot->add_known_commands("flee");
    snapshot->add_known_commands("practice <skill>");
    snapshot->add_known_commands("meditate");
    snapshot->add_known_commands("breakthrough");
    snapshot->add_known_commands("brew <recipe>");
    snapshot->add_known_commands("codex <category> [entry]");
    snapshot->add_known_commands("buy <item>");
    snapshot->add_known_commands("sell <item>");
    snapshot->add_known_commands("contribute <item>");
    snapshot->add_known_commands("join <sect>");
    snapshot->add_known_commands("team create|join|info|leave");
    snapshot->add_known_commands("event");
    snapshot->add_known_commands("chat <channel> <message>");
    snapshot->add_known_commands("say <message>");
    snapshot->add_known_commands("tell <player> <message>");
    snapshot->add_known_commands("reply <message>");
    snapshot->add_known_commands("emote <message>");
    snapshot->add_known_commands("follow <player>");
    snapshot->add_known_commands("guard <player>");
    snapshot->add_known_commands("trade <player>");
    snapshot->add_known_commands("challenge <player>");
    snapshot->add_known_commands("save");
    snapshot->set_recommended_poll_interval_ms(1500);
    fill_command_catalog(player, snapshot->mutable_command_catalog());
    fill_team_snapshot(player, snapshot->mutable_team());
    snapshot->set_progression_chapter(progression_chapter_for_player(player));
    snapshot->clear_unlocked_regions();
    for(const auto& region : unlocked_regions_for_player(player))
    {
        snapshot->add_unlocked_regions(region);
    }
    snapshot->set_sect_contribution(sect_contribution_for_player(player));
    snapshot->clear_titles();
    for(const auto& title : titles_for_player(player))
    {
        snapshot->add_titles(title);
    }
    snapshot->set_stage_label(stage_label_for_player(player));
    snapshot->set_newbie_protected(has_newbie_protection(player));
    snapshot->set_newbie_protection_summary(newbie_protection_summary_for_player(player));
    snapshot->set_recommended_loop(recommended_loop_for_player(player));
    const auto* scene = current_scene(player);
    const std::string location_text =
        scene == nullptr ? std::string("无名之地") : (scene->region_name.empty() ? scene->name : (scene->region_name + "-" + scene->name));
    snapshot->set_status_line_text(player.character_name + " | " + player.realm_name + " | 气血 " +
                                   std::to_string(player.hp) + "/" + std::to_string(player.max_hp) + " | 法力 " +
                                   std::to_string(current_status.mana) + "/" + std::to_string(player.status_attributes.mana) +
                                   " | 气力 " + std::to_string(current_status.sta) + "/" +
                                   std::to_string(player.status_attributes.sta));
    snapshot->set_subprompt_text((player.sect_name.empty() ? std::string("散修") : player.sect_name) + " | " + location_text +
                                 " | " + (has_newbie_protection(player) ? std::string("新手庇护中") : std::string("可自由行走")));
    snapshot->set_identity_track(identity_track_for_player(player));
    snapshot->set_rank_level(rank_level_for_player(player));
    snapshot->set_contribution_state(contribution_state_for_player(player));
    snapshot->set_reputation_state(reputation_state_for_player(player));
    snapshot->set_unread_board_count(unread_board_count_for_player(player));
    snapshot->clear_available_short_commands();
    static const std::vector<std::string> kShortCommands = {"look", "listen", "talk", "travel", "journal", "bag", "score", "rank"};
    for(const auto& command : kShortCommands)
    {
        snapshot->add_available_short_commands(command);
    }
    snapshot->clear_route_summaries();
    for(const auto& route : route_summaries_for_player(player))
    {
        fill_route_summary(route, snapshot->add_route_summaries());
    }
    snapshot->clear_weekly_events();
    for(const auto& event : weekly_events_for_player(player))
    {
        fill_weekly_event_summary(event, snapshot->add_weekly_events());
    }
    snapshot->clear_unlocked_route_ids();
    for(const auto& route_id : unlocked_routes_for_player(player))
    {
        snapshot->add_unlocked_route_ids(route_id);
    }

    snapshot->clear_skills();
    for(const auto& skill_state : player.skills)
    {
        const auto* skill_config = m_world->find_skill(skill_state.skill_id);
        auto* output = snapshot->add_skills();
        output->set_skill_id(skill_state.skill_id);
        output->set_name(skill_config == nullptr ? skill_state.skill_id : skill_config->name);
        output->set_category(skill_config == nullptr ? "" : skill_config->category);
        output->set_level(skill_state.level);
        output->set_proficiency(skill_state.proficiency);
        output->set_description(skill_config == nullptr ? "" : skill_config->description);
    }

    snapshot->clear_spells();
    for(const auto& spell_state : player.spells)
    {
        const auto* spell_config = m_world->find_spell(spell_state.spell_id);
        auto* output = snapshot->add_spells();
        output->set_spell_id(spell_state.spell_id);
        output->set_name(spell_config == nullptr ? spell_state.spell_id : spell_config->name);
        output->set_element(spell_config == nullptr ? "" : spell_config->element);
        output->set_level(spell_state.level);
        output->set_proficiency(spell_state.proficiency);
        output->set_mana_cost(spell_config == nullptr ? 0 : spell_config->mana_cost);
        output->set_description(spell_config == nullptr ? "" : spell_config->description);
        output->set_unlocked(spell_state.unlocked);
    }

    snapshot->clear_recipes();
    for(const auto& recipe_state : player.recipes)
    {
        const auto* recipe_config = m_world->find_recipe(recipe_state.recipe_id);
        auto* output = snapshot->add_recipes();
        output->set_recipe_id(recipe_state.recipe_id);
        output->set_name(recipe_config == nullptr ? recipe_state.recipe_id : recipe_config->name);
        output->set_unlocked(recipe_state.unlocked);
        output->set_level(recipe_state.level);
        output->set_proficiency(recipe_state.proficiency);
        output->set_description(recipe_config == nullptr ? "" : recipe_config->description);
    }

    auto* profession = snapshot->mutable_profession();
    profession->set_alchemy_level(player.profession.alchemy_level);
    profession->set_exploration_level(player.profession.exploration_level);
    profession->set_formation_level(player.profession.formation_level);
    profession->set_forging_level(player.profession.forging_level);

    snapshot->clear_codex_summaries();
    static const std::vector<std::string> kCodexCategories = {
        "人物志", "宗门志", "妖兽志", "奇虫志", "地理志", "灵草丹药志", "功法技能志", "法术志", "宝物阵法志", "韩立年历"};
    for(const auto& category : kCodexCategories)
    {
        auto entries = m_world->codex_entries_for_category(category);
        if(entries.empty())
        {
            continue;
        }
        int unlocked_count = 0;
        int unread_count = 0;
        for(const auto& entry : entries)
        {
            if(is_codex_unlocked(player, entry.entry_id))
            {
                ++unlocked_count;
            }
        }
        for(const auto& state : player.codex_entries)
        {
            if(state.unread)
            {
                if(const auto* entry = m_world->find_codex_entry(state.entry_id);
                   entry != nullptr && entry->category == category)
                {
                    ++unread_count;
                }
            }
        }
        auto* summary = snapshot->add_codex_summaries();
        summary->set_entry_id(category);
        summary->set_category(category);
        summary->set_title(category);
        summary->set_summary("已解锁 " + std::to_string(unlocked_count) + "/" + std::to_string(entries.size()));
        summary->set_unlocked(unlocked_count > 0);
        summary->set_unread(unread_count > 0);
    }
}

void MudGameRuntime::fill_scene_snapshot(const MudPlayerState& player,
                                         mud::SceneSnapshot* snapshot) const
{
    if(snapshot == nullptr)
    {
        return;
    }

    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return;
    }

    snapshot->set_scene_id(scene->scene_id);
    snapshot->set_scene_name(scene->name);
    snapshot->set_region_name(scene->region_name);
    snapshot->set_description(scene->description);
    snapshot->set_room_type(scene->room_type);
    snapshot->set_risk_level(scene->risk_level);
    snapshot->set_landmark(scene->landmark);
    snapshot->set_pvp_enabled(scene->pvp_enabled);
    snapshot->set_room_layer(scene->room_layer);
    snapshot->clear_exits();
    snapshot->clear_npcs();
    snapshot->clear_monsters();
    snapshot->clear_shops();
    snapshot->clear_players();
    snapshot->clear_items();
    snapshot->clear_resource_nodes();
    snapshot->clear_ground_loots();
    snapshot->clear_hazards();
    snapshot->clear_related_codex_entry_ids();
    snapshot->clear_rumors();
    snapshot->clear_loop_tags();
    snapshot->clear_local_board_entries();
    snapshot->clear_presence_board();
    snapshot->clear_exit_board();
    snapshot->clear_service_tags();
    snapshot->clear_rumor_topics();
    snapshot->clear_mentor_ids();
    snapshot->set_board_available(scene->board_available);

    for(const auto& entry : scene->exits)
    {
        auto* exit = snapshot->add_exits();
        exit->set_direction(entry.first);
        exit->set_target_scene_id(entry.second);
        if(const auto* target = m_world->find_scene(entry.second); target != nullptr)
        {
            exit->set_target_scene_name(target->name);
        }
    }

    for(const auto& npc_id : scene->npc_ids)
    {
        const auto* npc = m_world->find_npc(npc_id);
        if(npc == nullptr)
        {
            continue;
        }
        auto* node = snapshot->add_npcs();
        node->set_npc_id(npc->npc_id);
        node->set_name(npc->name);
        node->set_has_quest(!npc->quest_ids.empty());
        node->set_hint(!npc->presence_text.empty() ? npc->presence_text : npc->hint);
        node->set_codex_entry_id(npc->codex_entry_id);
    }

    for(const auto& monster_id : scene->monster_ids)
    {
        if(const auto* monster = m_world->find_monster(monster_id); monster != nullptr)
        {
            snapshot->add_monsters(monster->name);
        }
    }

    for(const auto& item_id : scene->shop_item_ids)
    {
        if(const auto* item = m_world->find_item(item_id); item != nullptr)
        {
            snapshot->add_shops(item->name);
            auto* visible_item = snapshot->add_items();
            visible_item->set_item_id(item->item_id);
            visible_item->set_name(item->name);
            visible_item->set_item_type(item->item_type);
            visible_item->set_description(item->description);
            visible_item->set_source("shop");
            visible_item->set_price(item->price);
            visible_item->set_codex_entry_id(item->codex_entry_id);
        }
    }

    for(const auto& node_id : scene->resource_node_ids)
    {
        const auto* node = m_world->find_resource_node(node_id);
        if(node == nullptr)
        {
            continue;
        }
        const auto* item = m_world->find_item(node->drop_item_id);
        auto* output = snapshot->add_resource_nodes();
        output->set_node_id(node->node_id);
        output->set_name(node->name);
        output->set_description(node->description);
        output->set_drop_item_id(node->drop_item_id);
        output->set_drop_item_name(item == nullptr ? node->drop_item_id : item->name);
        output->set_drop_item_count(node->drop_item_count);
        output->set_codex_entry_id(node->codex_entry_id);
    }

    for(const auto& loot_id : scene->ground_loot_ids)
    {
        const auto* loot = m_world->find_ground_loot(loot_id);
        if(loot == nullptr)
        {
            continue;
        }
        if(loot->one_time)
        {
            const auto one_time_key = "loot:" + loot->loot_id;
            if(auto flag_iter = player.flags.find(one_time_key); flag_iter != player.flags.end() &&
               flag_iter->second == "1")
            {
                continue;
            }
        }
        const auto* item = m_world->find_item(loot->item_id);
        auto* output = snapshot->add_ground_loots();
        output->set_loot_id(loot->loot_id);
        output->set_item_id(loot->item_id);
        output->set_item_name(item == nullptr ? loot->item_id : item->name);
        output->set_quantity(loot->quantity);
        output->set_description(loot->description);
        output->set_codex_entry_id(item == nullptr ? "" : item->codex_entry_id);
        output->set_lootable(true);
    }

    for(const auto& hazard_id : scene->hazard_ids)
    {
        const auto* hazard = m_world->find_hazard(hazard_id);
        if(hazard == nullptr)
        {
            continue;
        }
        auto* output = snapshot->add_hazards();
        output->set_hazard_id(hazard->hazard_id);
        output->set_name(hazard->name);
        output->set_description(hazard->description);
        output->set_hp_cost(hazard->hp_cost);
        output->set_mana_cost(hazard->mana_cost);
        output->set_sta_cost(hazard->sta_cost);
        output->set_codex_entry_id(hazard->codex_entry_id);
    }

    for(const auto& entry_id : scene->codex_entry_ids)
    {
        snapshot->add_related_codex_entry_ids(entry_id);
    }
    for(const auto& rumor : scene->rumors)
    {
        snapshot->add_rumors(rumor);
    }
    for(const auto& tag : scene->loop_tags)
    {
        snapshot->add_loop_tags(tag);
    }
    for(const auto& tag : scene->service_tags)
    {
        snapshot->add_service_tags(tag);
    }
    for(const auto& topic : scene->rumor_topics)
    {
        snapshot->add_rumor_topics(topic);
    }
    for(const auto& mentor_id : scene->mentor_ids)
    {
        snapshot->add_mentor_ids(mentor_id);
    }
    for(const auto& entry : board_entries_for_player(player, scene))
    {
        fill_summary_entry(entry, snapshot->add_local_board_entries());
    }
    for(const auto* post : board_posts_for_scene(player, scene->scene_id, 3))
    {
        if(post == nullptr)
        {
            continue;
        }
        const auto parsed_title = parse_board_post_title(post->title);
        const auto& subject = parsed_title.second;
        MudSummaryEntry entry;
        entry.entry_id = "post:" + std::to_string(post->event_id);
        entry.title = subject.empty() ? "无题留帖" : subject;
        entry.summary = compact_preview(post->content, 48);
        entry.status = "留帖";
        entry.category = "板帖";
        entry.command = "read " + std::to_string(post->event_id);
        entry.location_hint = scene->name;
        fill_summary_entry(entry, snapshot->add_local_board_entries());
    }
    snapshot->set_resource_refresh_summary(resource_refresh_summary_for_scene(player, *scene));
    snapshot->set_ambient_mood(scene_ambient_mood(*scene));
    snapshot->set_palette_id(scene_palette_id(*scene));
    snapshot->set_scene_brief(first_dense_sentence(scene->scene_brief.empty() ? scene->description : scene->scene_brief,
                                                   scene->description));
    if(!scene->scene_aftertaste.empty())
    {
        snapshot->set_scene_aftertaste(scene->scene_aftertaste);
    }
    else if(!scene->rumors.empty())
    {
        snapshot->set_scene_aftertaste(scene->rumors.front());
    }
    else if(!scene->landmark.empty())
    {
        snapshot->set_scene_aftertaste("此地地标：" + scene->landmark + "。");
    }
    else
    {
        snapshot->set_scene_aftertaste(scene_ambient_mood(*scene) + "。");
    }

    std::vector<const OnlinePresenceState*> visible_players;
    visible_players.reserve(m_online_presence.size());
    const auto now = mud_now_ms();
    for(const auto& [account, presence] : m_online_presence)
    {
        if(account.empty() || account == player.account)
        {
            continue;
        }
        if(presence.last_seen_ms <= 0 || now - presence.last_seen_ms > kScenePresenceTtlMs)
        {
            continue;
        }
        if(presence.player.location_scene_id != scene->scene_id)
        {
            continue;
        }
        visible_players.push_back(&presence);
    }

    std::sort(visible_players.begin(), visible_players.end(), [](const OnlinePresenceState* lhs,
                                                                 const OnlinePresenceState* rhs) {
        if(lhs == nullptr || rhs == nullptr)
        {
            return lhs != nullptr;
        }
        if(lhs->player.character_name == rhs->player.character_name)
        {
            return lhs->player.account < rhs->player.account;
        }
        return lhs->player.character_name < rhs->player.character_name;
    });

    for(const auto* presence : visible_players)
    {
        if(presence == nullptr)
        {
            continue;
        }
        auto* scene_player = snapshot->add_players();
        scene_player->set_account(presence->player.account);
        scene_player->set_character_name(presence->player.character_name);
        scene_player->set_title(presence->player.title);
        scene_player->set_realm_name(presence->player.realm_name);
        scene_player->set_sect_name(presence->player.sect_name);
    }

    for(const auto& npc_id : scene->npc_ids)
    {
        if(const auto* npc = m_world->find_npc(npc_id); npc != nullptr)
        {
            const std::string detail = !npc->presence_text.empty() ? npc->presence_text : (!npc->hint.empty() ? npc->hint : npc->dialogue);
            snapshot->add_presence_board("人物｜" + npc->name + "｜" + first_dense_sentence(detail, "此人正在此地停留。"));
        }
    }
    for(const auto* presence : visible_players)
    {
        if(presence == nullptr)
        {
            continue;
        }
        snapshot->add_presence_board("同道｜" + presence->player.character_name + "｜" + presence->player.realm_name);
    }
    for(const auto& monster_id : scene->monster_ids)
    {
        if(const auto* monster = m_world->find_monster(monster_id); monster != nullptr)
        {
            snapshot->add_presence_board("妖兽｜" + monster->name + "｜" + first_dense_sentence(monster->description, "气息危险。"));
        }
    }
    for(const auto& item_id : scene->shop_item_ids)
    {
        if(const auto* item = m_world->find_item(item_id); item != nullptr)
        {
            snapshot->add_presence_board("物件｜" + item->name + "｜" + item_type_display_name(item->item_type));
        }
    }
    for(const auto& node_id : scene->resource_node_ids)
    {
        if(const auto* node = m_world->find_resource_node(node_id); node != nullptr)
        {
            snapshot->add_presence_board("采点｜" + node->name + "｜可采 " +
                                         item_with_count_label(m_world.get(), node->drop_item_id, node->drop_item_count));
        }
    }
    for(const auto& loot_id : scene->ground_loot_ids)
    {
        const auto* loot = m_world->find_ground_loot(loot_id);
        if(loot == nullptr)
        {
            continue;
        }
        if(loot->one_time)
        {
            const auto one_time_key = "loot:" + loot->loot_id;
            if(auto flag_iter = player.flags.find(one_time_key); flag_iter != player.flags.end() &&
               flag_iter->second == "1")
            {
                continue;
            }
        }
        snapshot->add_presence_board("遗落｜" + item_display_name(m_world.get(), loot->item_id, "遗落物") +
                                     "｜可拾取 " + std::to_string(loot->quantity) + " 件");
    }
    for(const auto& [direction, target_id] : scene->exits)
    {
        const auto* target = m_world->find_scene(target_id);
        const std::string target_name = target == nullptr ? target_id : target->name;
        const std::string risk = target == nullptr ? std::string() : target->room_layer;
        snapshot->add_exit_board(direction_display_name(direction) + "｜" + target_name +
                                 (risk.empty() ? std::string() : ("｜" + risk)));
    }
}

void MudGameRuntime::remember_scene_presence(const MudPlayerState& player)
{
    if(player.account.empty())
    {
        return;
    }

    auto& presence = m_online_presence[player.account];
    presence.player = player;
    presence.last_seen_ms = mud_now_ms();
}

void MudGameRuntime::prune_scene_presence()
{
    const auto now = mud_now_ms();
    for(auto iter = m_online_presence.begin(); iter != m_online_presence.end();)
    {
        if(iter->second.last_seen_ms <= 0 || now - iter->second.last_seen_ms > kScenePresenceTtlMs)
        {
            iter = m_online_presence.erase(iter);
            continue;
        }
        ++iter;
    }
}

void MudGameRuntime::append_event(const std::string& target_account,
                                  const std::string& type,
                                  const std::string& title,
                                  const std::string& content,
                                  std::vector<MudEventEnvelope>* out_batch)
{
    MudEventEnvelope event;
    event.event_id = m_next_event_id++;
    event.target_account = target_account;
    event.type = type;
    event.title = title;
    event.content = content;
    event.server_time_ms = mud_now_ms();

    m_events.push_back(event);
    trim_events();
    if(out_batch != nullptr)
    {
        out_batch->push_back(event);
    }
}

void MudGameRuntime::merge_persisted_events(const std::vector<MudEventEnvelope>& events)
{
    if(events.empty())
    {
        return;
    }

    for(const auto& event : events)
    {
        if(event.event_id == 0)
        {
            continue;
        }

        auto exact_iter = std::find_if(m_events.begin(), m_events.end(), [&](const MudEventEnvelope& current) {
            return current.event_id == event.event_id;
        });
        if(exact_iter != m_events.end())
        {
            *exact_iter = event;
        }
        else
        {
            auto payload_iter =
                std::find_if(m_events.begin(), m_events.end(), [&](const MudEventEnvelope& current) {
                    return current.event_id != event.event_id && same_event_payload(current, event);
                });
            if(payload_iter != m_events.end())
            {
                *payload_iter = event;
            }
            else
            {
                m_events.push_back(event);
            }
        }

        if(event.event_id >= m_next_event_id)
        {
            m_next_event_id = event.event_id + 1;
        }
    }

    std::sort(m_events.begin(), m_events.end(), [](const MudEventEnvelope& lhs, const MudEventEnvelope& rhs) {
        return lhs.event_id < rhs.event_id;
    });
    m_events.erase(std::unique(m_events.begin(), m_events.end(), [](const MudEventEnvelope& lhs,
                                                                    const MudEventEnvelope& rhs) {
                       return lhs.event_id == rhs.event_id;
                   }),
                   m_events.end());
    trim_events();
}

void MudGameRuntime::add_events_to_response(const std::vector<MudEventEnvelope>& events,
                                            google::protobuf::RepeatedPtrField<mud::GameEvent>* out_events) const
{
    if(out_events == nullptr)
    {
        return;
    }

    out_events->Clear();
    for(const auto& event : events)
    {
        auto* output = out_events->Add();
        output->set_event_id(event.event_id);
        output->set_type(event.type);
        output->set_title(event.title);
        output->set_content(event.content);
        output->set_server_time_ms(event.server_time_ms);
        output->set_unread(true);
        output->set_channel(event_channel_name(event));
        output->set_tone(event_tone_name(event));
        output->set_render_mode("log_line");
        if(!event.title.empty())
        {
            output->add_emphasis_keywords(event.title);
        }
    }
}

void MudGameRuntime::trim_events()
{
    static constexpr size_t kMaxEvents = 512;
    if(m_events.size() > kMaxEvents)
    {
        m_events.erase(m_events.begin(), m_events.begin() + static_cast<std::ptrdiff_t>(m_events.size() - kMaxEvents));
    }
}

std::vector<MudEventEnvelope> MudGameRuntime::recent_events_for_account(const std::string& account, int limit) const
{
    const int normalized_limit = std::clamp(limit <= 0 ? 20 : limit, 1, 100);
    std::vector<MudEventEnvelope> filtered;
    filtered.reserve(static_cast<size_t>(normalized_limit));
    for(auto iter = m_events.rbegin(); iter != m_events.rend() && static_cast<int>(filtered.size()) < normalized_limit; ++iter)
    {
        if(!iter->target_account.empty() && iter->target_account != account)
        {
            continue;
        }
        filtered.push_back(*iter);
    }
    std::reverse(filtered.begin(), filtered.end());
    return filtered;
}

void MudGameRuntime::build_bootstrap_response(const std::string& account,
                                              const std::optional<MudPlayerState>& player,
                                              mud::BootstrapResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    response->set_need_create_character(!player.has_value());
    response->clear_available_origins();
    for(const auto& origin : m_world->origins())
    {
        auto* output = response->add_available_origins();
        output->set_origin_id(origin.origin_id);
        output->set_origin_name(origin.name);
        output->set_race_name(origin.race_name);
        output->set_homeland(origin.homeland);
        output->set_description(origin.description);
    }
    response->clear_available_backgrounds();
    for(const auto& background : m_world->backgrounds())
    {
        auto* output = response->add_available_backgrounds();
        output->set_background_id(background.background_id);
        output->set_name(background.name);
        output->set_description(background.description);
        output->set_starter_title(background.starter_title);
        output->set_focus_label(background.focus_label);
    }
    if(!player.has_value())
    {
        MudPlayerState preview;
        preview.account = account;
        preview.location_scene_id = m_world->defaults().starting_scene_id;
        fill_scene_snapshot(preview, response->mutable_scene());
        response->set_stage_label("凡躯启程");
        response->set_newbie_protected(true);
        response->set_newbie_protection_summary("新角色处于新手保护期，可先观察世界、接引导事务、熟悉六大循环。");
        response->set_recommended_loop("先在七玄门或太南散修坊接第一条事务，尽快形成一条稳定收益循环。");
        response->clear_route_summaries();
        response->clear_weekly_events();
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kSuccess,
                                                     "character not found");
        response->set_next_event_id(m_next_event_id == 0 ? 0 : (m_next_event_id - 1));
        return;
    }

    auto normalized_player = *player;
    normalize_player_state(&normalized_player);
    MudCommandExecution bootstrap_unlocks;
    unlock_codex_by_trigger(&normalized_player, "enter_scene", normalized_player.location_scene_id, &bootstrap_unlocks);
    m_character_names[normalized_player.account] = normalized_player.character_name;
    remember_scene_presence(normalized_player);
    if(auto team_iter = m_team_by_account.find(player->account); team_iter != m_team_by_account.end())
    {
        if(auto state_iter = m_teams.find(team_iter->second); state_iter != m_teams.end())
        {
            for(auto& member : state_iter->second.members)
            {
                if(member.account == normalized_player.account)
                {
                    member.display_name = normalized_player.character_name;
                    member.player_state = normalized_player;
                    break;
                }
            }
        }
    }
    fill_player_snapshot(normalized_player, response->mutable_player());
    fill_scene_snapshot(normalized_player, response->mutable_scene());
    fill_gameplay_guidance(normalized_player, response);
    auto recent_events = recent_events_for_account(account, 50);
    add_events_to_response(recent_events, response->mutable_events());
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
    response->set_next_event_id(recent_events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1))
                                                      : recent_events.back().event_id);
}

void MudGameRuntime::build_create_character_response(const MudPlayerState& player,
                                                     mud::CharacterCreateResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    auto normalized_player = player;
    normalize_player_state(&normalized_player);
    m_character_names[normalized_player.account] = normalized_player.character_name;
    remember_scene_presence(normalized_player);
    if(auto team_iter = m_team_by_account.find(normalized_player.account); team_iter != m_team_by_account.end())
    {
        if(auto state_iter = m_teams.find(team_iter->second); state_iter != m_teams.end())
        {
            for(auto& member : state_iter->second.members)
            {
                if(member.account == normalized_player.account)
                {
                    member.display_name = normalized_player.character_name;
                    member.player_state = normalized_player;
                    break;
                }
            }
        }
    }
    std::vector<MudEventEnvelope> events;
    append_event(normalized_player.account,
                 "system",
                 "踏入修仙路",
                 normalized_player.character_name + "自七玄门山脚启程，正式踏上凡人修仙之路。",
                 &events);
    fill_player_snapshot(normalized_player, response->mutable_player());
    fill_scene_snapshot(normalized_player, response->mutable_scene());
    add_events_to_response(events, response->mutable_events());
    response->set_next_event_id(events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1)) : events.back().event_id);
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 "character created");
}

void MudGameRuntime::build_command_response(const MudPlayerState& player,
                                            const std::string& command,
                                            const MudCommandExecution& execution,
                                            mud::CommandExecuteResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    m_character_names[player.account] = player.character_name;
    remember_scene_presence(player);
    if(auto team_iter = m_team_by_account.find(player.account); team_iter != m_team_by_account.end())
    {
        if(auto state_iter = m_teams.find(team_iter->second); state_iter != m_teams.end())
        {
            for(auto& member : state_iter->second.members)
            {
                if(member.account == player.account)
                {
                    member.display_name = player.character_name;
                    member.player_state = player;
                    break;
                }
            }
        }
    }
    auto* result = response->mutable_result();
    result->set_command(command);
    result->set_success(execution.success);
    result->set_title(execution.title);
    result->set_summary(execution.summary);
    result->set_recommended_poll_interval_ms(execution.recommended_poll_interval_ms);
    result->set_spell_summary(execution.spell_summary);
    result->set_brew_summary(execution.brew_summary);
    result->set_hazard_feedback(execution.hazard_feedback);
    result->clear_hints();
    for(const auto& hint : execution.hints)
    {
        result->add_hints(hint);
    }
    result->clear_unlocked_codex_entries();
    for(const auto& entry_id : execution.unlocked_codex_entry_ids)
    {
        if(const auto* entry = m_world->find_codex_entry(entry_id); entry != nullptr)
        {
            fill_codex_summary(player, *entry, result->add_unlocked_codex_entries());
        }
    }
    result->clear_panels();
    for(const auto& panel : execution.panels)
    {
        fill_structured_panel(panel, result->add_panels());
    }

    fill_player_snapshot(player, response->mutable_player());
    fill_scene_snapshot(player, response->mutable_scene());
    std::vector<MudEventEnvelope> visible_events;
    visible_events.reserve(execution.events.size());
    for(const auto& event : execution.events)
    {
        if(event.target_account.empty() || event.target_account == player.account)
        {
            visible_events.push_back(event);
        }
    }
    add_events_to_response(visible_events, response->mutable_events());
    response->set_next_event_id(visible_events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1))
                                                       : visible_events.back().event_id);
    http_code_message::gateway::set_code_message(response,
                                                 execution.success ? http_code_message::gateway::code::kSuccess
                                                                   : http_code_message::gateway::code::kInvalidMudCommand,
                                                 execution.summary);
}

void MudGameRuntime::build_feed_response(const std::string& account,
                                         const std::optional<MudPlayerState>& player,
                                         uint64_t after_event_id,
                                         int limit,
                                         mud::FeedPullResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    const int normalized_limit = std::clamp(limit <= 0 ? 50 : limit, 1, 100);
    std::vector<MudEventEnvelope> events;
    events.reserve(static_cast<size_t>(normalized_limit));
    for(const auto& event : m_events)
    {
        if(event.event_id <= after_event_id)
        {
            continue;
        }
        if(!event.target_account.empty() && event.target_account != account)
        {
            continue;
        }
        events.push_back(event);
        if(static_cast<int>(events.size()) >= normalized_limit)
        {
            break;
        }
    }

    add_events_to_response(events, response->mutable_events());
    response->set_next_event_id(events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1)) : events.back().event_id);
    response->set_recommended_poll_interval_ms(1500);
    if(player.has_value())
    {
        auto normalized_player = *player;
        normalize_player_state(&normalized_player);
        m_character_names[normalized_player.account] = normalized_player.character_name;
        remember_scene_presence(normalized_player);
        fill_scene_snapshot(normalized_player, response->mutable_scene());
    }
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
}

void MudGameRuntime::build_codex_list_response(const MudPlayerState& player,
                                               const std::string& category,
                                               mud::CodexListResponse* response) const
{
    if(response == nullptr)
    {
        return;
    }

    response->clear_entries();
    for(const auto& entry : m_world->codex_entries_for_category(category))
    {
        fill_codex_summary(player, entry, response->add_entries());
    }
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
}

void MudGameRuntime::build_codex_detail_response(const MudPlayerState& player,
                                                 const std::string& entry_id,
                                                 mud::CodexDetailResponse* response) const
{
    if(response == nullptr)
    {
        return;
    }

    const auto* entry = m_world->find_codex_entry(entry_id);
    if(entry == nullptr)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kInvalidMudCommand,
                                                     "codex entry not found");
        return;
    }

    auto* output = response->mutable_entry();
    output->set_entry_id(entry->entry_id);
    output->set_category(entry->category);
    output->set_title(entry->title);
    output->set_summary(entry->summary);
    output->set_content(is_codex_unlocked(player, entry->entry_id) ? entry->content : "资料未明，需通过剧情与探索解锁。");
    output->clear_related_scene_ids();
    output->clear_related_npc_ids();
    output->clear_related_monster_ids();
    output->clear_related_item_ids();
    output->clear_related_sect_ids();
    for(const auto& value : entry->related_scene_ids)
    {
        output->add_related_scene_ids(value);
    }
    for(const auto& value : entry->related_npc_ids)
    {
        output->add_related_npc_ids(value);
    }
    for(const auto& value : entry->related_monster_ids)
    {
        output->add_related_monster_ids(value);
    }
    for(const auto& value : entry->related_item_ids)
    {
        output->add_related_item_ids(value);
    }
    for(const auto& value : entry->related_sect_ids)
    {
        output->add_related_sect_ids(value);
    }
    output->set_unlocked(is_codex_unlocked(player, entry->entry_id));
    output->set_unread(false);
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
}

void MudGameRuntime::build_rank_response(MudLeaderboardType leaderboard_type,
                                         const std::vector<MudLeaderboardEntry>& entries,
                                         mud::RankListResponse* response) const
{
    if(response == nullptr)
    {
        return;
    }

    response->set_leaderboard(mud_leaderboard_name(leaderboard_type));
    response->clear_entries();
    for(const auto& entry : entries)
    {
        auto* output = response->add_entries();
        output->set_rank(entry.rank);
        output->set_account(entry.player.account);
        output->set_character_name(entry.player.character_name);
        output->set_realm_name(entry.player.realm_name);
        output->set_level(entry.player.level);
        output->set_exp(entry.player.exp);
        output->set_spirit_stone(entry.player.spirit_stone);
        output->set_sect_name(entry.player.sect_name);
        output->set_title(entry.player.title);
        if(leaderboard_type == MudLeaderboardType::chief && output->title().empty() && !entry.player.sect_id.empty())
        {
            if(const auto* sect = m_world->find_sect(entry.player.sect_id);
               sect != nullptr && !sect->chief_title.empty())
            {
                output->set_title(sect->name + "·" + sect->chief_title);
            }
        }
        output->set_score(entry.score);
        output->set_extra(entry.extra);
    }
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
}

void MudGameRuntime::restore_team_state(const std::vector<MudPlayerState>& team_members)
{
    if(team_members.empty())
    {
        return;
    }

    const auto& leader = team_members.front();
    if(leader.team_id.empty())
    {
        return;
    }

    MudTeamState team;
    team.team_id = leader.team_id;
    team.team_name = leader.team_name.empty() ? (leader.character_name + "的小队") : leader.team_name;
    team.leader_account = leader.team_leader_account.empty() ? leader.account : leader.team_leader_account;
    for(const auto& member_state : team_members)
    {
        MudTeamMemberState member;
        member.account = member_state.account;
        member.display_name = member_state.character_name;
        member.leader = member_state.account == team.leader_account;
        member.player_state = member_state;
        team.members.push_back(member);
        m_team_by_account[member.account] = team.team_id;
        m_character_names[member.account] = member.display_name;
    }

    m_teams[team.team_id] = std::move(team);
}

void MudGameRuntime::forget_team_state(const std::string& account)
{
    if(account.empty())
    {
        return;
    }

    auto team_iter = m_team_by_account.find(account);
    if(team_iter == m_team_by_account.end())
    {
        return;
    }

    const auto team_id = team_iter->second;
    m_team_by_account.erase(team_iter);

    auto state_iter = m_teams.find(team_id);
    if(state_iter == m_teams.end())
    {
        return;
    }

    auto& members = state_iter->second.members;
    members.erase(std::remove_if(members.begin(), members.end(), [&](const MudTeamMemberState& member) {
                      return member.account == account;
                  }),
                  members.end());
    if(members.empty())
    {
        m_teams.erase(state_iter);
    }
}

void MudGameRuntime::fill_team_snapshot(const MudPlayerState& player,
                                        mud::TeamState* snapshot) const
{
    if(snapshot == nullptr)
    {
        return;
    }

    snapshot->Clear();
    auto team_iter = m_team_by_account.find(player.account);
    if(team_iter == m_team_by_account.end())
    {
        return;
    }

    auto state_iter = m_teams.find(team_iter->second);
    if(state_iter == m_teams.end())
    {
        return;
    }

    const auto& state = state_iter->second;
    snapshot->set_team_id(state.team_id);
    snapshot->set_team_name(state.team_name);
    for(const auto& member : state.members)
    {
        auto* output = snapshot->add_members();
        output->set_account(member.account);
        output->set_display_name(member.display_name);
        output->set_leader(member.leader);
    }
}

std::vector<std::string> MudGameRuntime::titles_for_player(const MudPlayerState& player) const
{
    std::vector<std::string> titles;
    if(!player.title.empty())
    {
        titles.push_back(player.title);
    }
    if(!player.background_name.empty())
    {
        titles.push_back("凡俗出身·" + player.background_name);
    }
    if(!player.origin_name.empty())
    {
        titles.push_back("地域出身·" + player.origin_name);
    }
    if(!player.sect_name.empty() && !player.sect_rank.empty())
    {
        titles.push_back(player.sect_name + "·" + player.sect_rank);
    }
    const auto chief_title = current_chief_title_for_player(player);
    if(!chief_title.empty())
    {
        titles.push_back(chief_title);
    }
    if(flag_int_value(player, "bounty_score", 0) > 0)
    {
        titles.push_back("缉名在册");
    }
    return titles;
}

std::string MudGameRuntime::current_chief_title_for_player(const MudPlayerState& player) const
{
    if(player.account.empty() || player.sect_id.empty())
    {
        return {};
    }
    if(const auto* sect = m_world->find_sect(player.sect_id);
       sect != nullptr && !sect->chief_title.empty() && !player.sect_rank.empty() &&
       player.sect_rank == sect->chief_title)
    {
        return sect->name + "·" + sect->chief_title;
    }
    return {};
}

void MudGameRuntime::fill_command_catalog(const MudPlayerState& player,
                                          google::protobuf::RepeatedPtrField<mud::CommandDefinition>* output) const
{
    if(output == nullptr)
    {
        return;
    }

    struct CommandSeed
    {
        const char* id;
        const char* category;
        const char* label;
        const char* command;
        const char* summary;
        const char* composer_mode;
        const char* chat_channel;
        std::vector<const char*> aliases;
        const char* usage;
        const char* target_hint;
        const char* visibility_scope;
        bool execute_immediately;
    };

    static const std::vector<CommandSeed> kCatalog = {
        {"chat_world", "social", "世界聊天", "chat world ", "直接发往世界频道。", "chat", "world", {}, "chat world <内容>", "内容", "global", false},
        {"chat_team", "social", "队伍聊天", "chat team ", "发往当前队伍频道。", "chat", "team", {}, "chat team <内容>", "内容", "team", false},
        {"say", "social", "当面说话", "say ", "只让同场景修士听见。", "command", "", {}, "say <内容>", "内容", "scene", false},
        {"tell", "social", "私聊玩家", "tell ", "向指定玩家发送私聊。", "command", "", {}, "tell <玩家> <内容>", "玩家 内容", "global", false},
        {"reply", "social", "回复私聊", "reply ", "回复最近联系你的道友。", "command", "", {}, "reply <内容>", "内容", "global", false},
        {"emote", "social", "表情动作", "emote ", "向同场景广播动作描述。", "command", "", {}, "emote <动作>", "动作", "scene", false},
        {"help", "manual", "帮助目录", "help ", "查看玩法帮助主题。", "command", "", {"?"}, "help [topic]", "topic", "global", false},
        {"commands", "manual", "命令总览", "commands", "查看常用命令、别名与示例。", "command", "", {"cmds"}, "commands", "", "global", true},
        {"newbie", "manual", "新手指引", "newbie", "查看新手入世帮助。", "command", "", {}, "newbie", "", "global", true},
        {"look", "explore", "观察场景", "look", "重新查看当前房间。", "command", "", {"l"}, "look", "", "scene", true},
        {"here", "explore", "重述此地", "here", "用更短的方式重看当前房间。", "command", "", {}, "here", "", "scene", true},
        {"listen", "explore", "凝神细听", "listen", "听一听当前房间的风声和人物动静。", "command", "", {}, "listen", "", "scene", true},
        {"map", "explore", "查看地图", "map", "查看当前世界主图。", "command", "", {}, "map", "", "global", true},
        {"rumor", "explore", "打听传闻", "rumor", "查看当前房间与天地流言。", "command", "", {}, "rumor", "", "scene", true},
        {"who", "explore", "在线人物", "who", "查看附近与在线玩家。", "command", "", {}, "who", "", "global", true},
        {"travel", "explore", "路引航路", "travel", "查看已开路引、可走方向与下一站建议。", "command", "", {}, "travel", "", "scene", true},
        {"inspect", "tasks", "查看目标", "inspect ", "查看人物、怪物、物件详情。", "command", "", {"exa"}, "inspect <目标>", "目标", "scene", false},
        {"talk", "tasks", "交谈人物", "talk ", "与 NPC 交谈。", "command", "", {}, "talk <人物>", "人物", "scene", false},
        {"ask", "tasks", "追问话题", "ask ", "向当前人物追问风声或已显出的具体话题。", "command", "", {}, "ask <人物> about rumor | ask <人物> <话题>", "人物 话题", "scene", false},
        {"accept", "tasks", "接取任务", "accept ", "接取指定任务。", "command", "", {}, "accept <任务>", "任务", "scene", false},
        {"submit", "tasks", "提交任务", "submit ", "提交已完成任务。", "command", "", {}, "submit <任务>", "任务", "scene", false},
        {"tasks", "tasks", "任务列表", "tasks", "查看当前接取的任务。", "command", "", {"quest", "quests"}, "tasks", "", "global", true},
        {"journal", "tasks", "游历札记", "journal", "把当前线索、待办和去向整理成札记。", "command", "", {}, "journal", "", "global", true},
        {"board", "loops", "公共委托", "board", "查看当前区域可接的循环与委托，以及房间近帖。", "command", "", {}, "board", "", "scene", true},
        {"read", "loops", "读板帖子", "read ", "查看当前房间留言板上的帖子。", "command", "", {}, "read <编号>", "编号", "scene", false},
        {"post", "loops", "留下帖子", "post ", "在当前房间留言板发帖。", "command", "", {}, "post <题目>=<正文>", "题目=正文", "scene", false},
        {"discard", "loops", "收起帖子", "discard ", "把一条板帖从自己视野里收起。", "command", "", {}, "discard <编号>", "编号", "scene", false},
        {"work", "loops", "当前营生", "work", "查看此地可做的工作与营生。", "command", "", {}, "work", "", "scene", true},
        {"wanted", "loops", "悬赏目标", "wanted", "查看当前房间和周边的悬赏目标。", "command", "", {}, "wanted", "", "scene", true},
        {"fight", "combat", "攻击目标", "fight ", "与妖兽或敌对目标交战。", "command", "", {"kill"}, "fight <目标>", "目标", "scene", false},
        {"challenge", "combat", "切磋挑战", "challenge ", "在可冲突区挑战同场景玩家。", "command", "", {}, "challenge <玩家>", "玩家", "scene", false},
        {"flee", "combat", "抽身退后", "flee", "暂时脱离战斗节奏。", "command", "", {}, "flee", "", "scene", true},
        {"cast", "spell", "施放法术", "cast ", "施放已掌握法术。", "command", "", {}, "cast <法术> <目标>", "法术 目标", "scene", false},
        {"spells", "spell", "法术总览", "spells", "查看已掌握法术。", "command", "", {}, "spells", "", "global", true},
        {"practice", "cultivation", "运功修炼", "practice ", "修炼主修功法或技能。", "command", "", {}, "practice <技能>", "技能", "global", false},
        {"meditate", "cultivation", "静坐调息", "meditate", "恢复法力与神念。", "command", "", {}, "meditate", "", "global", true},
        {"breakthrough", "cultivation", "尝试突破", "breakthrough", "冲击下一层境界。", "command", "", {}, "breakthrough", "", "global", true},
        {"hp", "cultivation", "气机状态", "hp", "查看当前气血、法力、气力与气海火候。", "command", "", {}, "hp", "", "global", true},
        {"score", "cultivation", "人物总览", "score", "查看属性、境界、称号与成长。", "command", "", {"sc"}, "score", "", "global", true},
        {"rank", "cultivation", "查看榜单", "rank", "查看当前榜单。", "command", "", {}, "rank [realm|wealth|combat|alchemy|travel|bounty|chief]", "榜单类型", "global", true},
        {"skills", "cultivation", "技能总览", "skills", "查看技能熟练度。", "command", "", {}, "skills", "", "global", true},
        {"bag", "trade", "行囊清单", "bag", "查看背包和随身物件。", "command", "", {"i", "inventory"}, "bag", "", "global", true},
        {"harvest", "gather", "采集资源", "harvest ", "采集当前房间资源点。", "command", "", {}, "harvest <资源>", "资源", "scene", false},
        {"loot", "gather", "拾取掉落", "loot ", "拾取地面遗落物。", "command", "", {}, "loot <物品>", "物品", "scene", false},
        {"brew", "alchemy", "炼制丹药", "brew ", "按配方炼制丹药。", "command", "", {}, "brew <配方>", "配方", "scene", false},
        {"buy", "trade", "购买物件", "buy ", "从当前场景商铺购买。", "command", "", {}, "buy <物件>", "物件", "scene", false},
        {"sell", "trade", "出售物件", "sell ", "把背包物件卖给坊市。", "command", "", {}, "sell <物件>", "物件", "scene", false},
        {"trade", "trade", "交易请求", "trade ", "向同场景玩家发起交易请求。", "command", "", {}, "trade <玩家>", "玩家", "scene", false},
        {"join", "group", "拜入势力", "join ", "加入当前可引荐的门派。", "command", "", {}, "join <sect_id>", "sect_id", "scene", false},
        {"family", "group", "身份门派", "family", "查看散修/门派身份与阶位。", "command", "", {"sect"}, "family", "", "global", true},
        {"duty", "group", "身份事务", "duty", "查看当前散修或门派的成长事务。", "command", "", {}, "duty", "", "global", true},
        {"contribute", "group", "上交贡献", "contribute ", "上交当前背包材料，换取贡献或游历声望。", "command", "", {}, "contribute <物件>", "物件", "scene", false},
        {"claim", "group", "领取奖励", "claim ", "领取身份、阶段或周循环奖励。", "command", "", {}, "claim [id]", "id", "global", false},
        {"team", "group", "队伍操作", "team ", "创建、加入或管理队伍。", "command", "", {}, "team <create|join|info|leave>", "动作", "global", false},
        {"follow", "group", "跟随目标", "follow ", "标记要同行的目标。", "command", "", {}, "follow <玩家>", "玩家", "scene", false},
        {"guard", "group", "护卫目标", "guard ", "标记要照应的目标。", "command", "", {}, "guard <玩家>", "玩家", "scene", false},
        {"codex", "manual", "打开手册", "codex ", "查看资料手册条目。", "command", "", {}, "codex <分类> [条目]", "分类 条目", "global", false},
        {"event", "manual", "天地异象", "event", "查看近期世界大事。", "command", "", {}, "event", "", "global", true},
        {"save", "manual", "存档落笔", "save", "立即把当前角色状态写回存档。", "command", "", {}, "save", "", "global", true},
    };

    output->Clear();
    for(const auto& seed : kCatalog)
    {
        auto* item = output->Add();
        item->set_command_id(seed.id);
        item->set_category(seed.category);
        item->set_label(seed.label);
        item->set_command(seed.command);
        item->set_summary(seed.summary);
        item->set_composer_mode(seed.composer_mode);
        item->set_chat_channel(seed.chat_channel);
        for(const auto* alias : seed.aliases)
        {
            if(alias != nullptr && *alias != '\0')
            {
                item->add_aliases(alias);
            }
        }
        item->set_usage(seed.usage == nullptr ? "" : seed.usage);
        item->set_target_hint(seed.target_hint == nullptr ? "" : seed.target_hint);
        item->set_visibility_scope(seed.visibility_scope == nullptr ? "" : seed.visibility_scope);
        item->set_execute_immediately(seed.execute_immediately);
    }

    const auto* scene = current_scene(player);
    if(scene != nullptr && scene->board_available)
    {
        auto* item = output->Add();
        item->set_command_id("scene_board_tip");
        item->set_category("loops");
        item->set_label("此地可贴板");
        item->set_command("post ");
        item->set_summary("此地有留言板，可用 post 留字、read 读帖。");
        item->set_composer_mode("command");
        item->set_usage("post <题目>=<正文>");
        item->set_target_hint("题目=正文");
        item->set_visibility_scope("scene");
        item->set_execute_immediately(false);
    }
}

void MudGameRuntime::fill_gameplay_guidance(const MudPlayerState& player,
                                            mud::BootstrapResponse* response) const
{
    if(response == nullptr)
    {
        return;
    }
    response->set_stage_label(stage_label_for_player(player));
    response->set_newbie_protected(has_newbie_protection(player));
    response->set_newbie_protection_summary(newbie_protection_summary_for_player(player));
    response->set_recommended_loop(recommended_loop_for_player(player));
    response->clear_route_summaries();
    for(const auto& route : route_summaries_for_player(player))
    {
        fill_route_summary(route, response->add_route_summaries());
    }
    response->clear_weekly_events();
    for(const auto& event : weekly_events_for_player(player))
    {
        fill_weekly_event_summary(event, response->add_weekly_events());
    }
}

std::vector<std::string> MudGameRuntime::unlocked_routes_for_player(const MudPlayerState& player) const
{
    std::vector<std::string> routes{"loose_cultivator"};
    const auto* scene = current_scene(player);
    const auto has_completed = [&](const std::string& quest_id) {
        return std::any_of(player.quests.begin(), player.quests.end(), [&](const MudQuestState& quest) {
            return quest.quest_id == quest_id && quest.status == "completed";
        });
    };
    const auto add_route = [&](const std::string& route_id) {
        if(std::find(routes.begin(), routes.end(), route_id) == routes.end())
        {
            routes.push_back(route_id);
        }
    };

    if(player.sect_id == "qixuan_gate" || has_completed("backslope_wolf_skin") || has_completed("qixuan_herb") ||
       (scene != nullptr && scene->region_name == "七玄门"))
    {
        add_route("qixuan_gate");
    }
    if(player.sect_id == "huangfeng_valley" || has_completed("huangfeng_letter") ||
       has_completed("medicine_moss") || (scene != nullptr && scene->region_name.find("黄枫谷") != std::string::npos))
    {
        add_route("huangfeng_valley");
    }
    if(player.sect_id == "spirit_beast_mountain" || has_completed("spirit_feed_task") ||
       has_completed("spirit_bug_task") ||
       (scene != nullptr && scene->region_name.find("灵兽山") != std::string::npos))
    {
        add_route("spirit_beast_mountain");
    }
    return routes;
}

std::string MudGameRuntime::stage_label_for_player(const MudPlayerState& player) const
{
    if(player.realm_stage >= 6)
    {
        return "筑基初成";
    }
    if(player.realm_stage >= 5)
    {
        return "筑基准备";
    }
    if(player.realm_stage >= 4)
    {
        return "炼气冲刺";
    }
    if(player.realm_stage >= 2)
    {
        return "炼气稳固";
    }
    if(player.realm_stage >= 1)
    {
        return "炼气启程";
    }
    return "凡躯启程";
}

bool MudGameRuntime::has_newbie_protection(const MudPlayerState& player) const
{
    const auto now_s = mud_now_ms() / 1000;
    const auto opt_in_until_s = flag_int64_value(player, "pvp_opt_in_until_s", 0);
    if(opt_in_until_s > now_s)
    {
        return false;
    }
    const auto created_at_s = flag_int64_value(player, "created_at_s", 1);
    return (created_at_s > 1 && now_s - created_at_s < 3 * 60 * 60) || player.realm_stage < 4;
}

std::string MudGameRuntime::newbie_protection_summary_for_player(const MudPlayerState& player) const
{
    if(has_newbie_protection(player))
    {
        return "你仍处于新手保护期。三小时内或未到炼气中期前，在冲突区不会被其他玩家主动挑战。";
    }
    return "你的新手保护已解除，进入冲突区时请留意切磋与资源争夺。";
}

std::vector<MudRouteSummaryState> MudGameRuntime::route_summaries_for_player(const MudPlayerState& player) const
{
    const auto unlocked_routes = unlocked_routes_for_player(player);
    const auto route_unlocked = [&](const std::string& route_id) {
        return std::find(unlocked_routes.begin(), unlocked_routes.end(), route_id) != unlocked_routes.end();
    };

    const int loose_rep = flag_int_value(player, "loose_reputation", 0);
    std::string loose_rank = "行脚";
    if(loose_rep >= 720)
    {
        loose_rank = "洞主";
    }
    else if(loose_rep >= 420)
    {
        loose_rank = "采真";
    }
    else if(loose_rep >= 180)
    {
        loose_rank = "游方";
    }

    std::vector<MudRouteSummaryState> routes;
    routes.push_back({"loose_cultivator",
                      "散修",
                      player.sect_id.empty() ? ("当前阶位：" + loose_rank) : "仍可游历",
                      "散修路线强调采药、买卖、探路与公共委托，可不入门派一路打磨到筑基。",
                      player.sect_id.empty() ? "多做 board / travel / contribute，先把游历声望推到下一阶。" : "即使已入门派，也能继续经营散修委托与坊市关系。 "});
    routes.push_back({"qixuan_gate",
                      "七玄门",
                      player.sect_id == "qixuan_gate" ? "已加入" : (route_unlocked("qixuan_gate") ? "已接触" : "待接触"),
                      "七玄门偏凡俗江湖与新手历练，适合先打基础、熟路子、做护送与巡山。",
                      player.sect_id == "qixuan_gate" ? "继续做 duty 里的门内事务，稳步积累贡献。" : "先在七玄门和嘉元城完成引导事务，再考虑 join qixuan_gate。"});
    routes.push_back({"huangfeng_valley",
                      "黄枫谷",
                      player.sect_id == "huangfeng_valley" ? "已加入" : (route_unlocked("huangfeng_valley") ? "可转入" : "待接触"),
                      "黄枫谷提供药园、典籍与筑基散路线，是炼气后期转筑基的主力正道路线。",
                      player.sect_id == "huangfeng_valley" ? "优先做药园和外务事务，准备筑基所需的贡献与材料。" : "去黄枫谷外营接黄枫谷羽信与后续事务。"});
    routes.push_back({"spirit_beast_mountain",
                      "灵兽山",
                      player.sect_id == "spirit_beast_mountain" ? "已加入" : (route_unlocked("spirit_beast_mountain") ? "可转入" : "待接触"),
                      "灵兽山更偏向灵兽、灵虫、采药和喂养循环，收益稳定，适合长期经营。",
                      player.sect_id == "spirit_beast_mountain" ? "继续处理草料、灵虫和外岭差遣，稳住身份与材料来源。" : "先在灵兽山外门完成草料差与灵虫事务。 "});
    return routes;
}

std::vector<MudWeeklyEventSummaryState> MudGameRuntime::weekly_events_for_player(const MudPlayerState&) const
{
    return {
        {"blood_forbidden_open", "血禁开禁", "血色禁地每周会出现一次资源与妖兽同时活跃的窗口，单人可入，组队更稳。", "高危", "血禁石门、血雾沼泽、血兰谷", "wanted / board"},
        {"outer_port_tide", "外港海潮", "天南外港和乱星近海会在潮期迎来海材暴增，适合跑海猎与采珠。", "中危", "天南外港、群岛小埠、听潮坛", "travel / board"},
        {"black_reef_mining", "黑礁争采", "黑礁争采会刷新稀缺海材，也更容易引发玩家间的争夺与切磋。", "冲突", "黑礁、风暴航道、群岛礁路", "travel / wanted"}
    };
}

std::string MudGameRuntime::recommended_loop_for_player(const MudPlayerState& player) const
{
    const auto* scene = current_scene(player);
    if(player.realm_stage >= 5 && inventory_count(player, "foundation_pill") <= 0)
    {
        return "残区探禁";
    }
    if(player.sect_id.empty() && player.realm_stage < 3)
    {
        return has_loop_tag(scene, "采药炼丹") ? "采药炼丹" : "护送跑商";
    }
    if(!player.sect_id.empty() && has_loop_tag(scene, "门派事务"))
    {
        return "门派事务";
    }
    if(has_loop_tag(scene, "海猎采珠"))
    {
        return "海猎采珠";
    }
    if(has_loop_tag(scene, "巡山悬赏"))
    {
        return "巡山悬赏";
    }
    if(scene != nullptr && !scene->loop_tags.empty())
    {
        return scene->loop_tags.front();
    }
    return "采药炼丹";
}

std::string MudGameRuntime::identity_track_for_player(const MudPlayerState& player) const
{
    const auto* track = m_world->find_identity_track(player.sect_id.empty() ? "loose_cultivator" : player.sect_id);
    if(track != nullptr && !track->name.empty())
    {
        return track->name;
    }
    return player.sect_name.empty() ? "散修路" : player.sect_name;
}

int MudGameRuntime::rank_level_for_player(const MudPlayerState& player) const
{
    if(player.sect_id.empty())
    {
        const int loose_rep = flag_int_value(player, "loose_reputation", 0);
        if(loose_rep >= 720)
        {
            return 4;
        }
        if(loose_rep >= 420)
        {
            return 3;
        }
        if(loose_rep >= 180)
        {
            return 2;
        }
        return 1;
    }

    if(const auto* track = m_world->find_identity_track(player.sect_id); track != nullptr)
    {
        for(size_t index = 0; index < track->ranks.size(); ++index)
        {
            if(track->ranks[index] == player.sect_rank)
            {
                return static_cast<int>(index + 1);
            }
        }
    }
    return player.sect_rank.empty() ? 0 : 1;
}

std::string MudGameRuntime::contribution_state_for_player(const MudPlayerState& player) const
{
    if(player.sect_id.empty())
    {
        const int loose_rep = flag_int_value(player, "loose_reputation", 0);
        if(loose_rep >= 720)
        {
            return "游历资历已稳，可自立洞府。";
        }
        if(loose_rep >= 420)
        {
            return "游历资历渐成，再积攒 300 声望便可冲击洞主。";
        }
        if(loose_rep >= 180)
        {
            return "已成游方散人，再积攒 240 声望可进采真。";
        }
        return "仍在行脚起步，再积攒 180 声望可转游方。";
    }

    const int64_t contribution = sect_contribution_for_player(player);
    static const std::vector<int64_t> kThresholds = {0, 180, 420, 900};
    if(const auto* track = m_world->find_identity_track(player.sect_id); track != nullptr)
    {
        for(size_t index = 0; index < track->ranks.size() && index < kThresholds.size(); ++index)
        {
            if(track->ranks[index] != player.sect_rank)
            {
                continue;
            }
            if(index + 1 >= track->ranks.size() || index + 1 >= kThresholds.size())
            {
                return "门内贡献 " + std::to_string(contribution) + "，已达当前阶段上限。";
            }
            const auto need_more = std::max<int64_t>(0, kThresholds[index + 1] - contribution);
            return "门内贡献 " + std::to_string(contribution) + "，距「" + track->ranks[index + 1] + "」尚差 " +
                   std::to_string(need_more) + "。";
        }
    }
    return "门内贡献 " + std::to_string(contribution) + "。";
}

std::string MudGameRuntime::reputation_state_for_player(const MudPlayerState& player) const
{
    if(player.sect_id.empty())
    {
        return "散修声望 " + std::to_string(flag_int_value(player, "loose_reputation", 0)) + " · " +
               stage_label_for_player(player);
    }
    return (player.sect_name.empty() ? std::string("门内") : player.sect_name) + " · " +
           (player.sect_rank.empty() ? std::string("记名") : player.sect_rank);
}

int MudGameRuntime::unread_board_count_for_player(const MudPlayerState& player) const
{
    int unread_count = 0;
    for(auto iter = m_events.rbegin(); iter != m_events.rend(); ++iter)
    {
        if(iter->type != "board_post")
        {
            continue;
        }
        if(player.flags.find("board:hidden:" + std::to_string(iter->event_id)) != player.flags.end())
        {
            continue;
        }
        ++unread_count;
        if(unread_count >= 99)
        {
            break;
        }
    }
    return unread_count;
}

std::vector<MudSummaryEntry> MudGameRuntime::board_entries_for_player(const MudPlayerState& player,
                                                                      const MudSceneConfig* scene) const
{
    std::vector<MudSummaryEntry> entries;
    if(scene == nullptr)
    {
        return entries;
    }

    const auto push_entry = [&](MudSummaryEntry entry) {
        if(entry.entry_id.empty())
        {
            return;
        }
        if(std::any_of(entries.begin(), entries.end(), [&](const MudSummaryEntry& current) {
               return current.entry_id == entry.entry_id;
           }))
        {
            return;
        }
        entries.push_back(std::move(entry));
    };

    for(const auto& npc_id : scene->npc_ids)
    {
        const auto* npc = m_world->find_npc(npc_id);
        if(npc == nullptr)
        {
            continue;
        }
        for(const auto& quest_id : npc->quest_ids)
        {
            const auto* quest = m_world->find_quest(quest_id);
            if(quest == nullptr)
            {
                continue;
            }
            const auto* quest_state = find_quest_state(player, quest_id);
            std::string status = "可接";
            std::string command = "accept " + quest_id;
            std::string summary = quest->description;
            if(quest_state != nullptr)
            {
                if(quest_state->status == "completed")
                {
                    continue;
                }
                if(quest_state->status == "active")
                {
                    if(quest_state->progress >= quest->required_item_count)
                    {
                        status = "可交";
                        command = "submit " + quest_id;
                        summary = "材料已齐，可去交付任务。";
                    }
                    else
                    {
                        status = "进行中";
                        summary = "当前进度 " + std::to_string(quest_state->progress) + "/" +
                                  std::to_string(quest->required_item_count) + "。";
                    }
                }
            }
            push_entry({quest_id,
                        quest->title,
                        summary,
                        status,
                        "委托",
                        command,
                        npc->name,
                        "灵石 " + std::to_string(quest->reward_spirit_stone) + " / 修为 " + std::to_string(quest->reward_exp)});
        }
    }

    if(has_loop_tag(scene, "采药炼丹") && !scene->resource_node_ids.empty())
    {
        const auto* node = m_world->find_resource_node(scene->resource_node_ids.front());
        if(node != nullptr)
        {
            push_entry({"loop_gather_" + node->node_id,
                        "采药炼丹",
                        "低风险起步循环，适合积累材料、丹药和基础收入。",
                        "5-8 分钟",
                        "循环",
                        "harvest " + node->node_id,
                        scene->name,
                        item_with_count_label(m_world.get(), node->drop_item_id, node->drop_item_count)});
        }
    }
    if(has_loop_tag(scene, "护送跑商"))
    {
        push_entry({"loop_trade_" + scene->scene_id,
                    "护送跑商",
                    "沿官道、驿站和港口跑线，稳定积累灵石、路引和图鉴见闻。",
                    "8-12 分钟",
                    "循环",
                    "travel",
                    scene->region_name,
                    "灵石 / 游历"});
    }
    if(has_loop_tag(scene, "巡山悬赏") && !scene->monster_ids.empty())
    {
        if(const auto* monster = m_world->find_monster(scene->monster_ids.front()); monster != nullptr)
        {
            push_entry({"loop_bounty_" + monster->monster_id,
                        "巡山悬赏",
                        "以附近妖兽为目标，顺手积累修为、掉落和赏格。",
                        "6-10 分钟",
                        "循环",
                        "fight " + monster->name,
                        scene->name,
                        "修为 " + std::to_string(monster->reward_exp)});
        }
    }
    if(has_loop_tag(scene, "门派事务"))
    {
        push_entry({"loop_duty_" + scene->scene_id,
                    "门派事务",
                    "当前区域有身份相关事务可做，适合积累贡献与晋阶资历。",
                    "6-10 分钟",
                    "循环",
                    "duty",
                    scene->region_name,
                    "贡献 / 资历"});
    }
    if(has_loop_tag(scene, "海猎采珠"))
    {
        push_entry({"loop_sea_" + scene->scene_id,
                    "海猎采珠",
                    "海猎适合中后期稳定刷海材、药引和稀缺海货。",
                    "10-15 分钟",
                    "循环",
                    "wanted",
                    scene->region_name,
                    "海材 / 药引"});
    }
    if(has_loop_tag(scene, "残区探禁"))
    {
        push_entry({"loop_ruin_" + scene->scene_id,
                    "残区探禁",
                    "高风险高收益循环，主要产出筑基准备材料和禁制见闻。",
                    "12-20 分钟",
                    "循环",
                    "wanted",
                    scene->region_name,
                    "筑基材料 / 禁制条目"});
    }

    return entries;
}

std::vector<MudSummaryEntry> MudGameRuntime::work_entries_for_player(const MudPlayerState& player,
                                                                     const MudSceneConfig* scene) const
{
    std::vector<MudSummaryEntry> entries;
    if(scene == nullptr)
    {
        return entries;
    }

    for(const auto* job : m_world->jobs_for_scene(scene->scene_id))
    {
        if(job == nullptr)
        {
            continue;
        }

        const auto* quest = job->related_quest_id.empty() ? nullptr : m_world->find_quest(job->related_quest_id);
        const auto* quest_state =
            job->related_quest_id.empty() ? nullptr : find_quest_state(player, job->related_quest_id);

        std::string status = job->repeatable ? "可做" : "待接";
        std::string summary = job->description.empty() ? job->summary : job->description;
        std::string command = mud_trim(job->command_hint);
        if(command.empty())
        {
            if(const auto* issuer = m_world->find_npc(job->issuer_npc_id); issuer != nullptr)
            {
                command = "ask " + issuer->name + " about rumor";
            }
            else if(!job->issuer_npc_id.empty())
            {
                command = "ask " + job->issuer_npc_id + " about rumor";
            }
            else
            {
                command = "help work";
            }
        }
        if(quest != nullptr)
        {
            if(quest_state == nullptr)
            {
                status = quest->repeatable ? "可做" : "待接";
            }
            else if(quest_state->status == "active")
            {
                if(quest_state->progress >= quest->required_item_count && quest->required_item_count > 0)
                {
                    status = "可交";
                    command = "submit " + quest->quest_id;
                    summary = "这桩营生眼下已齐，回去交割即可。";
                }
                else
                {
                    status = "进行中";
                    command = "journal";
                    summary = "当前进度 " + std::to_string(quest_state->progress) + "/" +
                              std::to_string(quest->required_item_count) + "，先把这一轮活办完。";
                }
            }
            else if(quest_state->status == "completed")
            {
                status = quest->repeatable ? "可重做" : "已了";
            }
        }

        std::string issuer_hint = scene->name;
        if(const auto* issuer = m_world->find_npc(job->issuer_npc_id); issuer != nullptr)
        {
            issuer_hint = issuer->name;
        }

        entries.push_back({job->job_id,
                           job->title,
                           summary,
                           status,
                           job->kind.empty() ? std::string("营生") : job->kind,
                           command,
                           issuer_hint,
                           job->reward_summary});
    }

    if(entries.empty())
    {
        entries.push_back({"work_none",
                           "此地暂无营生",
                           "眼前没有稳定可开的工作，不妨先看 board 或 ask 人物 about rumor。",
                           "空闲",
                           "营生",
                           "board",
                           scene->name,
                           ""});
    }
    return entries;
}

std::vector<MudSummaryEntry> MudGameRuntime::duty_entries_for_player(const MudPlayerState& player,
                                                                     const MudSceneConfig* scene) const
{
    std::vector<MudSummaryEntry> entries;
    if(player.sect_id.empty())
    {
        const int loose_rep = flag_int_value(player, "loose_reputation", 0);
        std::string current_rank = "行脚";
        int next_threshold = 180;
        std::string next_rank = "游方";
        if(loose_rep >= 720)
        {
            current_rank = "洞主";
            next_threshold = 720;
            next_rank.clear();
        }
        else if(loose_rep >= 420)
        {
            current_rank = "采真";
            next_threshold = 720;
            next_rank = "洞主";
        }
        else if(loose_rep >= 180)
        {
            current_rank = "游方";
            next_threshold = 420;
            next_rank = "采真";
        }
        entries.push_back({"loose_rank",
                           "散修阶位",
                           "当前散修身份为「" + current_rank + "」。",
                           "游历声望 " + std::to_string(loose_rep),
                           "身份",
                           "",
                           "太南散修坊 / 各地坊市",
                           next_rank.empty() ? "已达当前阶段上限" : ("下阶：" + next_rank + " 需声望 " + std::to_string(next_threshold))});
        entries.push_back({"loose_board",
                           "散修事务",
                           "散修路线依赖公共委托、采药、跑商与探路来稳步滚动收益。",
                           "推荐",
                           "事务",
                           "board",
                           scene == nullptr ? "公共世界" : scene->region_name,
                           "灵石 / 手册 / 声望"});
        entries.push_back({"loose_claim",
                           "阶段奖励",
                           "若有阶段奖励可领，可在此统一领取补给与阶段支持。",
                           "可检查",
                           "事务",
                           "claim",
                           "当前角色",
                           "补给 / 丹药 / 灵石"});
        return entries;
    }

    const auto* sect = m_world->find_sect(player.sect_id);
    const int64_t contribution = sect_contribution_for_player(player);
    std::vector<std::string> ranks = sect == nullptr ? std::vector<std::string>{player.sect_rank}
                                                     : (sect->rank_titles.empty() ? std::vector<std::string>{sect->rank_title}
                                                                                  : sect->rank_titles);
    std::vector<int64_t> thresholds{0, 180, 420, 900};
    std::string next_rank;
    int64_t next_threshold = 0;
    for(size_t index = 0; index < ranks.size() && index < thresholds.size(); ++index)
    {
        if(player.sect_rank == ranks[index] && index + 1 < ranks.size() && index + 1 < thresholds.size())
        {
            next_rank = ranks[index + 1];
            next_threshold = thresholds[index + 1];
            break;
        }
    }
    entries.push_back({"sect_rank",
                       "门派阶位",
                       "你当前在「" + player.sect_name + "」的身份为「" + player.sect_rank + "」。",
                       "贡献 " + std::to_string(contribution),
                       "身份",
                       "",
                       player.sect_name,
                       next_rank.empty() ? "已到当前阶段上限" : ("下阶：" + next_rank + " 需贡献 " + std::to_string(next_threshold))});
    entries.push_back({"sect_contribute",
                       "上交贡献物",
                       "把当前背包里值钱或对门内有用的材料上交，可加快晋阶。",
                       "可执行",
                       "事务",
                       "contribute ",
                       scene == nullptr ? player.sect_name : scene->name,
                       "贡献 / 声望"});
    entries.push_back({"sect_board",
                       "门内事务",
                       "继续处理药园、巡查、外务、喂养和悬赏，身份成长会更稳。",
                       "推荐",
                       "事务",
                       "board",
                       player.sect_name,
                       "贡献 / 材料 / 筑基准备"});
    return entries;
}

std::vector<MudSummaryEntry> MudGameRuntime::wanted_entries_for_player(const MudPlayerState& player,
                                                                       const MudSceneConfig* scene) const
{
    std::vector<MudSummaryEntry> entries;
    if(scene == nullptr)
    {
        return entries;
    }
    for(const auto& monster_id : scene->monster_ids)
    {
        const auto* monster = m_world->find_monster(monster_id);
        if(monster == nullptr)
        {
            continue;
        }
        entries.push_back({monster->monster_id,
                           monster->name,
                           monster->description,
                           scene->risk_level.empty() ? "目标" : scene->risk_level,
                           "悬赏",
                           "fight " + monster->name,
                           scene->name,
                           "修为 " + std::to_string(monster->reward_exp) + " / " +
                               item_with_count_label(m_world.get(), monster->drop_item_id, monster->drop_item_count)});
    }
    if(entries.empty())
    {
        entries.push_back({"wanted_none",
                           "暂无悬赏",
                           "当前房间暂无明确悬赏目标，可先用 board 查看其他循环。",
                           "空闲",
                           "悬赏",
                           "board",
                           scene->region_name,
                           ""});
    }
    return entries;
}

std::vector<MudSummaryEntry> MudGameRuntime::travel_entries_for_player(const MudPlayerState& player,
                                                                       const MudSceneConfig* scene) const
{
    std::vector<MudSummaryEntry> entries;
    if(scene != nullptr)
    {
        for(const auto& exit : scene->exits)
        {
            const auto* target = m_world->find_scene(exit.second);
            entries.push_back({"travel_exit_" + exit.first,
                               direction_display_name(exit.first),
                               "沿当前道路前往下一处房间。",
                               "当前可行",
                               "路引",
                               "go " + exit.first,
                               target == nullptr ? exit.second : target->name,
                               target == nullptr ? "" : target->region_name});
        }
    }
    for(const auto& region : unlocked_regions_for_player(player))
    {
        entries.push_back({"travel_region_" + region,
                           region,
                           "你已接触此片区域，可沿当地路引继续深入。",
                           "已开路",
                           "路引",
                           "",
                           region,
                           ""});
    }
    return entries;
}

std::vector<MudSummaryEntry> MudGameRuntime::claim_entries_for_player(const MudPlayerState& player) const
{
    std::vector<MudSummaryEntry> entries;
    if(flag_int_value(player, "claim:starter_supplies", 0) == 0)
    {
        entries.push_back({"starter_supplies", "启程补给", "领取一份新手补给，帮助你更稳地跑完前期循环。", "可领取", "奖励", "claim starter_supplies", "当前角色", "小回气散 x2"});
    }
    if(player.realm_stage >= 3 && flag_int_value(player, "claim:steady_income", 0) == 0)
    {
        entries.push_back({"steady_income", "稳路资粮", "炼气中前期的稳定收益奖励，帮助你继续滚动循环。", "可领取", "奖励", "claim steady_income", "当前角色", "游方散 x2"});
    }
    if((sect_contribution_for_player(player) >= 260 || flag_int_value(player, "loose_reputation", 0) >= 260) &&
       flag_int_value(player, "claim:identity_credit", 0) == 0)
    {
        entries.push_back({"identity_credit", "身份资历", "身份路线达到一个节点后，可领取额外灵石与事务支持。", "可领取", "奖励", "claim identity_credit", "当前角色", "灵石 120"});
    }
    if(player.realm_stage >= 5 && inventory_count(player, "foundation_pill") <= 0 &&
       flag_int_value(player, "claim:foundation_support", 0) == 0)
    {
        entries.push_back({"foundation_support", "筑基支持", "炼气后期可领取一份筑基支持物资，用于最后冲刺。", "可领取", "奖励", "claim foundation_support", "当前角色", "筑基散 x1"});
    }
    return entries;
}

std::vector<const MudEventEnvelope*> MudGameRuntime::board_posts_for_scene(const MudPlayerState& player,
                                                                           const std::string& scene_id,
                                                                           int limit) const
{
    std::vector<const MudEventEnvelope*> posts;
    if(scene_id.empty())
    {
        return posts;
    }

    const int normalized_limit = std::clamp(limit <= 0 ? 8 : limit, 1, 32);
    for(auto iter = m_events.rbegin(); iter != m_events.rend() && static_cast<int>(posts.size()) < normalized_limit; ++iter)
    {
        if(iter->type != "board_post")
        {
            continue;
        }
        if(player.flags.find("board:hidden:" + std::to_string(iter->event_id)) != player.flags.end())
        {
            continue;
        }
        const auto [post_scene_id, subject] = parse_board_post_title(iter->title);
        if(post_scene_id != scene_id || subject.empty())
        {
            continue;
        }
        posts.push_back(&(*iter));
    }
    return posts;
}

const MudHelpTopicConfig* MudGameRuntime::match_help_topic(const std::string& key) const
{
    const auto query = mud_trim(key);
    if(query.empty())
    {
        return m_world->find_help_topic("newbie");
    }

    if(const auto* direct = m_world->find_help_topic(query); direct != nullptr)
    {
        return direct;
    }

    const auto query_lower = mud_to_lower_ascii(query);
    for(const auto& topic : m_world->help_topics())
    {
        const auto topic_id_lower = mud_to_lower_ascii(topic.topic_id);
        const auto title_lower = mud_to_lower_ascii(topic.title);
        if(topic_id_lower == query_lower || title_lower == query_lower ||
           topic_id_lower.find(query_lower) != std::string::npos ||
           title_lower.find(query_lower) != std::string::npos)
        {
            return m_world->find_help_topic(topic.topic_id);
        }
        for(const auto& keyword : topic.keywords)
        {
            const auto keyword_lower = mud_to_lower_ascii(keyword);
            if(keyword == query || keyword_lower == query_lower || keyword_lower.find(query_lower) != std::string::npos)
            {
                return m_world->find_help_topic(topic.topic_id);
            }
        }
        for(const auto& command : topic.related_commands)
        {
            const auto command_lower = mud_to_lower_ascii(command);
            if(command_lower == query_lower)
            {
                return m_world->find_help_topic(topic.topic_id);
            }
        }
    }
    return nullptr;
}

std::string MudGameRuntime::resource_refresh_summary_for_scene(const MudPlayerState& player,
                                                               const MudSceneConfig& scene) const
{
    if(scene.resource_node_ids.empty())
    {
        return "此地暂无资源点刷新。";
    }
    const auto now = mud_now_ms();
    int ready_count = 0;
    for(const auto& node_id : scene.resource_node_ids)
    {
        const auto* node = m_world->find_resource_node(node_id);
        if(node == nullptr)
        {
            continue;
        }
        const auto last_ms = flag_int64_value(player, "harvest:" + node->node_id, 0);
        if(last_ms <= 0 || now - last_ms >= node->cooldown_ms)
        {
            ++ready_count;
        }
    }
    return "资源点 " + std::to_string(scene.resource_node_ids.size()) + " 处，可采 " + std::to_string(ready_count) + " 处。";
}

void MudGameRuntime::maybe_emit_world_event()
{
    const int64_t now = mud_now_ms();
    if(m_last_world_event_ms != 0 && now - m_last_world_event_ms < 45000)
    {
        return;
    }

    static const std::vector<std::pair<std::string, std::string>> kWorldEvents = {
        {"血禁开禁", "血色禁地本周灵禁短暂松动，单人可闯，组队更稳，血兰与虫材都更活跃。"},
        {"外港海潮", "天南外港与乱星近海迎来大潮期，海灵藻、珠蚌和潮砂的收益同时抬升。"},
        {"黑礁争采", "黑礁深处冒出新的稀缺矿材与海齿，收益极高，也更容易引发同道争夺。"}
    };

    if(kWorldEvents.empty())
    {
        return;
    }

    const auto& event = kWorldEvents[m_world_event_cursor % kWorldEvents.size()];
    ++m_world_event_cursor;
    m_last_world_event_ms = now;
    append_event("",
                 "world",
                 event.first,
                 event.second,
                 nullptr);
}

std::vector<std::string> MudGameRuntime::unlocked_regions_for_player(const MudPlayerState& player) const
{
    std::vector<std::string> regions;
    const auto add_region = [&](const std::string& value) {
        if(value.empty())
        {
            return;
        }
        if(std::find(regions.begin(), regions.end(), value) == regions.end())
        {
            regions.push_back(value);
        }
    };

    add_region("七玄门");
    if(const auto* scene = current_scene(player); scene != nullptr)
    {
        add_region(scene->region_name);
    }
    for(const auto& quest : player.quests)
    {
        if(quest.status != "completed")
        {
            continue;
        }
        if(quest.quest_id == "qixuan_herb")
        {
            add_region("嘉元城");
        }
        if(quest.quest_id == "tainan_snake")
        {
            add_region("太南谷");
        }
        if(quest.quest_id == "huangfeng_letter")
        {
            add_region("黄枫谷");
        }
        if(quest.quest_id == "blood_forbidden_token")
        {
            add_region("血色禁地");
        }
        if(quest.quest_id == "chaos_sea_chart")
        {
            add_region("乱星海");
        }
        if(quest.quest_id == "xutian_key")
        {
            add_region("虚天殿");
        }
    }
    return regions;
}

std::string MudGameRuntime::progression_chapter_for_player(const MudPlayerState& player) const
{
    bool has_xutian = false;
    bool has_chaos_sea = false;
    bool has_blood = false;
    bool has_huangfeng = false;
    bool has_tainan = false;
    for(const auto& quest : player.quests)
    {
        if(quest.status != "completed")
        {
            continue;
        }
        has_tainan = has_tainan || quest.quest_id == "tainan_snake";
        has_huangfeng = has_huangfeng || quest.quest_id == "huangfeng_letter";
        has_blood = has_blood || quest.quest_id == "blood_forbidden_token";
        has_chaos_sea = has_chaos_sea || quest.quest_id == "chaos_sea_chart";
        has_xutian = has_xutian || quest.quest_id == "xutian_key";
    }

    if(has_xutian)
    {
        return "虚天殿风云";
    }
    if(has_chaos_sea)
    {
        return "乱星海启程";
    }
    if(has_blood)
    {
        return "血色禁地试炼";
    }
    if(has_huangfeng)
    {
        return "黄枫谷内门试炼";
    }
    if(has_tainan)
    {
        return "太南小会";
    }
    return "七玄门启程";
}

int64_t MudGameRuntime::sect_contribution_for_player(const MudPlayerState& player) const
{
    int64_t contribution = std::max(0, flag_int_value(player, "manual_contribution", 0));
    for(const auto& quest_state : player.quests)
    {
        if(quest_state.status != "completed")
        {
            continue;
        }
        const auto* quest = m_world->find_quest(quest_state.quest_id);
        if(quest == nullptr)
        {
            continue;
        }
        if(!player.sect_id.empty() && (!quest->reward_sect_id.empty() && quest->reward_sect_id != player.sect_id))
        {
            continue;
        }
        contribution += std::max<int64_t>(20, quest->reward_exp / 2 + quest->reward_spirit_stone);
    }
    return contribution;
}

int64_t MudGameRuntime::leaderboard_score(MudLeaderboardType leaderboard_type,
                                          const MudPlayerState& player) const
{
    const int64_t completed_quests = static_cast<int64_t>(std::count_if(
        player.quests.begin(), player.quests.end(), [](const MudQuestState& quest) { return quest.status == "completed"; }));
    switch(leaderboard_type)
    {
    case MudLeaderboardType::wealth:
        return player.spirit_stone;
    case MudLeaderboardType::combat:
        return static_cast<int64_t>(player.attack_power + player.defense_power + player.level * 10);
    case MudLeaderboardType::alchemy:
        return static_cast<int64_t>(player.profession.alchemy_level) * 100000 +
               static_cast<int64_t>(player.recipes.size()) * 1000 +
               flag_int_value(player, "brew_success_count", 0);
    case MudLeaderboardType::travel:
        return static_cast<int64_t>(player.profession.exploration_level) * 100000 +
               static_cast<int64_t>(player.codex_entries.size()) * 1000 + completed_quests;
    case MudLeaderboardType::bounty:
        return flag_int_value(player, "bounty_score", 0);
    case MudLeaderboardType::chief:
        return player.sect_id.empty() ? 0 : (sect_contribution_for_player(player) + player.realm_stage * 1000);
    case MudLeaderboardType::realm:
    default:
        return static_cast<int64_t>(player.realm_stage) * 1000000 + player.exp;
    }
}

std::string MudGameRuntime::leaderboard_extra(MudLeaderboardType leaderboard_type,
                                              const MudPlayerState& player) const
{
    switch(leaderboard_type)
    {
    case MudLeaderboardType::alchemy:
        return "丹道 " + std::to_string(player.profession.alchemy_level) + "阶";
    case MudLeaderboardType::travel:
        return "手册 " + std::to_string(player.codex_entries.size()) + " 条";
    case MudLeaderboardType::bounty:
        return "赏格 " + std::to_string(flag_int_value(player, "bounty_score", 0));
    case MudLeaderboardType::chief:
        return player.sect_name.empty() ? "散修" : player.sect_name + " · " + player.sect_rank;
    case MudLeaderboardType::combat:
        return "战力 " + std::to_string(player.attack_power + player.defense_power);
    case MudLeaderboardType::wealth:
        return "灵石 " + std::to_string(player.spirit_stone);
    case MudLeaderboardType::realm:
    default:
        return player.realm_name;
    }
}

MudCommandExecution MudGameRuntime::run_command(MudPlayerState* player,
                                                const std::string& command)
{
    return execute_command(player, command);
}

coro_task_t MudGameRuntime::bootstrap_async(const std::string& account,
                                            mud::BootstrapResponse* response)
{
    if(response == nullptr)
    {
        co_return;
    }

    if(m_repository == nullptr)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                     http_code_message::gateway::message::kMudPlayerRepositoryUnavailable);
        co_return;
    }

    auto* load_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_repository->load_player(account));
    if(load_result == nullptr || !load_result->success)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                     "load mud player failed");
        co_return;
    }

    response->set_need_create_character(!load_result->found);
    if(!load_result->found || !load_result->player.has_value())
    {
        MudPlayerState preview;
        preview.account = account;
        preview.location_scene_id = m_world->defaults().starting_scene_id;
        fill_scene_snapshot(preview, response->mutable_scene());
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kSuccess,
                                                     "character not found");
        response->set_next_event_id(m_next_event_id == 0 ? 0 : (m_next_event_id - 1));
        co_return;
    }

    fill_player_snapshot(*load_result->player, response->mutable_player());
    fill_scene_snapshot(*load_result->player, response->mutable_scene());
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
    response->set_next_event_id(m_next_event_id == 0 ? 0 : (m_next_event_id - 1));
}

coro_task_t MudGameRuntime::create_character_async(const std::string& account,
                                                   const std::string& character_name,
                                                   const std::string& origin_id,
                                                   const std::string& background_id,
                                                   mud::CharacterCreateResponse* response)
{
    if(response == nullptr)
    {
        co_return;
    }

    auto normalized_name = mud_trim(character_name);
    if(normalized_name.empty() || normalized_name.size() > 24)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kCharacterAlreadyExistsOrInvalidInput,
                                                     "invalid character name");
        co_return;
    }

    auto* load_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_repository->load_player(account));
    if(load_result == nullptr || !load_result->success)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                     "load mud player failed");
        co_return;
    }
    if(load_result->found)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kCharacterAlreadyExistsOrInvalidInput,
                                                     "character already exists");
        co_return;
    }

    auto player = make_default_player(account, normalized_name, origin_id, background_id);
    auto* create_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_repository->create_player(player));
    if(create_result == nullptr || !create_result->success || !create_result->create_ok)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                     "create mud player failed");
        co_return;
    }

    std::vector<MudEventEnvelope> events;
    append_event(account,
                 "system",
                 "踏入修仙路",
                 normalized_name + "自七玄门山脚启程，正式踏上凡人修仙之路。",
                 &events);
    fill_player_snapshot(player, response->mutable_player());
    fill_scene_snapshot(player, response->mutable_scene());
    add_events_to_response(events, response->mutable_events());
    response->set_next_event_id(events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1)) : events.back().event_id);
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 "character created");
}

coro_task_t MudGameRuntime::execute_command_async(const std::string& account,
                                                  const std::string& command,
                                                  mud::CommandExecuteResponse* response)
{
    if(response == nullptr)
    {
        co_return;
    }

    auto* load_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_repository->load_player(account));
    if(load_result == nullptr || !load_result->success)
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                     "load mud player failed");
        co_return;
    }
    if(!load_result->found || !load_result->player.has_value())
    {
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kCharacterNotFound,
                                                     http_code_message::gateway::message::kCharacterNotFound);
        co_return;
    }

    auto player = *load_result->player;
    auto execution = execute_command(&player, command);
    auto* result = response->mutable_result();
    result->set_command(command);
    result->set_success(execution.success);
    result->set_title(execution.title);
    result->set_summary(execution.summary);
    result->set_recommended_poll_interval_ms(execution.recommended_poll_interval_ms);
    result->clear_hints();
    for(const auto& hint : execution.hints)
    {
        result->add_hints(hint);
    }

    if(execution.success)
    {
        auto* save_result = dynamic_cast<MudPlayerRepositoryOpResult*>(co_await m_repository->save_player(player));
        if(save_result == nullptr || !save_result->success || !save_result->save_ok)
        {
            http_code_message::gateway::set_code_message(response,
                                                         http_code_message::gateway::code::kMudPlayerRepositoryUnavailable,
                                                         "save mud player failed");
            co_return;
        }
    }

    fill_player_snapshot(player, response->mutable_player());
    fill_scene_snapshot(player, response->mutable_scene());
    add_events_to_response(execution.events, response->mutable_events());
    response->set_next_event_id(execution.events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1)) : execution.events.back().event_id);
    http_code_message::gateway::set_code_message(response,
                                                 execution.success ? http_code_message::gateway::code::kSuccess
                                                                   : http_code_message::gateway::code::kInvalidMudCommand,
                                                 execution.summary);
}

coro_task_t MudGameRuntime::pull_feed_async(const std::string& account,
                                            uint64_t after_event_id,
                                            int limit,
                                            mud::FeedPullResponse* response)
{
    if(response == nullptr)
    {
        co_return;
    }

    const int normalized_limit = std::clamp(limit <= 0 ? 50 : limit, 1, 100);
    std::vector<MudEventEnvelope> events;
    events.reserve(static_cast<size_t>(normalized_limit));
    for(const auto& event : m_events)
    {
        if(event.event_id <= after_event_id)
        {
            continue;
        }
        if(!event.target_account.empty() && event.target_account != account)
        {
            continue;
        }
        events.push_back(event);
        if(static_cast<int>(events.size()) >= normalized_limit)
        {
            break;
        }
    }

    add_events_to_response(events, response->mutable_events());
    response->set_next_event_id(events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1)) : events.back().event_id);
    response->set_recommended_poll_interval_ms(1500);
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
}

MudCommandExecution MudGameRuntime::execute_command(MudPlayerState* player,
                                                    const std::string& command_text)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "命令失败";
        execution.summary = "玩家状态为空";
        return execution;
    }

    normalize_player_state(player);

    auto parsed = parse_mud_command(command_text);
    if(parsed.verb.empty())
    {
        execution.title = "命令为空";
        execution.summary = "请输入修仙指令。";
        execution.hints = {"例如：look", "例如：go north", "例如：fight 青木狼"};
        return execution;
    }

    auto verb = parsed.verb;
    if(verb == "i" || verb == "inv")
    {
        verb = "bag";
    }
    else if(verb == "quest" || verb == "quests")
    {
        verb = "tasks";
    }
    else if(verb == "cmds")
    {
        verb = "commands";
    }
    else if(verb == "?")
    {
        verb = "help";
    }

    if(verb == "look")
    {
        return execute_look(player);
    }
    if(verb == "help")
    {
        return execute_help(*player, parsed.args);
    }
    if(verb == "commands")
    {
        return execute_commands(*player);
    }
    if(verb == "newbie")
    {
        return execute_newbie(*player);
    }
    if(verb == "here")
    {
        return execute_look(player);
    }
    if(verb == "listen")
    {
        return execute_listen(*player);
    }
    if(verb == "map")
    {
        return execute_map(player);
    }
    if(verb == "rumor")
    {
        return execute_rumor(*player);
    }
    if(verb == "who")
    {
        return execute_who(*player);
    }
    if(verb == "hp")
    {
        return execute_hp(*player);
    }
    if(verb == "score")
    {
        return execute_score(*player);
    }
    if(verb == "rank")
    {
        return execute_rank(*player, parsed.args);
    }
    if(verb == "board")
    {
        return execute_board(*player);
    }
    if(verb == "read")
    {
        return execute_read(player, parsed.args);
    }
    if(verb == "post")
    {
        return execute_post(player, parsed.raw_args);
    }
    if(verb == "discard")
    {
        return execute_discard(player, parsed.args);
    }
    if(verb == "work")
    {
        return execute_work(*player);
    }
    if(verb == "duty")
    {
        return execute_duty(*player);
    }
    if(verb == "wanted")
    {
        return execute_wanted(*player);
    }
    if(verb == "travel")
    {
        return execute_travel(*player);
    }
    if(verb == "claim")
    {
        return execute_claim(player, parsed.args);
    }
    if(verb == "tasks")
    {
        return execute_tasks(*player);
    }
    if(verb == "journal")
    {
        return execute_journal(*player);
    }
    if(verb == "skills")
    {
        return execute_skills(*player);
    }
    if(verb == "spells")
    {
        return execute_spells(*player);
    }
    if(verb == "family")
    {
        return execute_family(*player);
    }
    if(verb == "bag" || verb == "inventory")
    {
        return execute_bag(*player);
    }
    if(verb == "inspect")
    {
        return execute_inspect(player, parsed.args);
    }
    if(verb == "go")
    {
        return execute_go(player, parsed.args);
    }
    if(verb == "talk")
    {
        return execute_talk(player, parsed.args);
    }
    if(verb == "ask")
    {
        return execute_ask(player, parsed.args);
    }
    if(verb == "accept")
    {
        return execute_accept(player, parsed.args);
    }
    if(verb == "submit")
    {
        return execute_submit(player, parsed.args);
    }
    if(verb == "fight")
    {
        return execute_fight(player, parsed.args);
    }
    if(verb == "use")
    {
        return execute_use(player, parsed.args);
    }
    if(verb == "loot")
    {
        return execute_loot(player, parsed.args);
    }
    if(verb == "harvest")
    {
        return execute_harvest(player, parsed.args);
    }
    if(verb == "cast")
    {
        return execute_cast(player, parsed.args);
    }
    if(verb == "flee")
    {
        return execute_flee();
    }
    if(verb == "practice")
    {
        return execute_practice(player, parsed.args);
    }
    if(verb == "meditate")
    {
        return execute_meditate(player);
    }
    if(verb == "breakthrough")
    {
        return execute_breakthrough(player);
    }
    if(verb == "brew")
    {
        return execute_brew(player, parsed.args);
    }
    if(verb == "buy")
    {
        return execute_buy(player, parsed.args);
    }
    if(verb == "sell")
    {
        return execute_sell(player, parsed.args);
    }
    if(verb == "contribute")
    {
        return execute_contribute(player, parsed.args);
    }
    if(verb == "join")
    {
        return execute_join(player, parsed.args);
    }
    if(verb == "team")
    {
        return execute_team(player, parsed.args);
    }
    if(verb == "event")
    {
        return execute_event();
    }
    if(verb == "chat")
    {
        return execute_chat(*player, parsed.raw_args);
    }
    if(verb == "say")
    {
        return execute_say(*player, parsed.raw_args);
    }
    if(verb == "tell")
    {
        return execute_tell(player, parsed.args, parsed.raw_args);
    }
    if(verb == "reply")
    {
        return execute_reply(player, parsed.raw_args);
    }
    if(verb == "emote")
    {
        return execute_emote(*player, parsed.raw_args);
    }
    if(verb == "follow")
    {
        return execute_follow(player, parsed.args);
    }
    if(verb == "guard")
    {
        return execute_guard(player, parsed.args);
    }
    if(verb == "trade")
    {
        return execute_trade(player, parsed.args);
    }
    if(verb == "challenge")
    {
        return execute_challenge(player, parsed.args);
    }
    if(verb == "codex")
    {
        return execute_codex(player, parsed.args);
    }
    if(verb == "save")
    {
        return execute_save(*player);
    }

    execution.title = "未知指令";
    execution.summary = "这道指令暂时无法识别，请优先使用界面里的功能按钮或中文提示操作。";
    execution.hints = {"可直接输入 look、talk、ask、go、fight、journal、bag、score 等指令继续游历。"};
    return execution;
}

MudCommandExecution MudGameRuntime::execute_look(MudPlayerState* player) const
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(scene == nullptr)
    {
        execution.title = "神识迷失";
        execution.summary = "当前场景无效。";
        return execution;
    }

    std::vector<std::string> npc_names;
    for(const auto& npc_id : scene->npc_ids)
    {
        if(const auto* npc = m_world->find_npc(npc_id); npc != nullptr)
        {
            npc_names.push_back(npc->name);
        }
    }

    std::vector<std::string> exit_names;
    for(const auto& entry : scene->exits)
    {
        exit_names.push_back(entry.first);
    }
    std::sort(exit_names.begin(), exit_names.end());

    execution.success = true;
    execution.title = scene->name;
    execution.summary = scene->description;
    execution.hints.push_back("在场人物：" + (npc_names.empty() ? std::string("无人") : join_strings(npc_names)));
    execution.hints.push_back("可走方向：" + (exit_names.empty() ? std::string("无") : join_strings(exit_names)));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_map(MudPlayerState* player) const
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(scene == nullptr)
    {
        execution.title = "地图失效";
        execution.summary = "当前场景不存在。";
        return execution;
    }

    execution.success = true;
    execution.title = "人界地图";
    execution.summary = "你当前位于「" + scene->region_name + " / " + scene->name + "」。";
    for(const auto& entry : scene->exits)
    {
        const auto* target = m_world->find_scene(entry.second);
        execution.hints.push_back(direction_display_name(entry.first) + "通往「" +
                                  (target == nullptr ? std::string("未知去处") : target->name) + "」。");
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_go(MudPlayerState* player,
                                               const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(scene == nullptr)
    {
        execution.title = "无法移动";
        execution.summary = "当前场景不存在。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "缺少方向";
        execution.summary = "请选择一个方向后再移动。";
        execution.hints = {"例如：go north", "例如：go east"};
        return execution;
    }

    const auto direction = normalize_direction(args.front());
    if(auto iter = scene->exits.find(direction); iter != scene->exits.end())
    {
        player->location_scene_id = iter->second;
        const auto* target = current_scene(*player);
        execution.success = true;
        execution.title = "御风而行";
        execution.summary = "你离开「" + scene->name + "」，来到「" +
                            (target == nullptr ? iter->second : target->name) + "」。";
        unlock_codex_by_trigger(player, "enter_scene", player->location_scene_id, &execution);
        append_event(player->account,
                     "move",
                     execution.title,
                     execution.summary,
                     &execution.events);
        return execution;
    }

    execution.title = "路途不通";
    execution.summary = "当前方位没有道路可走。";
    execution.hints.push_back("可先用 travel 或 map 梳理当前出口。");
    return execution;
}

MudCommandExecution MudGameRuntime::execute_talk(MudPlayerState* player,
                                                 const std::vector<std::string>& args) const
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "无法对话";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    const auto* npc = args.empty() ? nullptr : match_scene_npc(*player, args.front());
    if(npc == nullptr)
    {
        execution.title = "无人应答";
        execution.summary = "当前场景找不到这个人物。";
        execution.hints.push_back("可先用 look 或 here 重观当前场景人物。");
        return execution;
    }

    execution.success = true;
    execution.title = "与" + npc->name + "交谈";
    refresh_quest_progress(player);
    unlock_codex_by_trigger(player, "talk_npc", npc->npc_id, &execution);
    const bool talked_before = player->flags["talked:" + npc->npc_id] == "1";
    std::vector<std::string> hinted_quest_ids;
    auto push_hint_once = [&](const std::string& quest_id, std::string hint) {
        if(std::find(hinted_quest_ids.begin(), hinted_quest_ids.end(), quest_id) != hinted_quest_ids.end())
        {
            return;
        }
        hinted_quest_ids.push_back(quest_id);
        execution.hints.push_back(std::move(hint));
    };
    for(const auto& quest_id : npc->quest_ids)
    {
        const auto* quest = m_world->find_quest(quest_id);
        if(quest == nullptr)
        {
            continue;
        }

        const auto* quest_state = find_quest_state(*player, quest->quest_id);
        if(quest_state == nullptr)
        {
            push_hint_once(quest->quest_id, "可接任务：accept " + quest->quest_id + "（" + quest->title + "）");
            continue;
        }

        if(quest_state->status == "completed" || quest_state->status == "submitted")
        {
            continue;
        }

        if(quest_state->status == "active")
        {
            if(npc->npc_id == quest->submit_npc_id && quest_state->progress >= quest->required_item_count)
            {
                push_hint_once(quest->quest_id, "可提交任务：submit " + quest->quest_id + "（" + quest->title + "）");
                continue;
            }

            if(quest_state->progress < quest->required_item_count)
            {
                push_hint_once(quest->quest_id,
                               "任务进行中：" + quest->title + "（" +
                                   std::to_string(quest_state->progress) + " / " +
                                   std::to_string(quest->required_item_count) + "）");
                continue;
            }

            if(npc->npc_id != quest->submit_npc_id)
            {
                if(const auto* submit_npc = m_world->find_npc(quest->submit_npc_id); submit_npc != nullptr)
                {
                    push_hint_once(quest->quest_id, "任务已齐：把材料交给" + submit_npc->name + "。");
                }
            }
        }
    }
    for(const auto& quest_state : player->quests)
    {
        if(quest_state.status != "active")
        {
            continue;
        }
        if(std::find(hinted_quest_ids.begin(), hinted_quest_ids.end(), quest_state.quest_id) != hinted_quest_ids.end())
        {
            continue;
        }
        const auto* quest = m_world->find_quest(quest_state.quest_id);
        if(quest == nullptr || quest->submit_npc_id != npc->npc_id)
        {
            continue;
        }
        if(quest_state.progress >= quest->required_item_count)
        {
            push_hint_once(quest->quest_id, "可提交任务：submit " + quest->quest_id + "（" + quest->title + "）");
            continue;
        }
        push_hint_once(quest->quest_id,
                       "任务进行中：" + quest->title + "（" + std::to_string(quest_state.progress) + " / " +
                           std::to_string(quest->required_item_count) + "）");
    }
    if(!npc->sect_offer_id.empty())
    {
        if(const auto* sect = m_world->find_sect(npc->sect_offer_id); sect != nullptr)
        {
            execution.hints.push_back("若想拜入" + sect->name + "，可输入：join " + sect->sect_id);
        }
    }
    bool has_submit_hint = false;
    bool has_progress_hint = false;
    for(const auto& hint : execution.hints)
    {
        if(hint.find("可提交任务：") != std::string::npos || hint.find("任务已齐：") != std::string::npos)
        {
            has_submit_hint = true;
        }
        if(hint.find("任务进行中：") != std::string::npos)
        {
            has_progress_hint = true;
        }
    }
    if(has_submit_hint && !npc->submit_talk_text.empty())
    {
        execution.summary = npc->submit_talk_text;
    }
    else if(has_progress_hint && !npc->progress_talk_text.empty())
    {
        execution.summary = npc->progress_talk_text;
    }
    else if(!talked_before && !npc->first_talk_text.empty())
    {
        execution.summary = npc->first_talk_text;
    }
    else if(talked_before && !npc->repeat_talk_text.empty())
    {
        execution.summary = npc->repeat_talk_text;
    }
    else
    {
        execution.summary = npc->dialogue.empty() ? (npc->name + "静静看着你。") : npc->dialogue;
    }
    for(const auto& topic : npc->ask_topics)
    {
        const auto label = ask_topic_label(topic);
        if(!label.empty())
        {
            execution.hints.push_back("可问：" + label);
        }
    }
    player->flags["talked:" + npc->npc_id] = "1";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_ask(MudPlayerState* player,
                                                const std::vector<std::string>& args) const
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "追问失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "问话题";
        execution.summary = "请先写明要追问的人物，再补上话题。";
        execution.hints = {"格式：ask 厉飞雨 差事", "格式：ask 辛如音 阵法"};
        return execution;
    }

    const auto* npc = match_scene_npc(*player, args.front());
    if(npc == nullptr)
    {
        execution.title = "无人可问";
        execution.summary = "当前场景找不到这个人物。";
        execution.hints.push_back("可先用 look、here 或 inspect 重观眼前人物。");
        return execution;
    }

    const auto rumor_sources = m_world->rumor_sources_for_npc(npc->npc_id);
    const bool using_about = args.size() >= 2 && is_about_keyword(args[1]);

    if(args.size() < 2 || (using_about && args.size() < 3))
    {
        execution.success = true;
        execution.title = "向" + npc->name + "追问";
        execution.summary = npc->name + "抬眼看了你一下，像是在等你把想问的话题说得更明白。";
        if(!rumor_sources.empty())
        {
            execution.hints.push_back("可问：ask " + npc->name + " about rumor");
            for(const auto* rumor_source : rumor_sources)
            {
                if(rumor_source == nullptr || rumor_source->topic.empty())
                {
                    continue;
                }
                execution.hints.push_back("可问：ask " + npc->name + " about " + rumor_source->topic);
            }
        }
        for(const auto& topic : npc->ask_topics)
        {
            const auto label = ask_topic_label(topic);
            if(!label.empty())
            {
                execution.hints.push_back("可问：" + label);
            }
        }
        if(execution.hints.empty())
        {
            execution.hints.push_back("此人暂时没有显露更多可追问的话题。");
        }
        return execution;
    }

    const std::string topic_query = using_about
                                        ? mud_trim(join_strings(std::vector<std::string>(args.begin() + 2, args.end()), " "))
                                        : mud_trim(join_strings(std::vector<std::string>(args.begin() + 1, args.end()), " "));
    if(using_about && !rumor_sources.empty())
    {
        const MudRumorSourceConfig* matched_rumor = nullptr;
        for(const auto* rumor_source : rumor_sources)
        {
            if(rumor_source == nullptr)
            {
                continue;
            }
            if(topic_query.empty() || is_rumor_keyword(topic_query) || ask_topic_matches(rumor_source->topic, topic_query) ||
               rumor_source->summary.find(topic_query) != std::string::npos)
            {
                matched_rumor = rumor_source;
                break;
            }
        }
        if(matched_rumor == nullptr && is_rumor_keyword(topic_query))
        {
            matched_rumor = rumor_sources.front();
        }
        if(matched_rumor != nullptr)
        {
            execution.success = true;
            execution.title = "向" + npc->name + "打听风声";
            execution.summary = matched_rumor->summary.empty() ? generic_ask_response(*npc, "风声") : matched_rumor->summary;
            unlock_codex_by_trigger(player, "talk_npc", npc->npc_id, &execution);
            player->flags["talked:" + npc->npc_id] = "1";

            MudStructuredPanelState panel;
            panel.panel_id = "help";
            panel.title = matched_rumor->topic.empty() ? execution.title : matched_rumor->topic;
            panel.summary = execution.summary;
            panel.render_mode = "notice_block";
            panel.style_id = "mud-manual";
            panel.document_id = matched_rumor->source_id;
            panel.panel_kind = "rumor";
            panel.body_lines = matched_rumor->body_lines;
            panel.inline_commands.push_back("board");
            panel.inline_commands.push_back("work");

            for(const auto& job_id : matched_rumor->job_ids)
            {
                const auto* job = m_world->find_job(job_id);
                if(job == nullptr)
                {
                    continue;
                }
                std::string command = mud_trim(job->command_hint);
                if(command.empty())
                {
                    if(const auto* issuer = m_world->find_npc(job->issuer_npc_id); issuer != nullptr)
                    {
                        command = "ask " + issuer->name + " about rumor";
                    }
                    else
                    {
                        command = "work";
                    }
                }
                panel.entries.push_back({job->job_id,
                                         job->title,
                                         job->summary,
                                         job->kind,
                                         "风声",
                                         command,
                                         npc->name,
                                         job->reward_summary});
            }
            for(const auto& quest_id : matched_rumor->quest_ids)
            {
                const auto* quest = m_world->find_quest(quest_id);
                if(quest == nullptr)
                {
                    continue;
                }
                std::string status = "可接";
                std::string command = "accept " + quest->quest_id;
                if(const auto* quest_state = find_quest_state(*player, quest->quest_id); quest_state != nullptr)
                {
                    if(quest_state->status == "active")
                    {
                        status = quest_state->progress >= quest->required_item_count ? "可交" : "进行中";
                        command = quest_state->progress >= quest->required_item_count ? ("submit " + quest->quest_id)
                                                                                     : "journal";
                    }
                    else if(quest_state->status == "completed")
                    {
                        status = "已了";
                        command.clear();
                    }
                }
                panel.entries.push_back({quest->quest_id,
                                         quest->title,
                                         quest->description,
                                         status,
                                         "风声",
                                         command,
                                         npc->name,
                                         "灵石 " + std::to_string(quest->reward_spirit_stone) + " / 修为 " +
                                             std::to_string(quest->reward_exp)});
            }

            int followup_count = 0;
            for(const auto* rumor_source : rumor_sources)
            {
                if(rumor_source == nullptr || rumor_source == matched_rumor || rumor_source->topic.empty())
                {
                    continue;
                }
                execution.hints.push_back("可继续问：ask " + npc->name + " about " + rumor_source->topic);
                if(++followup_count >= 3)
                {
                    break;
                }
            }
            execution.panels.push_back(std::move(panel));
            return execution;
        }
    }

    auto matched = npc->ask_topics.end();
    for(auto iter = npc->ask_topics.begin(); iter != npc->ask_topics.end(); ++iter)
    {
        if(ask_topic_matches(*iter, topic_query))
        {
            matched = iter;
            break;
        }
    }

    if(matched == npc->ask_topics.end())
    {
        execution.title = "话题未明";
        execution.summary = npc->name + "没有正面回应这个话题。";
        for(const auto& topic : npc->ask_topics)
        {
            const auto label = ask_topic_label(topic);
            if(!label.empty())
            {
                execution.hints.push_back("可问：" + label);
            }
        }
        if(execution.hints.empty())
        {
            execution.hints.push_back("先与此人交谈，等风声露出更多口子。");
        }
        return execution;
    }

    execution.success = true;
    const auto parsed_topic = parse_ask_topic_entry(*matched);
    execution.title = "向" + npc->name + "问起「" + parsed_topic.first + "」";
    execution.summary = parsed_topic.second.empty() ? generic_ask_response(*npc, parsed_topic.first) : parsed_topic.second;
    unlock_codex_by_trigger(player, "talk_npc", npc->npc_id, &execution);
    player->flags["talked:" + npc->npc_id] = "1";

    int followup_count = 0;
    for(const auto& topic : npc->ask_topics)
    {
        const auto label = ask_topic_label(topic);
        if(label.empty() || label == parsed_topic.first)
        {
            continue;
        }
        execution.hints.push_back("可继续问：" + label);
        if(++followup_count >= 3)
        {
            break;
        }
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_accept(MudPlayerState* player,
                                                   const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "接取失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    const auto* quest = args.empty() ? nullptr : match_scene_quest(*player, args.front());
    if(quest == nullptr)
    {
        execution.title = "未找到任务";
        execution.summary = "当前场景没有这个可接任务。";
        execution.hints.push_back("先与 NPC 对话，再使用 accept <quest_id>");
        return execution;
    }

    auto* quest_state = find_quest_state(player, quest->quest_id);
    if(quest_state != nullptr && quest_state->status == "completed")
    {
        execution.title = "任务已了";
        execution.summary = "这个任务你已经完成。";
        return execution;
    }
    if(quest_state != nullptr && quest_state->status == "active")
    {
        execution.title = "任务进行中";
        execution.summary = "你已经接下这个任务。";
        return execution;
    }

    MudQuestState new_state;
    new_state.quest_id = quest->quest_id;
    new_state.status = "active";
    player->quests.push_back(new_state);
    refresh_quest_progress(player);

    execution.success = true;
    execution.title = "接取任务";
    execution.summary = "你接下了任务「" + quest->title + "」。";
    execution.hints.push_back(quest->description);
    unlock_codex_by_trigger(player, "accept_quest", quest->quest_id, &execution);
    append_event(player->account,
                 "quest",
                 execution.title,
                 execution.summary,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_submit(MudPlayerState* player,
                                                   const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "提交失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    const auto* scene = current_scene(*player);
    const auto normalized_key = args.empty() ? std::string() : mud_to_lower_ascii(args.front());
    MudQuestState* quest_state = nullptr;
    const auto* quest = static_cast<const MudQuestConfig*>(nullptr);
    for(auto& candidate_state : player->quests)
    {
        if(candidate_state.status != "active")
        {
            continue;
        }
        const auto* candidate_quest = m_world->find_quest(candidate_state.quest_id);
        if(candidate_quest == nullptr || !quest_key_matches(*candidate_quest, normalized_key))
        {
            continue;
        }
        quest = candidate_quest;
        quest_state = &candidate_state;
        break;
    }
    if(quest == nullptr || quest_state == nullptr)
    {
        execution.title = "未找到任务";
        execution.summary = "当前没有这个可提交的任务。";
        return execution;
    }
    if(!scene_has_npc(scene, quest->submit_npc_id))
    {
        execution.title = "无法提交";
        if(const auto* submit_npc = m_world->find_npc(quest->submit_npc_id); submit_npc != nullptr)
        {
            execution.summary = "此任务需要交给" + submit_npc->name + "。";
        }
        else
        {
            execution.summary = "当前场景没有这个任务可提交。";
        }
        return execution;
    }

    refresh_quest_progress(player);
    if(quest_state->progress < quest->required_item_count)
    {
        execution.title = "材料未齐";
        execution.summary = "任务还未达到提交条件。";
        execution.hints.push_back("需要：" + item_with_count_label(m_world.get(),
                                                                 quest->required_item_id,
                                                                 quest->required_item_count));
        return execution;
    }

    if(!quest->required_item_id.empty() && quest->required_item_count > 0)
    {
        remove_inventory_item(player, quest->required_item_id, quest->required_item_count);
    }

    quest_state->status = "completed";
    quest_state->progress = quest->required_item_count;
    player->spirit_stone += quest->reward_spirit_stone;
    player->exp += quest->reward_exp;
    if(player->sect_id.empty())
    {
        set_flag_int(player,
                     "loose_reputation",
                     flag_int_value(*player, "loose_reputation", 0) +
                         std::max(18, quest->reward_spirit_stone / 2 + static_cast<int>(quest->reward_exp / 12)));
    }
    if(!quest->reward_item_id.empty() && quest->reward_item_count > 0)
    {
        add_inventory_item(player, quest->reward_item_id, quest->reward_item_count, false);
        unlock_codex_by_trigger(player, "obtain_item", quest->reward_item_id, &execution);
    }
    refresh_quest_progress(player);

    execution.success = true;
    execution.title = "任务完成";
    execution.summary = "你完成了「" + quest->title + "」，获得灵石 " +
                        std::to_string(quest->reward_spirit_stone) + "、修为 " +
                        std::to_string(quest->reward_exp) + "。";
    unlock_codex_by_trigger(player, "submit_quest", quest->quest_id, &execution);
    if(!quest->reward_item_id.empty() && quest->reward_item_count > 0)
    {
        execution.hints.push_back("奖励物品：" + item_with_count_label(m_world.get(),
                                                                   quest->reward_item_id,
                                                                   quest->reward_item_count));
    }
    append_event(player->account,
                 "quest",
                 execution.title,
                 execution.summary,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_fight(MudPlayerState* player,
                                                  const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "战斗失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    const auto* monster = args.empty() ? nullptr : match_scene_monster(*player, args.front());
    if(monster == nullptr)
    {
        execution.title = "目标不存在";
        execution.summary = "当前场景找不到这个对手。";
        execution.hints.push_back("可先用 look 或 listen 察看当前场景妖兽。");
        return execution;
    }

    const std::string damage_key = monster_damage_flag_key(*player, *monster);
    const int accumulated_damage = std::max(0, flag_int_value(*player, damage_key, 0));
    const int effective_hp = std::max(1, monster->hp - accumulated_damage);
    const int player_damage = std::max(1, player->attack_power + player->skill_level * 2 - monster->defense);
    const int monster_damage = std::max(1, monster->attack - std::max(1, player->defense_power / 2));
    const bool player_wins = player_damage >= effective_hp ||
                             (player->hp + player->attack_power + player->defense_power) >=
                                 (effective_hp + monster->attack + monster->defense);

    if(player_wins)
    {
        clear_flag(player, damage_key);
        player->hp = std::max(1, player->hp - std::max(1, monster_damage / 2));
        player->exp += monster->reward_exp;
        player->spirit_stone += monster->reward_spirit_stone;
        set_flag_int(player,
                     "bounty_score",
                     flag_int_value(*player, "bounty_score", 0) + std::max(1, monster->reward_spirit_stone / 40));
        if(player->sect_id.empty())
        {
            set_flag_int(player,
                         "loose_reputation",
                         flag_int_value(*player, "loose_reputation", 0) + std::max(8, monster->reward_spirit_stone / 6));
        }
        if(!monster->drop_item_id.empty() && monster->drop_item_count > 0)
        {
            add_inventory_item(player, monster->drop_item_id, monster->drop_item_count, false);
            unlock_codex_by_trigger(player, "obtain_item", monster->drop_item_id, &execution);
            execution.hints.push_back("掉落入包：" +
                                      item_with_count_label(m_world.get(),
                                                            monster->drop_item_id,
                                                            monster->drop_item_count));
        }
        refresh_quest_progress(player);

        execution.success = true;
        execution.title = "战斗得胜";
        execution.summary = "你击败了「" + monster->name + "」，获得修为 " +
                            std::to_string(monster->reward_exp) + "、灵石 " +
                            std::to_string(monster->reward_spirit_stone) + "。";
        if(accumulated_damage > 0)
        {
            execution.hints.push_back("此前的法术削弱了这头妖兽。");
        }
        unlock_codex_by_trigger(player, "defeat_monster", monster->monster_id, &execution);
        append_event(player->account,
                     "combat",
                     execution.title,
                     execution.summary,
                     &execution.events);
        return execution;
    }

    player->hp = std::max(1, player->hp - monster_damage);
    execution.title = "战斗失利";
    execution.summary = "你被「" + monster->name + "」逼退，气血降至 " + std::to_string(player->hp) + "。";
    if(const auto* recover_item = m_world == nullptr ? nullptr : m_world->find_item("small_recover_pill");
       recover_item != nullptr)
    {
        execution.hints.push_back("可在背包中使用「" + recover_item->name + "」恢复气血。");
    }
    else
    {
        execution.hints.push_back("可在背包中使用回气药物恢复气血。");
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_use(MudPlayerState* player,
                                                const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "使用失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "缺少物品";
        execution.summary = "请选择一件背包中的物品后再使用。";
        return execution;
    }

    const int inventory_index = find_inventory_index(*player, args.front());
    if(inventory_index < 0)
    {
        execution.title = "背包空空";
        execution.summary = "你没有这件物品。";
        return execution;
    }

    auto& inventory_item = player->inventory[static_cast<size_t>(inventory_index)];
    const auto* item_config = m_world->find_item(inventory_item.item_id);
    if(item_config == nullptr)
    {
        execution.title = "使用失败";
        execution.summary = "物品配置不存在。";
        return execution;
    }

    if(item_config->equipable)
    {
        inventory_item.equipped = !inventory_item.equipped;
        if(inventory_item.equipped)
        {
            player->attack_power += item_config->attack_bonus;
            player->defense_power += item_config->defense_bonus;
            execution.success = true;
            execution.title = "装备上身";
            execution.summary = "你装备了「" + item_config->name + "」。";
        }
        else
        {
            player->attack_power = std::max(1, player->attack_power - item_config->attack_bonus);
            player->defense_power = std::max(0, player->defense_power - item_config->defense_bonus);
            execution.success = true;
            execution.title = "收起装备";
            execution.summary = "你卸下了「" + item_config->name + "」。";
        }
        append_event(player->account,
                     "inventory",
                     execution.title,
                     execution.summary,
                     &execution.events);
        return execution;
    }

    if(item_config->consumable)
    {
        player->hp = std::min(player->max_hp, player->hp + item_config->hp_restore);
        player->exp += item_config->exp_gain;
        player->skill_level += item_config->skill_level_gain;
        set_flag_int(player,
                     "foundation_qihai",
                     flag_int_value(*player, "foundation_qihai", 0) +
                         std::max(6, item_config->exp_gain > 0 ? static_cast<int>(item_config->exp_gain / 6) : 8));
        remove_inventory_item(player, inventory_item.item_id, 1);
        refresh_quest_progress(player);
        execution.success = true;
        execution.title = "服用成功";
        execution.summary = "你使用了「" + item_config->name + "」。";
        append_event(player->account,
                     "inventory",
                     execution.title,
                     execution.summary,
                     &execution.events);
        return execution;
    }

    execution.title = "无法使用";
    execution.summary = "这件物品当前没有可执行的使用效果。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_flee() const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "收敛气息";
    execution.summary = "你暂时停下脚步，稳住心神。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_practice(MudPlayerState* player,
                                                     const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "修炼失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    if(!args.empty() && args.front() != player->primary_skill)
    {
        execution.title = "功法不符";
        execution.summary = "当前仅支持修炼本命功法「" + player->primary_skill + "」。";
        return execution;
    }

    const int64_t gain = 20 + static_cast<int64_t>(player->skill_level) * 10;
    player->exp += gain;
    set_flag_int(player,
                 "foundation_qihai",
                 flag_int_value(*player, "foundation_qihai", 0) + std::max(12, static_cast<int>(gain / 2)));
    if(player->exp / 100 > player->skill_level)
    {
        ++player->skill_level;
        player->attack_power += 1;
        player->defense_power += 1;
    }

    execution.success = true;
    execution.title = "吐纳修炼";
    execution.summary = "你运转「" + player->primary_skill + "」，修为增加 " + std::to_string(gain) + "。";
    unlock_codex_by_trigger(player, "practice_skill", "long_spring_art", &execution);
    append_event(player->account,
                 "cultivation",
                 execution.title,
                 execution.summary,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_breakthrough(MudPlayerState* player)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "突破失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    if(player->exp < player->next_breakthrough_exp)
    {
        execution.title = "火候未到";
        execution.summary = "当前修为不足，尚不能突破。";
        execution.hints.push_back("继续修炼「" + player->primary_skill + "」，或挑战附近妖兽积累突破火候。");
        return execution;
    }

    if(player->realm_stage >= 6)
    {
        execution.title = "境界封盘";
        execution.summary = "当前阶段上限已锁定为筑基初期，后续更高境界暂未开放。";
        return execution;
    }

    if(player->realm_stage >= 5)
    {
        const int qihai = flag_int_value(*player, "foundation_qihai", 0);
        const int64_t identity_credit = player->sect_id.empty() ? flag_int_value(*player, "loose_reputation", 0)
                                                                : sect_contribution_for_player(*player);
        const bool has_material = inventory_count(*player, "foundation_pill") > 0;
        if(qihai < 120 || identity_credit < 260 || !has_material)
        {
            execution.title = "筑基未备";
            execution.summary = "冲击筑基前还需要同时稳固气海、备齐材料并满足身份资历。";
            execution.hints.push_back("气海稳固：" + std::to_string(qihai) + "/120");
            execution.hints.push_back(std::string("筑基材料：") + (has_material ? "已备筑基散" : "缺少筑基散"));
            execution.hints.push_back("身份资历：" + std::to_string(identity_credit) + "/260");
            return execution;
        }
        remove_inventory_item(player, "foundation_pill", 1);
    }

    ++player->realm_stage;
    player->realm_name = realm_name_for_stage(player->realm_stage);
    player->level += 1;
    player->next_breakthrough_exp += 120 + player->realm_stage * 80;
    player->max_hp += 20;
    player->hp = player->max_hp;
    player->attack_power += 8;
    player->defense_power += 4;
    player->title = "踏入" + player->realm_name;

    execution.success = true;
    execution.title = "突破成功";
    execution.summary = "你成功突破至「" + player->realm_name + "」，气血与战力大涨。";
    unlock_codex_by_trigger(player, "breakthrough_realm", std::to_string(player->realm_stage), &execution);
    append_event(player->account,
                 "breakthrough",
                 execution.title,
                 execution.summary,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_buy(MudPlayerState* player,
                                                const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(scene == nullptr)
    {
        execution.title = "购买失败";
        execution.summary = "当前场景不存在。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "缺少物品";
        execution.summary = "请选择一件坊市物品后再购买。";
        return execution;
    }

    const auto key = mud_to_lower_ascii(args.front());
    const MudItemConfig* item_config = nullptr;
    for(const auto& item_id : scene->shop_item_ids)
    {
        const auto* item = m_world->find_item(item_id);
        if(item == nullptr)
        {
            continue;
        }
        if(mud_to_lower_ascii(item->item_id) == key || mud_to_lower_ascii(item->name) == key)
        {
            item_config = item;
            break;
        }
    }
    if(item_config == nullptr)
    {
        execution.title = "坊市无货";
        execution.summary = "当前场景无法购买这件物品。";
        return execution;
    }
    if(player->spirit_stone < item_config->price)
    {
        execution.title = "灵石不足";
        execution.summary = "你手头灵石不够。";
        return execution;
    }

    player->spirit_stone -= item_config->price;
    add_inventory_item(player, item_config->item_id, 1, false);
    refresh_quest_progress(player);

    execution.success = true;
    execution.title = "交易完成";
    execution.summary = "你买下了「" + item_config->name + "」，花费灵石 " + std::to_string(item_config->price) + "。";
    unlock_codex_by_trigger(player, "obtain_item", item_config->item_id, &execution);
    append_event(player->account,
                 "trade",
                 execution.title,
                 execution.summary,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_sell(MudPlayerState* player,
                                                 const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "出售失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "缺少物品";
        execution.summary = "请选择一件背包物品后再出售。";
        return execution;
    }

    const int inventory_index = find_inventory_index(*player, args.front());
    if(inventory_index < 0)
    {
        execution.title = "未找到物品";
        execution.summary = "背包中没有这件物品。";
        return execution;
    }

    auto item_state = player->inventory[static_cast<size_t>(inventory_index)];
    const auto* item_config = m_world->find_item(item_state.item_id);
    if(item_config == nullptr || item_config->price <= 0)
    {
        execution.title = "无法出售";
        execution.summary = "这件物品当前没有回收价值。";
        return execution;
    }

    if(item_state.equipped)
    {
        player->attack_power = std::max(1, player->attack_power - item_config->attack_bonus);
        player->defense_power = std::max(0, player->defense_power - item_config->defense_bonus);
    }
    player->inventory.erase(player->inventory.begin() + inventory_index);
    player->spirit_stone += std::max(1, item_config->price / 2);
    refresh_quest_progress(player);

    execution.success = true;
    execution.title = "出售完成";
    execution.summary = "你卖出了「" + item_config->name + "」，获得灵石 " +
                        std::to_string(std::max(1, item_config->price / 2)) + "。";
    append_event(player->account,
                 "trade",
                 execution.title,
                 execution.summary,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_join(MudPlayerState* player,
                                                 const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "拜入失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(!player->sect_id.empty())
    {
        execution.title = "已有宗门";
        execution.summary = "你已是「" + player->sect_name + "」门人。";
        return execution;
    }

    const auto* scene = current_scene(*player);
    if(scene == nullptr)
    {
        execution.title = "拜入失败";
        execution.summary = "当前场景不存在。";
        return execution;
    }

    const MudSectConfig* matched_sect = nullptr;
    if(!args.empty())
    {
        const auto key = mud_to_lower_ascii(args.front());
        for(const auto& npc_id : scene->npc_ids)
        {
            const auto* npc = m_world->find_npc(npc_id);
            if(npc == nullptr || npc->sect_offer_id.empty())
            {
                continue;
            }
            const auto* sect = m_world->find_sect(npc->sect_offer_id);
            if(sect != nullptr && (mud_to_lower_ascii(sect->sect_id) == key || mud_to_lower_ascii(sect->name) == key))
            {
                matched_sect = sect;
                break;
            }
        }
    }
    else
    {
        for(const auto& npc_id : scene->npc_ids)
        {
            const auto* npc = m_world->find_npc(npc_id);
            if(npc != nullptr && !npc->sect_offer_id.empty())
            {
                matched_sect = m_world->find_sect(npc->sect_offer_id);
                if(matched_sect != nullptr)
                {
                    break;
                }
            }
        }
    }

    if(matched_sect == nullptr)
    {
        execution.title = "无人引荐";
        execution.summary = "当前场景没有可加入的宗门。";
        return execution;
    }
    if(!matched_sect->joinable)
    {
        execution.title = "暂未开放";
        execution.summary = "「" + matched_sect->name + "」当前只作为世界势力存在，暂不可直接拜入。";
        return execution;
    }

    bool has_completed_quest = false;
    for(const auto& quest : player->quests)
    {
        if(quest.status != "completed")
        {
            continue;
        }
        if(matched_sect->sect_id == "qixuan_gate")
        {
            has_completed_quest = true;
            break;
        }
        const auto* quest_config = m_world->find_quest(quest.quest_id);
        if(quest_config != nullptr && quest_config->reward_sect_id == matched_sect->sect_id)
        {
            has_completed_quest = true;
            break;
        }
    }
    if(!has_completed_quest)
    {
        execution.title = "资历不足";
        execution.summary = "先完成至少一个引导任务，再来拜入宗门。";
        return execution;
    }

    player->sect_id = matched_sect->sect_id;
    player->sect_name = matched_sect->name;
    player->sect_rank = matched_sect->rank_title;
    execution.success = true;
    execution.title = "拜入宗门";
    execution.summary = "你正式加入「" + matched_sect->name + "」，身份为「" + matched_sect->rank_title + "」。";
    unlock_codex_by_trigger(player, "join_sect", matched_sect->sect_id, &execution);
    append_event("",
                 "sect",
                 execution.title,
                 player->character_name + "加入了" + matched_sect->name + "。",
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_team(MudPlayerState* player,
                                                 const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "组队失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "组队指令";
        execution.summary = "可用队伍操作：创建队伍、加入队伍、查看队伍、离开队伍。";
        return execution;
    }

    const auto sub = mud_to_lower_ascii(args.front());
    auto team_owner_iter = m_team_by_account.find(player->account);
    const bool in_team = team_owner_iter != m_team_by_account.end();

    if(sub == "create")
    {
        if(in_team)
        {
            execution.title = "已有队伍";
            execution.summary = "你已经在队伍中了，可直接查看当前队伍信息。";
            return execution;
        }

        MudTeamState team;
        team.team_id = player->account;
        team.team_name = player->character_name + "的小队";
        team.leader_account = player->account;
        player->team_id = team.team_id;
        player->team_name = team.team_name;
        player->team_leader_account = team.leader_account;
        MudTeamMemberState leader_member;
        leader_member.account = player->account;
        leader_member.display_name = player->character_name;
        leader_member.leader = true;
        leader_member.player_state = *player;
        team.members.push_back(std::move(leader_member));
        m_teams.insert_or_assign(team.team_id, team);
        m_team_by_account.insert_or_assign(player->account, team.team_id);

        execution.success = true;
        execution.title = "组队成功";
        execution.summary = "你创建了队伍「" + team.team_name + "」。";
        append_event(player->account, "team", execution.title, execution.summary, &execution.events);
        return execution;
    }

    if(sub == "join")
    {
        if(args.size() < 2)
        {
            execution.title = "缺少队长";
            execution.summary = "请选择一位队长后再加入队伍。";
            return execution;
        }
        if(in_team)
        {
            execution.title = "已在队伍";
            execution.summary = "请先离开当前队伍，再加入其他队伍。";
            return execution;
        }

        const auto leader_account = args[1];
        auto team_iter = m_teams.find(leader_account);
        if(team_iter == m_teams.end())
        {
            execution.title = "队伍不存在";
            execution.summary = "当前没有这个队长创建的队伍。";
            return execution;
        }
        if(team_iter->second.members.size() >= 4)
        {
            execution.title = "队伍已满";
            execution.summary = "当前队伍已达到 4 人上限。";
            return execution;
        }

        player->team_id = team_iter->second.team_id;
        player->team_name = team_iter->second.team_name;
        player->team_leader_account = team_iter->second.leader_account;
        MudTeamMemberState member;
        member.account = player->account;
        member.display_name = player->character_name;
        member.leader = false;
        member.player_state = *player;
        team_iter->second.members.push_back(std::move(member));
        m_team_by_account.insert_or_assign(player->account, leader_account);
        execution.success = true;
        execution.title = "加入队伍";
        execution.summary = "你加入了「" + team_iter->second.team_name + "」。";
        for(const auto& member : team_iter->second.members)
        {
            append_event(member.account,
                         "team",
                         execution.title,
                         player->character_name + "加入了队伍。",
                         &execution.events);
        }
        return execution;
    }

    if(sub == "info")
    {
        if(!in_team)
        {
            execution.title = "暂无队伍";
            execution.summary = "你当前还没有加入任何队伍。";
            return execution;
        }

        auto team_iter = m_teams.find(team_owner_iter->second);
        if(team_iter == m_teams.end())
        {
            m_team_by_account.erase(player->account);
            execution.title = "队伍失效";
            execution.summary = "队伍已解散，请重新创建。";
            return execution;
        }

        execution.success = true;
        execution.title = team_iter->second.team_name;
        execution.summary = "当前成员：" + std::to_string(team_iter->second.members.size()) + "/4";
        for(const auto& member : team_iter->second.members)
        {
            execution.hints.push_back((member.leader ? "队长 · " : "成员 · ") + member.display_name + " (" + member.account + ")");
        }
        return execution;
    }

    if(sub == "leave")
    {
        if(!in_team)
        {
            execution.title = "暂无队伍";
            execution.summary = "你当前还没有加入任何队伍。";
            return execution;
        }

        auto team_iter = m_teams.find(team_owner_iter->second);
        if(team_iter == m_teams.end())
        {
            m_team_by_account.erase(player->account);
            execution.title = "队伍失效";
            execution.summary = "队伍已解散。";
            return execution;
        }

        auto& members = team_iter->second.members;
        members.erase(std::remove_if(members.begin(), members.end(), [&](const MudTeamMemberState& member) {
                          return member.account == player->account;
                      }),
                      members.end());
        m_team_by_account.erase(player->account);
        player->team_id.clear();
        player->team_name.clear();
        player->team_leader_account.clear();

        if(members.empty())
        {
            m_teams.erase(team_iter);
        }
        else if(team_iter->second.leader_account == player->account)
        {
            auto new_team = team_iter->second;
            new_team.leader_account = members.front().account;
            new_team.team_id = new_team.leader_account;
            for(auto& member : new_team.members)
            {
                member.leader = member.account == new_team.leader_account;
                m_team_by_account[member.account] = new_team.team_id;
                if(member.account == player->account)
                {
                    continue;
                }
                auto persisted_member = member.player_state;
                persisted_member.team_id = new_team.team_id;
                persisted_member.team_name = new_team.team_name;
                persisted_member.team_leader_account = new_team.leader_account;
                member.player_state = persisted_member;
                execution.extra_players_to_save.push_back(std::move(persisted_member));
            }
            m_teams.erase(team_iter);
            m_teams.insert_or_assign(new_team.team_id, new_team);
        }

        execution.success = true;
        execution.title = "离开队伍";
        execution.summary = "你离开了当前队伍。";
        append_event(player->account, "team", execution.title, execution.summary, &execution.events);
        return execution;
    }

    execution.title = "未知组队指令";
    execution.summary = "当前仅支持创建队伍、加入队伍、查看队伍与离开队伍。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_event() const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "近期天地异象";

    int count = 0;
    for(auto iter = m_events.rbegin(); iter != m_events.rend() && count < 4; ++iter)
    {
        if(iter->type != "world")
        {
            continue;
        }
        execution.hints.push_back(iter->title + "： " + iter->content);
        ++count;
    }

    execution.summary = count == 0 ? "近期尚无新的天地异象。" : "近期天地异象如下。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_chat(const MudPlayerState& player,
                                                 const std::string& raw_args)
{
    MudCommandExecution execution;
    auto trimmed = mud_trim(raw_args);
    const auto split_pos = trimmed.find(' ');
    if(trimmed.empty() || split_pos == std::string::npos)
    {
        execution.title = "发言失败";
        execution.summary = "请先选择频道，再输入要发送的消息。";
        return execution;
    }

    const auto channel = mud_trim(trimmed.substr(0, split_pos));
    const auto normalized_channel = mud_to_lower_ascii(channel);
    const auto message = mud_trim(trimmed.substr(split_pos + 1));
    if(channel.empty() || message.empty())
    {
        execution.title = "发言失败";
        execution.summary = "频道和消息都不能为空。";
        return execution;
    }

    execution.success = true;
    execution.title = "频道发言";
    const auto channel_name = normalized_channel == "team" ? std::string("队伍") : std::string("世界");
    execution.summary = "你向" + channel_name + "频道发送了消息。";
    if(normalized_channel == "world" || normalized_channel == "public")
    {
        append_event("",
                     "chat",
                     "[" + channel + "] " + player.character_name,
                     message,
                     &execution.events);
    }
    else if(normalized_channel == "team")
    {
        auto team_iter = m_team_by_account.find(player.account);
        if(team_iter == m_team_by_account.end())
        {
            execution.success = false;
            execution.title = "发言失败";
            execution.summary = "你当前没有队伍，无法使用队伍频道。";
            execution.events.clear();
            return execution;
        }

        auto state_iter = m_teams.find(team_iter->second);
        if(state_iter == m_teams.end())
        {
            execution.success = false;
            execution.title = "发言失败";
            execution.summary = "队伍状态已失效，请重新组队。";
            execution.events.clear();
            return execution;
        }

        execution.events.clear();
        for(const auto& member : state_iter->second.members)
        {
            append_event(member.account,
                         "chat",
                         "[team] " + player.character_name,
                         message,
                         &execution.events);
        }
    }
    else
    {
        execution.success = false;
        execution.title = "发言失败";
        execution.summary = "当前只支持世界频道与队伍频道发言，请先选择频道后再发送。";
        return execution;
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_say(const MudPlayerState& player,
                                                const std::string& raw_args)
{
    MudCommandExecution execution;
    const auto message = mud_trim(raw_args);
    if(message.empty())
    {
        execution.title = "发言失败";
        execution.summary = "请输入你想对当前房间说的话。";
        return execution;
    }

    execution.success = true;
    execution.title = "当面发言";
    execution.summary = "你向周围的人当面开口。";
    append_event(player.account,
                 "chat",
                 "[local] " + player.character_name,
                 message,
                 &execution.events);
    for(const auto& [account, presence] : m_online_presence)
    {
        if(account.empty() || account == player.account || presence.player.location_scene_id != player.location_scene_id)
        {
            continue;
        }
        append_event(account, "chat", "[local] " + player.character_name, message, &execution.events);
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_tell(MudPlayerState* player,
                                                 const std::vector<std::string>& args,
                                                 const std::string& raw_args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "私聊失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.size() < 2)
    {
        execution.title = "私聊失败";
        execution.summary = "请先给出玩家账号或角色名，再输入内容。";
        return execution;
    }

    const auto target_key = mud_to_lower_ascii(args.front());
    std::string message = mud_trim(raw_args);
    const auto split_pos = message.find(' ');
    message = split_pos == std::string::npos ? std::string() : mud_trim(message.substr(split_pos + 1));
    if(message.empty())
    {
        execution.title = "私聊失败";
        execution.summary = "私聊内容不能为空。";
        return execution;
    }

    const OnlinePresenceState* target_presence = nullptr;
    for(const auto& [account, presence] : m_online_presence)
    {
        if(mud_to_lower_ascii(account) == target_key ||
           mud_to_lower_ascii(presence.player.character_name) == target_key)
        {
            target_presence = &presence;
            break;
        }
    }
    if(target_presence == nullptr)
    {
        execution.title = "私聊失败";
        execution.summary = "当前找不到这个在线玩家。";
        return execution;
    }

    execution.success = true;
    execution.title = "传音入耳";
    execution.summary = "你向「" + target_presence->player.character_name + "」发去了一道私聊。";
    player->flags["last_tell_target"] = target_presence->player.account;
    player->flags["last_tell_from"] = target_presence->player.account;

    auto target_player = target_presence->player;
    target_player.flags["last_tell_target"] = player->account;
    target_player.flags["last_tell_from"] = player->account;
    execution.extra_players_to_save.push_back(target_player);
    remember_scene_presence(target_player);

    append_event(player->account,
                 "chat",
                 "[tell->" + target_presence->player.character_name + "] " + player->character_name,
                 message,
                 &execution.events);
    append_event(target_presence->player.account,
                 "chat",
                 "[tell] " + player->character_name,
                 message,
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_reply(MudPlayerState* player,
                                                  const std::string& raw_args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "回复失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    const auto message = mud_trim(raw_args);
    if(message.empty())
    {
        execution.title = "回复失败";
        execution.summary = "回复内容不能为空。";
        return execution;
    }

    auto target_account_iter = player->flags.find("last_tell_target");
    if(target_account_iter == player->flags.end() || target_account_iter->second.empty())
    {
        target_account_iter = player->flags.find("last_tell_from");
    }
    if(target_account_iter == player->flags.end() || target_account_iter->second.empty())
    {
        execution.title = "回复失败";
        execution.summary = "近期没人与你私聊，无法直接回复。";
        return execution;
    }

    std::vector<std::string> args{target_account_iter->second, message};
    return execute_tell(player, args, target_account_iter->second + " " + message);
}

MudCommandExecution MudGameRuntime::execute_emote(const MudPlayerState& player,
                                                  const std::string& raw_args)
{
    MudCommandExecution execution;
    const auto message = mud_trim(raw_args);
    if(message.empty())
    {
        execution.title = "动作失败";
        execution.summary = "请输入你想表现的动作。";
        return execution;
    }

    execution.success = true;
    execution.title = "表情动作";
    execution.summary = "你做出了一个动作。";
    append_event(player.account, "chat", "[emote] " + player.character_name, message, &execution.events);
    for(const auto& [account, presence] : m_online_presence)
    {
        if(account.empty() || account == player.account || presence.player.location_scene_id != player.location_scene_id)
        {
            continue;
        }
        append_event(account, "chat", "[emote] " + player.character_name, message, &execution.events);
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_follow(MudPlayerState* player,
                                                   const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "跟随失败";
        execution.summary = "请指定一名同场景玩家。";
        return execution;
    }
    const auto* target = match_scene_player_presence(*player, args.front());
    if(target == nullptr)
    {
        execution.title = "跟随失败";
        execution.summary = "当前场景找不到这名玩家。";
        return execution;
    }

    player->flags["follow_target"] = target->player.account;
    execution.success = true;
    execution.title = "跟随目标";
    execution.summary = "你将「" + target->player.character_name + "」标记为同行目标。";
    append_event(player->account, "social", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_guard(MudPlayerState* player,
                                                  const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "护卫失败";
        execution.summary = "请指定一名同场景玩家。";
        return execution;
    }
    const auto* target = match_scene_player_presence(*player, args.front());
    if(target == nullptr)
    {
        execution.title = "护卫失败";
        execution.summary = "当前场景找不到这名玩家。";
        return execution;
    }

    player->flags["guard_target"] = target->player.account;
    execution.success = true;
    execution.title = "护卫目标";
    execution.summary = "你决定照应「" + target->player.character_name + "」。";
    append_event(player->account, "social", execution.title, execution.summary, &execution.events);
    append_event(target->player.account,
                 "social",
                 "有人护持",
                 player->character_name + "表示会在此地照应你。",
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_trade(MudPlayerState* player,
                                                  const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "交易失败";
        execution.summary = "请先指定一名同场景玩家。";
        return execution;
    }
    const auto* target = match_scene_player_presence(*player, args.front());
    if(target == nullptr)
    {
        execution.title = "交易失败";
        execution.summary = "当前场景找不到这名玩家。";
        return execution;
    }

    execution.success = true;
    execution.title = "交易请求";
    execution.summary = "你向「" + target->player.character_name + "」发起了交易请求。";
    append_event(player->account, "social", execution.title, execution.summary, &execution.events);
    append_event(target->player.account,
                 "social",
                 "交易请求",
                 player->character_name + "希望与你交易，可先通过聊天确认内容。",
                 &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_challenge(MudPlayerState* player,
                                                      const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "切磋失败";
        execution.summary = "请先指定一名同场景玩家。";
        return execution;
    }

    const auto* scene = current_scene(*player);
    if(scene == nullptr || !scene->pvp_enabled)
    {
        execution.title = "切磋受限";
        execution.summary = "这里不是可冲突区域，无法发起切磋。";
        return execution;
    }

    const auto* target_presence = match_scene_player_presence(*player, args.front());
    if(target_presence == nullptr)
    {
        execution.title = "切磋失败";
        execution.summary = "当前场景找不到这名玩家。";
        return execution;
    }

    auto target_player = target_presence->player;
    normalize_player_state(&target_player);
    if(has_newbie_protection(target_player))
    {
        execution.title = "切磋受限";
        execution.summary = "对方仍在新手保护期内，暂不能被主动挑战。";
        return execution;
    }
    set_flag_int64(player, "pvp_opt_in_until_s", mud_now_ms() / 1000 + 10 * 60);
    const int attack_score = player->attack_power + player->combat_attributes.phys_hit / 2 + player->realm_stage * 4;
    const int defense_score = target_player.defense_power + target_player.combat_attributes.dodge / 2 +
                              target_player.realm_stage * 3;
    const int damage = std::max(12, attack_score - defense_score / 2);
    target_player.hp = std::max(0, target_player.hp - damage);

    execution.success = true;
    execution.title = "发起切磋";
    execution.summary = "你对「" + target_player.character_name + "」造成了 " + std::to_string(damage) + " 点伤势。";

    if(target_player.hp <= 0)
    {
        const int64_t spirit_loss = std::min<int64_t>(120, std::max<int64_t>(30, target_player.spirit_stone / 10));
        target_player.spirit_stone = std::max<int64_t>(0, target_player.spirit_stone - spirit_loss);
        target_player.hp = std::max(1, target_player.max_hp * 35 / 100);
        set_flag_int(player, "bounty_score", flag_int_value(*player, "bounty_score", 0) + 1);
        execution.summary += " 对手气海一滞，败退当场，遗失灵石 " + std::to_string(spirit_loss) + "。";
        append_event(target_player.account,
                     "combat",
                     "切磋落败",
                     player->character_name + "在" + scene->name + "击败了你，你损失灵石 " +
                         std::to_string(spirit_loss) + "。",
                     &execution.events);
    }
    else
    {
        append_event(target_player.account,
                     "combat",
                     "遭遇切磋",
                     player->character_name + "在" + scene->name + "与你交手，你受了 " +
                         std::to_string(damage) + " 点伤势。",
                     &execution.events);
    }

    execution.extra_players_to_save.push_back(target_player);
    remember_scene_presence(target_player);
    append_event(player->account, "combat", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_score(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = player.character_name.empty() ? "人物总览" : player.character_name;
    execution.summary = player.realm_name + " · " + player.background_name + " · " +
                        (player.sect_name.empty() ? std::string("散修") : player.sect_name);
    execution.hints.push_back("气血 " + std::to_string(player.hp) + "/" + std::to_string(player.max_hp) +
                              " · 灵石 " + std::to_string(player.spirit_stone));
    execution.hints.push_back("阶段： " + stage_label_for_player(player) + " · 推荐循环：" + recommended_loop_for_player(player));

    MudStructuredPanelState panel;
    panel.panel_id = "score";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.entries.push_back({"realm", "境界", stage_label_for_player(player), player.realm_name, "人物", "breakthrough", "当前境界", ""});
    panel.entries.push_back({"vitals",
                             "气机",
                             "气血 " + std::to_string(player.hp) + "/" + std::to_string(player.max_hp) + " · 法力 " +
                                 std::to_string(flag_int_value(player, "current_mana", player.status_attributes.mana)) +
                                 "/" + std::to_string(player.status_attributes.mana),
                             "稳态",
                             "人物",
                             "meditate",
                             "调息可恢复法力与神念",
                             ""});
    panel.entries.push_back({"progress",
                             "修为",
                             std::to_string(player.exp) + "/" + std::to_string(player.next_breakthrough_exp) + " · 推荐：" +
                                 recommended_loop_for_player(player),
                             stage_label_for_player(player),
                             "人物",
                             "journal",
                             "结合札记可继续推进",
                             ""});
    panel.entries.push_back({"attributes",
                             "根骨",
                             "神识 " + std::to_string(player.base_attributes.spi) + " · 经脉 " +
                                 std::to_string(player.base_attributes.gin) + " · 炼体 " +
                                 std::to_string(player.base_attributes.str) + " · 灵觉 " +
                                 std::to_string(player.base_attributes.per),
                             "基础",
                             "人物",
                             "skills",
                             "",
                             ""});
    panel.entries.push_back({"temperament",
                             "心性",
                             "悟性 " + std::to_string(player.base_attributes.int_attr) + " · 魅力 " +
                                 std::to_string(player.base_attributes.cha) + " · 机缘 " +
                                 std::to_string(player.base_attributes.luc),
                             "潜质",
                             "人物",
                             "family",
                             "",
                             ""});
    for(const auto& title : titles_for_player(player))
    {
        panel.entries.push_back({"title:" + title, "头衔", title, "在身", "人物", "", "", ""});
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_help(const MudPlayerState& player,
                                                const std::vector<std::string>& args) const
{
    const auto* topic = args.empty() ? m_world->find_help_topic("newbie") : match_help_topic(join_strings(args, " "));
    MudCommandExecution execution;
    if(topic == nullptr)
    {
        execution.title = "未找到帮助";
        execution.summary = "没有找到对应的帮助主题，可试试 help newbie、help commands、help work。";
        execution.hints = {"help newbie", "help commands", "help work", "help board"};
        return execution;
    }

    execution.success = true;
    execution.title = topic->title;
    execution.summary = topic->summary;

    MudStructuredPanelState panel;
    panel.panel_id = topic->topic_id == "commands" ? "commands" : "help";
    panel.title = topic->title;
    panel.summary = topic->summary;
    panel.render_mode = "notice_block";
    panel.style_id = "mud-manual";
    panel.document_id = topic->topic_id;
    panel.panel_kind = "help_topic";
    panel.body_lines = topic->body_lines;
    panel.inline_commands = topic->inline_commands;

    mud::PlayerSnapshot snapshot;
    fill_command_catalog(player, snapshot.mutable_command_catalog());
    for(const auto& command_id : topic->related_commands)
    {
        for(const auto& definition : snapshot.command_catalog())
        {
            bool matched = definition.command_id() == command_id || definition.command() == command_id;
            if(!matched)
            {
                for(const auto& alias : definition.aliases())
                {
                    if(alias == command_id)
                    {
                        matched = true;
                        break;
                    }
                }
            }
            if(!matched)
            {
                continue;
            }

            panel.entries.push_back({definition.command_id(),
                                     definition.label(),
                                     definition.summary(),
                                     definition.usage().empty() ? std::string("相关命令") : definition.usage(),
                                     "帮助",
                                     definition.command(),
                                     definition.target_hint(),
                                     ""});
            break;
        }
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_commands(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "常用命令";
    execution.summary = "高频命令、别名与用法都整理在这一札里。";

    mud::PlayerSnapshot snapshot;
    fill_command_catalog(player, snapshot.mutable_command_catalog());

    MudStructuredPanelState panel;
    panel.panel_id = "commands";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.render_mode = "notice_block";
    panel.style_id = "mud-manual";
    panel.document_id = "commands";
    panel.panel_kind = "command_manual";
    panel.inline_commands = {"help newbie", "help work", "help board"};
    panel.body_lines.push_back("先看 help <topic>，再照着 usage 输入，就不容易迷路。");

    int line_count = 0;
    for(const auto& definition : snapshot.command_catalog())
    {
        if(definition.command_id() == "scene_board_tip")
        {
            continue;
        }
        std::string line = definition.usage().empty() ? definition.command() : definition.usage();
        if(!definition.aliases().empty())
        {
            std::vector<std::string> aliases;
            for(const auto& alias : definition.aliases())
            {
                aliases.push_back(alias);
            }
            line += " | 别名 " + join_strings(aliases, "/");
        }
        if(!definition.summary().empty())
        {
            line += " | " + definition.summary();
        }
        panel.body_lines.push_back(std::move(line));
        ++line_count;
        if(line_count >= 16)
        {
            break;
        }
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_newbie(const MudPlayerState& player) const
{
    return execute_help(player, {"newbie"});
}

MudCommandExecution MudGameRuntime::execute_hp(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "气机状态";
    execution.summary = "你默运一遍内息，把当前气血、法力与气海火候重新理顺。";

    const int current_mana = flag_int_value(player, "current_mana", player.status_attributes.mana);
    const int current_sen = flag_int_value(player, "current_sen", player.status_attributes.sen);
    const int current_sta = flag_int_value(player, "current_sta", player.status_attributes.sta);
    const int qihai = flag_int_value(player, "foundation_qihai", 0);

    MudStructuredPanelState panel;
    panel.panel_id = "hp";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.render_mode = "dossier_block";
    panel.style_id = "mud-tablet";
    panel.panel_kind = "status_sheet";
    panel.entries.push_back({"kee",
                             "气血",
                             std::to_string(player.hp) + "/" + std::to_string(player.max_hp),
                             player.hp * 100 / std::max(1, player.max_hp) >= 60 ? "稳" : "亏损",
                             "气机",
                             "meditate",
                             "",
                             ""});
    panel.entries.push_back({"mana",
                             "法力",
                             std::to_string(current_mana) + "/" + std::to_string(player.status_attributes.mana),
                             current_mana * 100 / std::max(1, player.status_attributes.mana) >= 60 ? "可用" : "偏空",
                             "气机",
                             "meditate",
                             "",
                             ""});
    panel.entries.push_back({"sen",
                             "神念",
                             std::to_string(current_sen) + "/" + std::to_string(player.status_attributes.sen),
                             "神识",
                             "气机",
                             "",
                             "",
                             ""});
    panel.entries.push_back({"sta",
                             "气力",
                             std::to_string(current_sta) + "/" + std::to_string(player.status_attributes.sta),
                             "体魄",
                             "气机",
                             "",
                             "",
                             ""});
    panel.entries.push_back({"qihai",
                             "气海火候",
                             std::to_string(qihai),
                             stage_label_for_player(player),
                             "根基",
                             "breakthrough",
                             contribution_state_for_player(player),
                             ""});
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_rank(const MudPlayerState& player,
                                                const std::vector<std::string>& args) const
{
    MudCommandExecution execution;
    if(m_repository == nullptr)
    {
        execution.title = "查看榜单失败";
        execution.summary = "当前无法访问榜单数据。";
        return execution;
    }

    const auto leaderboard_type =
        args.empty() ? MudLeaderboardType::realm : mud_parse_leaderboard_type(join_strings(args, " "));
    MudPlayerRepositoryOpResult rank_result;
    if(!wait_mud_player_repository_result(m_repository.get(),
                                          m_repository->list_top_players(leaderboard_type, 10),
                                          &rank_result,
                                          1500) ||
       !rank_result.success)
    {
        execution.title = "查看榜单失败";
        execution.summary = rank_result.error.empty() ? "榜单暂时未能取回。" : rank_result.error;
        return execution;
    }

    const auto leaderboard_label = [&]() -> std::string {
        switch(leaderboard_type)
        {
        case MudLeaderboardType::wealth:
            return "财富榜";
        case MudLeaderboardType::combat:
            return "战力榜";
        case MudLeaderboardType::alchemy:
            return "丹道榜";
        case MudLeaderboardType::travel:
            return "游历榜";
        case MudLeaderboardType::bounty:
            return "赏金榜";
        case MudLeaderboardType::chief:
            return "首席榜";
        case MudLeaderboardType::realm:
        default:
            return "境界榜";
        }
    }();

    execution.success = true;
    execution.title = leaderboard_label;
    execution.summary = rank_result.players.empty() ? "当前榜上暂未抄录到条目。" : "当前只抄录前 10 名。";

    MudStructuredPanelState panel;
    panel.panel_id = "rank";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.style_id = "jianghu-board";
    panel.panel_kind = "leaderboard";
    panel.document_id = "rank:" + mud_leaderboard_name(leaderboard_type);
    panel.ascii_lines.push_back("名次  人物           境界/身份         分值");
    for(const auto& entry : rank_result.players)
    {
        std::ostringstream line;
        line << (entry.rank < 10 ? "0" : "") << entry.rank << ". ";
        line << compact_preview(entry.player.character_name, 12);
        const size_t display_width = std::min<size_t>(12, entry.player.character_name.size());
        const size_t padding = display_width >= 12 ? 1 : (13 - display_width);
        line << std::string(padding, ' ');
        line << compact_preview(entry.extra.empty() ? entry.player.realm_name : entry.extra, 16) << " ";
        line << entry.score;
        panel.ascii_lines.push_back(line.str());
        panel.entries.push_back({"rank:" + std::to_string(entry.rank),
                                 "#" + std::to_string(entry.rank) + " " + entry.player.character_name,
                                 entry.extra.empty() ? entry.player.realm_name : entry.extra,
                                 std::to_string(entry.score),
                                 "榜单",
                                 "",
                                 entry.player.sect_name,
                                 ""});
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_board(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    const auto* scene = current_scene(player);
    execution.success = true;
    execution.title = "公共委托板";
    execution.summary = scene != nullptr && scene->board_available ? "此地可接事务、循环与近帖如下。" : "此地可接事务与循环如下。";
    MudStructuredPanelState panel;
    panel.panel_id = "board";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.entries = board_entries_for_player(player, scene);
    if(scene != nullptr && scene->board_available)
    {
        int index = 1;
        for(const auto* post : board_posts_for_scene(player, scene->scene_id, 5))
        {
            if(post == nullptr)
            {
                continue;
            }
            const auto parsed_title = parse_board_post_title(post->title);
            const auto& subject = parsed_title.second;
            panel.entries.push_back({"board_post_" + std::to_string(post->event_id),
                                     "板帖 " + std::to_string(index) + " · " + (subject.empty() ? std::string("无题") : subject),
                                     compact_preview(post->content, 42),
                                     "留帖",
                                     "板帖",
                                     "read " + std::to_string(index),
                                     scene->name,
                                     ""});
            ++index;
        }
        execution.hints.push_back("此地可用 post <题目>=<正文> 留帖，也可用 read <编号> 读帖。");
    }
    const bool has_actionable_entries = !panel.entries.empty();
    if(!has_actionable_entries)
    {
        panel.entries.push_back({"board_empty",
                                 scene != nullptr && scene->board_available ? "板面暂空" : "此地暂无委托",
                                 scene != nullptr && scene->board_available
                                     ? "眼下还没有新的委托或板帖，可自行留字，或换处地方打听风声。"
                                     : "此地暂时没有新的公开委托，可先用 rumor 或 travel 换一处地方看看。",
                                 "空闲",
                                 "板面",
                                 scene != nullptr && scene->board_available ? "post " : "travel",
                                 scene == nullptr ? "此地" : scene->name,
                                 ""});
        execution.hints.push_back("此地暂时没有新的公开委托，可先用 rumor 或 travel 换一处地方看看。");
    }
    else
    {
        execution.hints.push_back("可优先处理标记为“可接”或“可交”的条目。");
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_read(MudPlayerState* player,
                                                const std::vector<std::string>& args) const
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(player == nullptr || scene == nullptr || !scene->board_available)
    {
        execution.title = "读板失败";
        execution.summary = "此地没有可读的留言板。";
        return execution;
    }

    const auto posts = board_posts_for_scene(*player, scene->scene_id, 12);
    if(posts.empty())
    {
        execution.title = "板上空空";
        execution.summary = "此地板面眼下还没有新帖。";
        execution.hints.push_back("可用 post <题目>=<正文> 留下第一张帖子。");
        return execution;
    }
    if(args.empty())
    {
        return execute_board(*player);
    }

    const auto selector = mud_trim(args.front());
    const MudEventEnvelope* target_post = nullptr;
    int target_index = -1;
    try
    {
        const auto index = std::stoi(selector);
        if(index >= 1 && index <= static_cast<int>(posts.size()))
        {
            target_post = posts[static_cast<size_t>(index - 1)];
            target_index = index;
        }
    }
    catch(...)
    {
    }
    if(target_post == nullptr)
    {
        for(size_t index = 0; index < posts.size(); ++index)
        {
            const auto* post = posts[index];
            if(post == nullptr)
            {
                continue;
            }
            const auto parsed_title = parse_board_post_title(post->title);
            const auto& subject = parsed_title.second;
            if(std::to_string(post->event_id) == selector || subject.find(selector) != std::string::npos)
            {
                target_post = post;
                target_index = static_cast<int>(index + 1);
                break;
            }
        }
    }
    if(target_post == nullptr)
    {
        execution.title = "未找到帖子";
        execution.summary = "当前板面上没有这条帖子。";
        execution.hints.push_back("可先用 board 看看当前板面编号。");
        return execution;
    }

    const auto parsed_title = parse_board_post_title(target_post->title);
    const auto& subject = parsed_title.second;
    execution.success = true;
    execution.title = subject.empty() ? "无题留帖" : subject;
    execution.summary = "你把这张板帖重新读了一遍。";
    const auto discard_selector = target_index > 0 ? std::to_string(target_index) : selector;
    execution.hints.push_back("可用 discard " + discard_selector + " 把它从自己的视野里收起。");

    MudStructuredPanelState panel;
    panel.panel_id = "read";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.render_mode = "notice_block";
    panel.style_id = "mud-manual";
    panel.document_id = "board_post:" + std::to_string(target_post->event_id);
    panel.panel_kind = "board_post";
    panel.body_lines = split_multiline_text(target_post->content);
    panel.inline_commands = {"board"};
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_post(MudPlayerState* player,
                                                const std::string& raw_args)
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(player == nullptr || scene == nullptr || !scene->board_available)
    {
        execution.title = "留帖失败";
        execution.summary = "此地没有可用的留言板。";
        return execution;
    }

    const auto payload = mud_trim(raw_args);
    const auto separator = payload.find('=');
    if(payload.empty() || separator == std::string::npos)
    {
        execution.title = "留帖失败";
        execution.summary = "格式应为 post <题目>=<正文>。";
        execution.hints.push_back("例如：post 收药=后湾长期收海灵藻与盐壳。");
        return execution;
    }

    const auto subject = mud_trim(payload.substr(0, separator));
    const auto content = mud_trim(payload.substr(separator + 1));
    if(subject.empty() || content.empty())
    {
        execution.title = "留帖失败";
        execution.summary = "题目和正文都不能为空。";
        return execution;
    }
    if(subject.size() > 24)
    {
        execution.title = "留帖失败";
        execution.summary = "题目太长，最多留 24 个字节。";
        execution.hints.push_back("可把题目压成一句短话，例如：收药、求购、借路。");
        return execution;
    }
    if(content.size() > 240)
    {
        execution.title = "留帖失败";
        execution.summary = "正文太长，板面每帖最多留 240 个字节。";
        execution.hints.push_back("若内容较多，可拆成两张短帖，免得把板面挤满。");
        return execution;
    }

    execution.success = true;
    execution.title = "留下板帖";
    execution.summary = "你在「" + scene->name + "」留下了一张题为「" + subject + "」的帖子。";
    append_event(player->account, "board_post", scene->scene_id + "|" + subject, content, &execution.events);
    execution.hints.push_back("可先 board 看看板面，再用 read 1 重读最新帖子。");
    return execution;
}

MudCommandExecution MudGameRuntime::execute_discard(MudPlayerState* player,
                                                   const std::vector<std::string>& args) const
{
    MudCommandExecution execution;
    const auto* scene = player == nullptr ? nullptr : current_scene(*player);
    if(player == nullptr || scene == nullptr || !scene->board_available)
    {
        execution.title = "收帖失败";
        execution.summary = "此地没有可整理的留言板。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "收帖失败";
        execution.summary = "请先指定要收起的帖子编号。";
        execution.hints.push_back("可先执行 board 查看当前板面。");
        return execution;
    }

    const auto posts = board_posts_for_scene(*player, scene->scene_id, 12);
    const auto selector = mud_trim(args.front());
    const MudEventEnvelope* target_post = nullptr;
    try
    {
        const auto index = std::stoi(selector);
        if(index >= 1 && index <= static_cast<int>(posts.size()))
        {
            target_post = posts[static_cast<size_t>(index - 1)];
        }
    }
    catch(...)
    {
    }
    if(target_post == nullptr)
    {
        for(const auto* post : posts)
        {
            if(post == nullptr)
            {
                continue;
            }
            const auto parsed_title = parse_board_post_title(post->title);
            const auto& subject = parsed_title.second;
            if(std::to_string(post->event_id) == selector || subject.find(selector) != std::string::npos)
            {
                target_post = post;
                break;
            }
        }
    }
    if(target_post == nullptr)
    {
        execution.title = "收帖失败";
        execution.summary = "当前板面上没有这条帖子。";
        return execution;
    }

    player->flags["board:hidden:" + std::to_string(target_post->event_id)] = "1";
    execution.success = true;
    execution.title = "收起帖子";
    execution.summary = "这张板帖已从你的卷页里收起。";
    execution.hints.push_back("若要再看，可等待新帖刷新或重新回到此地查看板面。");
    return execution;
}

MudCommandExecution MudGameRuntime::execute_work(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    const auto* scene = current_scene(player);
    execution.success = true;
    execution.title = "当前营生";
    execution.summary = "此地眼下能稳步开做的营生如下。";

    MudStructuredPanelState panel;
    panel.panel_id = "work";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.panel_kind = "job_board";
    panel.document_id = scene == nullptr ? "work" : ("work:" + scene->scene_id);
    panel.entries = work_entries_for_player(player, scene);
    execution.panels.push_back(std::move(panel));
    execution.hints.push_back("若想接更多活，可先 talk 人物，再 ask <人物> about rumor。");
    return execution;
}

MudCommandExecution MudGameRuntime::execute_duty(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    const auto* scene = current_scene(player);
    execution.success = true;
    execution.title = player.sect_id.empty() ? "散修事务" : (player.sect_name + "事务");
    execution.summary = player.sect_id.empty() ? "散修当前可走的身份事务如下。"
                                               : "门内当前最稳的身份事务如下。";
    MudStructuredPanelState panel;
    panel.panel_id = "duty";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.entries = duty_entries_for_player(player, scene);
    if(!panel.entries.empty())
    {
        execution.panels.push_back(std::move(panel));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_wanted(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    const auto* scene = current_scene(player);
    execution.success = true;
    execution.title = "悬赏目标";
    execution.summary = "此地可战目标、掉落与风险如下。";
    MudStructuredPanelState panel;
    panel.panel_id = "wanted";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.entries = wanted_entries_for_player(player, scene);
    if(!panel.entries.empty())
    {
        execution.panels.push_back(std::move(panel));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_travel(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    const auto* scene = current_scene(player);
    execution.success = true;
    execution.title = "路引与航路";
    execution.summary = "当前出口与已开路引如下。";
    MudStructuredPanelState panel;
    panel.panel_id = "travel";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.entries = travel_entries_for_player(player, scene);
    if(!panel.entries.empty())
    {
        execution.panels.push_back(std::move(panel));
    }
    execution.hints.push_back("travel 只负责梳理路引和路线，不会直接把你传送过去。");
    return execution;
}

MudCommandExecution MudGameRuntime::execute_claim(MudPlayerState* player,
                                                  const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "领取失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    const auto claimable = claim_entries_for_player(*player);
    if(args.empty())
    {
        execution.success = true;
        execution.title = "可领奖励";
        execution.summary = claimable.empty() ? "当前没有可领取的奖励。" : "当前可领取补给与阶段支持如下。";
        MudStructuredPanelState panel;
        panel.panel_id = "claim";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.entries = claimable;
        if(!panel.entries.empty())
        {
            execution.panels.push_back(std::move(panel));
        }
        return execution;
    }

    const auto claim_id = mud_to_lower_ascii(args.front());
    const auto iter = std::find_if(claimable.begin(), claimable.end(), [&](const MudSummaryEntry& entry) {
        return mud_to_lower_ascii(entry.entry_id) == claim_id;
    });
    if(iter == claimable.end())
    {
        execution.title = "领取失败";
        execution.summary = "当前没有这个可领取奖励。";
        execution.hints.push_back("可先执行 claim 查看当前奖励列表。");
        return execution;
    }

    if(claim_id == "starter_supplies")
    {
        add_inventory_item(player, "small_recover_pill", 2, false);
        set_flag_int(player, "claim:starter_supplies", 1);
    }
    else if(claim_id == "steady_income")
    {
        add_inventory_item(player, "wanderer_powder", 2, false);
        set_flag_int(player, "claim:steady_income", 1);
    }
    else if(claim_id == "identity_credit")
    {
        player->spirit_stone += 120;
        set_flag_int(player, "claim:identity_credit", 1);
    }
    else if(claim_id == "foundation_support")
    {
        add_inventory_item(player, "foundation_pill", 1, false);
        set_flag_int(player, "claim:foundation_support", 1);
    }

    refresh_quest_progress(player);
    execution.success = true;
    execution.title = "领取成功";
    execution.summary = "你领取了「" + iter->title + "」。";
    append_event(player->account, "reward", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_contribute(MudPlayerState* player,
                                                       const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "上交失败";
        execution.summary = "请先指定背包中的材料或物件。";
        return execution;
    }

    const int inventory_index = find_inventory_index(*player, args.front());
    if(inventory_index < 0)
    {
        execution.title = "上交失败";
        execution.summary = "背包中没有这件可上交物品。";
        return execution;
    }

    const auto item_state = player->inventory[static_cast<size_t>(inventory_index)];
    const auto* item = m_world->find_item(item_state.item_id);
    if(item == nullptr)
    {
        execution.title = "上交失败";
        execution.summary = "未识别这件物品，暂不能上交。";
        return execution;
    }

    remove_inventory_item(player, item_state.item_id, 1);
    const int gain = std::max(20, item->price / 2 + static_cast<int>(item->tags.size()) * 6);
    if(player->sect_id.empty())
    {
        set_flag_int(player, "loose_reputation", flag_int_value(*player, "loose_reputation", 0) + gain);
        execution.summary = "你上交了「" + item->name + "」，游历声望提升 " + std::to_string(gain) + "。";
    }
    else
    {
        set_flag_int(player, "manual_contribution", flag_int_value(*player, "manual_contribution", 0) + gain);
        execution.summary = "你向「" + player->sect_name + "」上交了「" + item->name + "」，贡献提升 " + std::to_string(gain) + "。";
        if(const auto* sect = m_world->find_sect(player->sect_id); sect != nullptr && !sect->rank_titles.empty())
        {
            const int64_t contribution = sect_contribution_for_player(*player);
            if(contribution >= 900 && sect->rank_titles.size() >= 4)
            {
                player->sect_rank = sect->rank_titles[3];
            }
            else if(contribution >= 420 && sect->rank_titles.size() >= 3)
            {
                player->sect_rank = sect->rank_titles[2];
            }
            else if(contribution >= 180 && sect->rank_titles.size() >= 2)
            {
                player->sect_rank = sect->rank_titles[1];
            }
            else
            {
                player->sect_rank = sect->rank_titles.front();
            }
        }
    }
    execution.success = true;
    execution.title = "上交完成";
    execution.hints.push_back("可再用 duty 查看最新身份进度。");
    append_event(player->account, "group", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_tasks(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "当前任务";
    if(player.quests.empty())
    {
        execution.summary = "你当前没有已接任务。";
        execution.hints.push_back("可先用 talk 与当前场景人物交谈，或用 rumor 打听周围线索。");
        return execution;
    }

    execution.summary = "你当前共有 " + std::to_string(player.quests.size()) + " 条任务记录。";
    MudStructuredPanelState panel;
    panel.panel_id = "tasks";
    panel.title = "游历札记";
    panel.summary = execution.summary;
    for(const auto& quest_state : player.quests)
    {
        const auto* quest = m_world->find_quest(quest_state.quest_id);
        const auto title = quest == nullptr ? quest_state.quest_id : quest->title;
        const auto target = quest == nullptr ? 0 : quest->required_item_count;
        execution.hints.push_back(title + " · " + quest_state.status + " · " + std::to_string(quest_state.progress) +
                                  "/" + std::to_string(target));
        panel.entries.push_back({quest_state.quest_id,
                                 title,
                                 quest == nullptr ? std::string("这段经历尚无详细注脚。") : quest->description,
                                 quest_status_display_name(quest_state.status),
                                 "札记",
                                 quest_state.status == "active" && quest_state.progress >= target && target > 0
                                     ? ("submit " + quest_state.quest_id)
                                     : "",
                                 quest == nullptr ? "" : quest->chapter,
                                 quest_state.status == "active" && quest_state.progress >= target && target > 0
                                     ? "可交付"
                                     : ""});
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_skills(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "技能总览";
    execution.summary = "已掌握 " + std::to_string(player.skills.size()) + " 项技能。";
    for(const auto& skill_state : player.skills)
    {
        const auto* skill = m_world->find_skill(skill_state.skill_id);
        execution.hints.push_back((skill == nullptr ? skill_state.skill_id : skill->name) + " · Lv." +
                                  std::to_string(skill_state.level) + " · 熟练 " +
                                  std::to_string(skill_state.proficiency));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_spells(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "法术总览";
    execution.summary = "已掌握 " + std::to_string(player.spells.size()) + " 项法术。";
    for(const auto& spell_state : player.spells)
    {
        const auto* spell = m_world->find_spell(spell_state.spell_id);
        execution.hints.push_back((spell == nullptr ? spell_state.spell_id : spell->name) + " · Lv." +
                                  std::to_string(spell_state.level) + " · 熟练 " +
                                  std::to_string(spell_state.proficiency));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_family(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "身份门派";
    execution.summary = player.sect_name.empty() ? "你当前仍是散修。" : ("你隶属「" + player.sect_name + "」。");
    execution.hints.push_back("出身： " + player.origin_name + " · " + player.background_name);
    if(!player.sect_name.empty())
    {
        execution.hints.push_back("阶位： " + player.sect_rank + " · 贡献 " +
                                  std::to_string(sect_contribution_for_player(player)));
    }
    else
    {
        execution.hints.push_back("散修声望： " + std::to_string(flag_int_value(player, "loose_reputation", 0)) +
                                  " · 当前阶段 " + stage_label_for_player(player));
        if(const auto* scene = current_scene(player); scene != nullptr)
        {
            for(const auto& npc_id : scene->npc_ids)
            {
                const auto* npc = m_world->find_npc(npc_id);
                if(npc == nullptr || npc->sect_offer_id.empty())
                {
                    continue;
                }
                if(const auto* sect = m_world->find_sect(npc->sect_offer_id); sect != nullptr)
                {
                    execution.hints.push_back("可引荐： " + sect->name + "（找 " + npc->name + "）");
                }
            }
        }
    }
    MudStructuredPanelState panel;
    panel.panel_id = "family";
    panel.title = execution.title;
    panel.summary = execution.summary;
    panel.entries.push_back({"origin",
                             "出身来历",
                             player.origin_name + " · " + player.background_name,
                             player.sect_name.empty() ? "散修" : player.sect_name,
                             "身份",
                             "",
                             "",
                             ""});
    if(!player.sect_name.empty())
    {
        panel.entries.push_back({"sect",
                                 "门内阶位",
                                 player.sect_rank + " · 贡献 " + std::to_string(sect_contribution_for_player(player)),
                                 "在籍",
                                 "身份",
                                 "duty",
                                 player.sect_name,
                                 ""});
    }
    else
    {
        panel.entries.push_back({"loose",
                                 "散修行脚",
                                 "游历声望 " + std::to_string(flag_int_value(player, "loose_reputation", 0)) + " · " +
                                     stage_label_for_player(player),
                                 "未入门",
                                 "身份",
                                 "board",
                                 "散修路线",
                                 ""});
    }
    for(const auto& route : route_summaries_for_player(player))
    {
        execution.hints.push_back(route.title + " · " + route.status + " · " + route.next_step);
        panel.entries.push_back({route.route_id,
                                 route.title,
                                 route.summary,
                                 route.status,
                                 "路线",
                                 "",
                                 route.next_step,
                                 ""});
    }
    execution.panels.push_back(std::move(panel));
    return execution;
}

MudCommandExecution MudGameRuntime::execute_who(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "在线人物";
    execution.summary = "当前在线约 " + std::to_string(m_online_presence.size()) + " 人。";
    MudStructuredPanelState panel;
    panel.panel_id = "who";
    panel.title = execution.title;
    panel.summary = execution.summary;
    for(const auto& [account, presence] : m_online_presence)
    {
        if(account.empty())
        {
            continue;
        }
        const auto scene_suffix = presence.player.location_scene_id == player.location_scene_id ? " · 同场景" : "";
        execution.hints.push_back(presence.player.character_name + " (" + account + ") · " +
                                  presence.player.realm_name + scene_suffix);
        panel.entries.push_back({account,
                                 presence.player.character_name,
                                 presence.player.realm_name + scene_suffix,
                                 presence.player.sect_name.empty() ? "散修" : presence.player.sect_name,
                                 "同道",
                                 "",
                                 presence.player.location_scene_id == player.location_scene_id ? "就在眼前" : "在线",
                                 ""});
    }
    if(!panel.entries.empty())
    {
        execution.panels.push_back(std::move(panel));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_rumor(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "坊间传闻";
    const auto* scene = current_scene(player);
    if(scene != nullptr)
    {
        for(const auto& rumor : scene->rumors)
        {
            execution.hints.push_back("本地 · " + rumor);
        }
    }
    int world_count = 0;
    for(auto iter = m_events.rbegin(); iter != m_events.rend() && world_count < 3; ++iter)
    {
        if(iter->type != "world")
        {
            continue;
        }
        execution.hints.push_back("天地 · " + iter->title + "： " + iter->content);
        ++world_count;
    }
    execution.summary = execution.hints.empty() ? "此地暂时没有新的风声。" : "你整理了周围和天地间的风声。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_listen(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "凝神细听";
    const auto* scene = current_scene(player);
    execution.summary = scene == nullptr ? "你只听见风声从识海边缘掠过。"
                                         : ("你在「" + scene->name + "」静了静心，听周围动静。");

    MudStructuredPanelState panel;
    panel.panel_id = "listen";
    panel.title = execution.title;
    panel.summary = execution.summary;
    if(scene != nullptr)
    {
        for(const auto& rumor : scene->rumors)
        {
            panel.entries.push_back({"rumor:" + rumor, "本地风声", rumor, "耳闻", "风声", "", scene->name, ""});
            execution.hints.push_back("本地 · " + rumor);
        }
        for(const auto& npc_id : scene->npc_ids)
        {
            if(const auto* npc = m_world->find_npc(npc_id); npc != nullptr && !npc->dialogue.empty())
            {
                panel.entries.push_back({"npc:" + npc->npc_id, npc->name, npc->dialogue, "人物", "风声", "talk " + npc->name, "", ""});
            }
        }
        for(const auto& monster_id : scene->monster_ids)
        {
            if(const auto* monster = m_world->find_monster(monster_id); monster != nullptr)
            {
                panel.entries.push_back({"monster:" + monster->monster_id,
                                         "妖兽动静",
                                         monster->name + "的气息仍在附近徘徊。",
                                         "警惕",
                                         "风声",
                                         "inspect " + monster->name,
                                         scene->name,
                                         ""});
            }
        }
    }
    if(panel.entries.empty())
    {
        execution.hints.push_back("此地一时静悄悄的，没有太多额外风声。");
    }
    else
    {
        execution.panels.push_back(std::move(panel));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_journal(const MudPlayerState& player) const
{
    auto execution = execute_tasks(player);
    execution.title = "游历札记";
    execution.summary = player.quests.empty() ? "你眼下还没有写下新的游历札记。"
                                              : "你把当前线索、待办和去向整理成了札记。";
    for(auto& panel : execution.panels)
    {
        panel.panel_id = "journal";
        panel.title = execution.title;
        panel.summary = execution.summary;
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_bag(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "行囊清单";
    execution.summary = player.inventory.empty() ? "囊中空空，只余一身风尘。"
                                                 : "你翻了翻随身行囊，把常用物件重新理了一遍。";
    MudStructuredPanelState panel;
    panel.panel_id = "bag";
    panel.title = execution.title;
    panel.summary = execution.summary;
    for(const auto& item_state : player.inventory)
    {
        const auto* item = m_world->find_item(item_state.item_id);
        panel.entries.push_back({item_state.item_id,
                                 item == nullptr ? item_state.item_id : item->name,
                                 item == nullptr ? std::string("一件随身物。") : item->description,
                                 item_state.equipped ? "已备" : ("x" + std::to_string(item_state.quantity)),
                                 "行囊",
                                 (item != nullptr && item->consumable) ? ("use " + item_state.item_id) : ("inspect " + item_state.item_id),
                                 item == nullptr ? "" : item_type_display_name(item->item_type),
                                 ""});
    }
    if(!panel.entries.empty())
    {
        execution.panels.push_back(std::move(panel));
    }
    return execution;
}

MudCommandExecution MudGameRuntime::execute_inspect(MudPlayerState* player,
                                                    const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "查看失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.empty())
    {
        execution.title = "查看目标";
        execution.summary = "请选择一个人物、妖兽、资源点或物件后再查看。";
        return execution;
    }

    const auto key = args.front();
    if(const auto* other_player = match_scene_player_presence(*player, key); other_player != nullptr)
    {
        execution.success = true;
        execution.title = other_player->player.character_name;
        execution.summary = other_player->player.realm_name + " · " +
                            (other_player->player.sect_name.empty() ? std::string("散修")
                                                                    : other_player->player.sect_name);
        MudStructuredPanelState panel;
        panel.panel_id = "inspect";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.panel_kind = "dossier";
        panel.render_mode = "dossier_block";
        panel.entries.push_back({"player",
                                 "同场修士",
                                 other_player->player.realm_name,
                                 other_player->player.sect_name.empty() ? "散修" : other_player->player.sect_name,
                                 "人物",
                                 "say",
                                 other_player->player.location_scene_id == player->location_scene_id ? "就在眼前" : "可联络",
                                 ""});
        if(!other_player->player.title.empty())
        {
            execution.hints.push_back("头衔： " + other_player->player.title);
            panel.entries.push_back({"title", "头衔", other_player->player.title, "在身", "人物", "", "", ""});
        }
        execution.hints.push_back("若要当面交流，可用 say / tell / team / challenge 等指令。");
        execution.panels.push_back(std::move(panel));
        return execution;
    }
    if(const auto* npc = match_scene_npc(*player, key); npc != nullptr)
    {
        execution.success = true;
        execution.title = npc->name;
        execution.summary = !npc->look_text.empty() ? npc->look_text :
                            (npc->description.empty() ? npc->hint : npc->description);
        MudStructuredPanelState panel;
        panel.panel_id = "inspect";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.panel_kind = "dossier";
        panel.render_mode = "dossier_block";
        panel.entries.push_back({"npc",
                                 "眼前人物",
                                 execution.summary,
                                 npc->role.empty() ? "人物" : npc->role,
                                 "人物",
                                 "talk " + npc->name,
                                 npc->sect_offer_id.empty() ? "" : npc->sect_offer_id,
                                 ""});
        if(!npc->dialogue.empty())
        {
            execution.hints.push_back(npc->dialogue);
            panel.entries.push_back({"dialogue", "当前话头", npc->dialogue, "可谈", "人物", "talk " + npc->name, "", ""});
        }
        for(const auto& topic : npc->ask_topics)
        {
            const auto label = ask_topic_label(topic);
            if(!label.empty())
            {
                execution.hints.push_back("可问：" + label);
                panel.entries.push_back({"topic:" + label, "可问话题", label, "追问", "人物", "ask " + npc->name + " " + label, "", ""});
            }
        }
        if(!npc->codex_entry_id.empty())
        {
            unlock_codex_entry(player, npc->codex_entry_id, &execution);
        }
        execution.panels.push_back(std::move(panel));
        return execution;
    }
    if(const auto* monster = match_scene_monster(*player, key); monster != nullptr)
    {
        execution.success = true;
        execution.title = monster->name;
        execution.summary = monster->description;
        MudStructuredPanelState panel;
        panel.panel_id = "inspect";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.panel_kind = "dossier";
        panel.render_mode = "dossier_block";
        panel.entries.push_back({"monster",
                                 "敌对目标",
                                 monster->description,
                                 monster->kind.empty() ? "敌对" : monster->kind,
                                 "妖兽",
                                 "fight " + monster->name,
                                 "",
                                 ""});
        execution.hints.push_back("掉落：" + item_with_count_label(m_world.get(),
                                                                 monster->drop_item_id,
                                                                 monster->drop_item_count));
        panel.entries.push_back({"drop",
                                 "掉落",
                                 item_with_count_label(m_world.get(), monster->drop_item_id, monster->drop_item_count),
                                 "战利",
                                 "妖兽",
                                 "fight " + monster->name,
                                 "",
                                 ""});
        if(!monster->codex_entry_id.empty())
        {
            unlock_codex_entry(player, monster->codex_entry_id, &execution);
        }
        execution.panels.push_back(std::move(panel));
        return execution;
    }
    if(const auto* node = match_scene_resource_node(*player, key); node != nullptr)
    {
        execution.success = true;
        execution.title = node->name;
        execution.summary = node->description;
        MudStructuredPanelState panel;
        panel.panel_id = "inspect";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.panel_kind = "dossier";
        panel.render_mode = "dossier_block";
        panel.entries.push_back({"node",
                                 "可采资源",
                                 node->description,
                                 "资源点",
                                 "采集",
                                 "harvest " + node->name,
                                 "",
                                 ""});
        execution.hints.push_back("可采集：" + item_with_count_label(m_world.get(),
                                                                   node->drop_item_id,
                                                                   node->drop_item_count));
        panel.entries.push_back({"yield",
                                 "产出",
                                 item_with_count_label(m_world.get(), node->drop_item_id, node->drop_item_count),
                                 "可采",
                                 "采集",
                                 "harvest " + node->name,
                                 "",
                                 ""});
        execution.panels.push_back(std::move(panel));
        return execution;
    }
    if(const auto* loot = match_scene_ground_loot(*player, key); loot != nullptr)
    {
        const auto* item = m_world->find_item(loot->item_id);
        execution.success = true;
        execution.title = item == nullptr ? std::string("遗落物") : item->name;
        execution.summary = loot->description;
        MudStructuredPanelState panel;
        panel.panel_id = "inspect";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.panel_kind = "dossier";
        panel.render_mode = "dossier_block";
        panel.entries.push_back({"loot",
                                 "地面遗落",
                                 loot->description,
                                 "可拾",
                                 "拾取",
                                 "loot " + execution.title,
                                 "",
                                 ""});
        execution.hints.push_back("可拾取数量：" + std::to_string(loot->quantity));
        panel.entries.push_back({"quantity",
                                 "数量",
                                 std::to_string(loot->quantity),
                                 "在地",
                                 "拾取",
                                 "loot " + execution.title,
                                 "",
                                 ""});
        if(item != nullptr && !item->codex_entry_id.empty())
        {
            unlock_codex_entry(player, item->codex_entry_id, &execution);
        }
        execution.panels.push_back(std::move(panel));
        return execution;
    }

    const int inventory_index = find_inventory_index(*player, key);
    if(inventory_index >= 0)
    {
        const auto& item_state = player->inventory[static_cast<size_t>(inventory_index)];
        const auto* item = m_world->find_item(item_state.item_id);
        execution.success = true;
        execution.title = item == nullptr ? std::string("随身物件") : item->name;
        execution.summary = item == nullptr ? "背包中的一件物品。" : item->description;
        MudStructuredPanelState panel;
        panel.panel_id = "inspect";
        panel.title = execution.title;
        panel.summary = execution.summary;
        panel.panel_kind = "dossier";
        panel.render_mode = "dossier_block";
        panel.entries.push_back({"bag",
                                 "行囊物件",
                                 execution.summary,
                                 item_state.equipped ? "已备" : "随身",
                                 "物件",
                                 item != nullptr && item->consumable ? ("use " + item_state.item_id) : "",
                                 "",
                                 ""});
        execution.hints.push_back("数量：" + std::to_string(item_state.quantity));
        panel.entries.push_back({"quantity",
                                 "数量",
                                 std::to_string(item_state.quantity),
                                 "在囊",
                                 "物件",
                                 "",
                                 "",
                                 ""});
        if(item != nullptr && !item->codex_entry_id.empty())
        {
            unlock_codex_entry(player, item->codex_entry_id, &execution);
        }
        execution.panels.push_back(std::move(panel));
        return execution;
    }

    execution.title = "目标未明";
    execution.summary = "当前视野中找不到这个目标。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_loot(MudPlayerState* player,
                                                 const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "拾取失败";
        execution.summary = "请选择一件地面物件后再拾取。";
        return execution;
    }

    const auto* loot = match_scene_ground_loot(*player, args.front());
    if(loot == nullptr)
    {
        execution.title = "无可拾取";
        execution.summary = "当前场景没有这件地面物件。";
        return execution;
    }

    const auto one_time_key = "loot:" + loot->loot_id;
    if(loot->one_time && player->flags[one_time_key] == "1")
    {
        execution.title = "已拾取";
        execution.summary = "这件物品你已经收起了。";
        return execution;
    }

    add_inventory_item(player, loot->item_id, loot->quantity, false);
    set_flag_int(player, one_time_key, 1);
    refresh_quest_progress(player);
    unlock_codex_by_trigger(player, "obtain_item", loot->item_id, &execution);
    execution.success = true;
    execution.title = "拾取成功";
    execution.summary = "你拾起了「" + item_display_name(m_world.get(), loot->item_id, "遗落物") + "」。";
    append_event(player->account, "loot", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_harvest(MudPlayerState* player,
                                                    const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "采集失败";
        execution.summary = "请选择一个资源点后再采集。";
        return execution;
    }

    const auto* node = match_scene_resource_node(*player, args.front());
    if(node == nullptr)
    {
        execution.title = "无可采集";
        execution.summary = "当前场景没有这个资源点。";
        return execution;
    }

    const auto now = mud_now_ms();
    const auto cooldown_key = "harvest:" + node->node_id;
    const auto last_ms = flag_int64_value(*player, cooldown_key, 0);
    if(last_ms > 0 && now - last_ms < node->cooldown_ms)
    {
        execution.title = "尚未恢复";
        execution.summary = "这个资源点还没有恢复灵性。";
        return execution;
    }

    add_inventory_item(player, node->drop_item_id, node->drop_item_count, false);
    set_flag_int64(player, cooldown_key, now);
    player->profession.exploration_level = std::max(1, player->profession.exploration_level) + 1;
    if(player->sect_id.empty())
    {
        set_flag_int(player, "loose_reputation", flag_int_value(*player, "loose_reputation", 0) + 10);
    }
    refresh_quest_progress(player);
    unlock_codex_by_trigger(player, "obtain_item", node->drop_item_id, &execution);
    execution.success = true;
    execution.title = "采集完成";
    execution.summary = "你从「" + node->name + "」中收获了" +
                        item_with_count_label(m_world.get(), node->drop_item_id, node->drop_item_count) + "。";
    append_event(player->account, "harvest", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_cast(MudPlayerState* player,
                                                 const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.size() < 2)
    {
        execution.title = "施法失败";
        execution.summary = "请先选定法术和目标，再进行施法。";
        return execution;
    }

    MudSpellState* spell_state = nullptr;
    const std::string spell_key = mud_to_lower_ascii(args[0]);
    for(auto& spell : player->spells)
    {
        const auto* config = m_world->find_spell(spell.spell_id);
        if(config != nullptr &&
           (mud_to_lower_ascii(config->name) == spell_key || mud_to_lower_ascii(config->spell_id) == spell_key))
        {
            spell_state = &spell;
            break;
        }
    }
    if(spell_state == nullptr)
    {
        execution.title = "法术未习";
        execution.summary = "你尚未掌握这门法术。";
        return execution;
    }

    const auto* spell_config = m_world->find_spell(spell_state->spell_id);
    const auto* monster = match_scene_monster(*player, args[1]);
    if(spell_config == nullptr || monster == nullptr)
    {
        execution.title = "施法失败";
        execution.summary = "当前无法对这个目标施法。";
        return execution;
    }

    const int current_mana = flag_int_value(*player, "current_mana", player->status_attributes.mana);
    if(current_mana < spell_config->mana_cost)
    {
        execution.title = "法力不足";
        execution.summary = "当前法力不足以施展此术。";
        return execution;
    }

    const int damage = std::max(1,
                                spell_config->power + spell_state->level * 4 + player->base_attributes.int_attr +
                                    player->combat_attributes.spell_damage - monster->defense / 2);
    const std::string damage_key = monster_damage_flag_key(*player, *monster);
    const int accumulated_damage = std::max(0, flag_int_value(*player, damage_key, 0));
    const int total_damage = std::min(monster->hp, accumulated_damage + damage);
    const int remaining_hp = std::max(0, monster->hp - total_damage);
    set_flag_int(player, "current_mana", std::max(0, current_mana - spell_config->mana_cost));
    spell_state->proficiency += damage;
    execution.success = true;
    execution.title = "法术命中";
    execution.summary = "你施展「" + spell_config->name + "」，命中「" + monster->name + "」。";
    execution.spell_summary = spell_config->name + " 对 " + monster->name + " 造成 " + std::to_string(damage) + " 点法术伤害";
    execution.hints.push_back("法力消耗：" + std::to_string(spell_config->mana_cost));
    if(remaining_hp == 0)
    {
        clear_flag(player, damage_key);
        player->exp += monster->reward_exp;
        player->spirit_stone += monster->reward_spirit_stone;
        if(!monster->drop_item_id.empty())
        {
            add_inventory_item(player, monster->drop_item_id, monster->drop_item_count, false);
            unlock_codex_by_trigger(player, "obtain_item", monster->drop_item_id, &execution);
            execution.hints.push_back("掉落入包：" +
                                      item_with_count_label(m_world.get(),
                                                            monster->drop_item_id,
                                                            monster->drop_item_count));
        }
        refresh_quest_progress(player);
        execution.title = "法术得胜";
        execution.summary = "你施展「" + spell_config->name + "」，击溃了「" + monster->name + "」。";
        execution.hints.push_back("获得修为 " + std::to_string(monster->reward_exp) + "、灵石 " +
                                  std::to_string(monster->reward_spirit_stone));
        unlock_codex_by_trigger(player, "defeat_monster", monster->monster_id, &execution);
        execution.hints.push_back("妖兽在法术下溃散。");
    }
    else
    {
        set_flag_int(player, damage_key, total_damage);
        execution.hints.push_back("「" + monster->name + "」尚余约 " + std::to_string(remaining_hp) + " 点气血。");
    }
    unlock_codex_by_trigger(player, "cast_spell", spell_state->spell_id, &execution);
    append_event(player->account, "spell", execution.title, execution.spell_summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_meditate(MudPlayerState* player)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "调息失败";
        execution.summary = "玩家状态为空。";
        return execution;
    }

    set_flag_int(player, "current_mana", player->status_attributes.mana);
    set_flag_int(player, "current_sen", player->status_attributes.sen);
    set_flag_int(player, "current_sta", player->status_attributes.sta);
    player->exp += 12;
    set_flag_int(player, "foundation_qihai", flag_int_value(*player, "foundation_qihai", 0) + 12);
    execution.success = true;
    execution.title = "静坐调息";
    execution.summary = "你缓缓调匀呼吸，法力、神念与气力都恢复了不少。";
    append_event(player->account, "cultivation", execution.title, execution.summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_brew(MudPlayerState* player,
                                                 const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr || args.empty())
    {
        execution.title = "炼制失败";
        execution.summary = "请选择一张已掌握的配方后再炼制。";
        return execution;
    }

    const std::string recipe_key = mud_to_lower_ascii(args.front());
    const MudRecipeConfig* recipe_config = nullptr;
    for(const auto& recipe_state : player->recipes)
    {
        const auto* config = m_world->find_recipe(recipe_state.recipe_id);
        if(config != nullptr &&
           (mud_to_lower_ascii(config->recipe_id) == recipe_key || mud_to_lower_ascii(config->name) == recipe_key))
        {
            recipe_config = config;
            break;
        }
    }
    if(recipe_config == nullptr)
    {
        recipe_config = m_world->find_recipe(args.front());
    }
    if(recipe_config == nullptr)
    {
        execution.title = "配方未得";
        execution.summary = "你尚未掌握这张配方。";
        return execution;
    }

    for(const auto& ingredient : recipe_config->ingredient_items)
    {
        if(inventory_count(*player, ingredient.item_id) < ingredient.quantity)
        {
            execution.title = "材料不足";
            execution.summary = "炼制所需材料不足。";
            execution.hints.push_back("缺少：" + item_with_count_label(m_world.get(), ingredient.item_id, ingredient.quantity));
            return execution;
        }
    }

    for(const auto& ingredient : recipe_config->ingredient_items)
    {
        remove_inventory_item(player, ingredient.item_id, ingredient.quantity);
    }
    add_inventory_item(player, recipe_config->result_item_id, recipe_config->result_quantity, false);
    if(auto* recipe_state = find_recipe_state(player, recipe_config->recipe_id); recipe_state != nullptr)
    {
        recipe_state->unlocked = true;
        recipe_state->proficiency += 12;
    }
    player->profession.alchemy_level = std::max(1, player->profession.alchemy_level + 1);
    unlock_codex_by_trigger(player, "brew_recipe", recipe_config->recipe_id, &execution);
    unlock_codex_by_trigger(player, "obtain_item", recipe_config->result_item_id, &execution);
    execution.success = true;
    execution.title = "炼制完成";
    execution.summary = "你顺利完成了一次炼制。";
    execution.brew_summary = recipe_config->name + "，获得 " +
                             item_with_count_label(m_world.get(),
                                                   recipe_config->result_item_id,
                                                   recipe_config->result_quantity);
    append_event(player->account, "brew", execution.title, execution.brew_summary, &execution.events);
    return execution;
}

MudCommandExecution MudGameRuntime::execute_codex(MudPlayerState* player,
                                                  const std::vector<std::string>& args)
{
    MudCommandExecution execution;
    if(player == nullptr)
    {
        execution.title = "手册不可用";
        execution.summary = "玩家状态为空。";
        return execution;
    }
    if(args.empty())
    {
        execution.success = true;
        execution.title = "手册分类";
        execution.summary = "可查看人物志、宗门志、妖兽志、奇虫志、地理志、灵草丹药志、功法技能志、法术志、宝物阵法志、韩立年历。";
        return execution;
    }

    const auto category = args.front();
    if(args.size() == 1)
    {
        const auto entries = m_world->codex_entries_for_category(category);
        execution.success = true;
        execution.title = category;
        execution.summary = "该分类共 " + std::to_string(entries.size()) + " 条。";
        for(const auto& entry : entries)
        {
            execution.hints.push_back((is_codex_unlocked(*player, entry.entry_id) ? "已解锁 · " : "未解锁 · ") + entry.title);
        }
        return execution;
    }

    const auto key = mud_to_lower_ascii(args[1]);
    for(const auto& entry : m_world->codex_entries_for_category(category))
    {
        if(mud_to_lower_ascii(entry.entry_id) == key || mud_to_lower_ascii(entry.title) == key)
        {
            execution.success = true;
            execution.title = entry.title;
            execution.summary = entry.summary;
            execution.hints.push_back(is_codex_unlocked(*player, entry.entry_id) ? entry.content : "资料未明，需继续推进剧情解锁。");
            return execution;
        }
    }

    execution.title = "条目未找到";
    execution.summary = "当前分类下没有这个手册条目。";
    return execution;
}

MudCommandExecution MudGameRuntime::execute_save(const MudPlayerState& player) const
{
    MudCommandExecution execution;
    execution.success = true;
    execution.title = "存档落定";
    execution.summary = player.character_name.empty() ? "当前角色状态已经重新落笔存好。"
                                                     : (player.character_name + "的当前状态已经重新落笔存好。");
    execution.hints.push_back("之后即使断线或刷新，也会从这份状态继续恢复。");
    return execution;
}

const MudSceneConfig* MudGameRuntime::current_scene(const MudPlayerState& player) const
{
    const auto* scene = m_world == nullptr ? nullptr : m_world->find_scene(player.location_scene_id);
    if(scene == nullptr && m_world != nullptr)
    {
        scene = m_world->find_scene(m_world->defaults().starting_scene_id);
    }
    return scene;
}

const MudQuestConfig* MudGameRuntime::match_scene_quest(const MudPlayerState& player,
                                                        const std::string& key) const
{
    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return nullptr;
    }

    const auto normalized_key = mud_to_lower_ascii(key);
    for(const auto& npc_id : scene->npc_ids)
    {
        const auto* npc = m_world->find_npc(npc_id);
        if(npc == nullptr)
        {
            continue;
        }
        for(const auto& quest_id : npc->quest_ids)
        {
            const auto* quest = m_world->find_quest(quest_id);
            if(quest == nullptr)
            {
                continue;
            }
            if(normalized_key.empty() ||
               mud_to_lower_ascii(quest->quest_id) == normalized_key ||
               mud_to_lower_ascii(quest->title) == normalized_key)
            {
                return quest;
            }
        }
    }
    return nullptr;
}

const MudNpcConfig* MudGameRuntime::match_scene_npc(const MudPlayerState& player,
                                                    const std::string& key) const
{
    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return nullptr;
    }

    const auto normalized_key = mud_to_lower_ascii(key);
    for(const auto& npc_id : scene->npc_ids)
    {
        const auto* npc = m_world->find_npc(npc_id);
        if(npc != nullptr && (mud_to_lower_ascii(npc->npc_id) == normalized_key || mud_to_lower_ascii(npc->name) == normalized_key))
        {
            return npc;
        }
    }
    return nullptr;
}

const MudMonsterConfig* MudGameRuntime::match_scene_monster(const MudPlayerState& player,
                                                            const std::string& key) const
{
    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return nullptr;
    }

    const auto normalized_key = mud_to_lower_ascii(key);
    for(const auto& monster_id : scene->monster_ids)
    {
        const auto* monster = m_world->find_monster(monster_id);
        if(monster != nullptr &&
           (normalized_key.empty() || mud_to_lower_ascii(monster->monster_id) == normalized_key ||
            mud_to_lower_ascii(monster->name) == normalized_key))
        {
            return monster;
        }
    }
    return nullptr;
}

const MudGameRuntime::OnlinePresenceState* MudGameRuntime::match_scene_player_presence(const MudPlayerState& player,
                                                                                        const std::string& key) const
{
    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return nullptr;
    }

    const auto normalized_key = mud_to_lower_ascii(key);
    const auto now = mud_now_ms();
    for(const auto& [account, presence] : m_online_presence)
    {
        if(account.empty() || account == player.account)
        {
            continue;
        }
        if(presence.last_seen_ms <= 0 || now - presence.last_seen_ms > kScenePresenceTtlMs)
        {
            continue;
        }
        if(presence.player.location_scene_id != scene->scene_id)
        {
            continue;
        }
        if(normalized_key.empty() || mud_to_lower_ascii(account) == normalized_key ||
           mud_to_lower_ascii(presence.player.character_name) == normalized_key)
        {
            return &presence;
        }
    }
    return nullptr;
}

const MudResourceNodeConfig* MudGameRuntime::match_scene_resource_node(const MudPlayerState& player,
                                                                       const std::string& key) const
{
    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return nullptr;
    }

    const auto normalized_key = mud_to_lower_ascii(key);
    for(const auto& node_id : scene->resource_node_ids)
    {
        const auto* node = m_world->find_resource_node(node_id);
        if(node != nullptr &&
           (normalized_key.empty() || mud_to_lower_ascii(node->node_id) == normalized_key ||
            mud_to_lower_ascii(node->name) == normalized_key))
        {
            return node;
        }
    }
    return nullptr;
}

const MudGroundLootConfig* MudGameRuntime::match_scene_ground_loot(const MudPlayerState& player,
                                                                   const std::string& key) const
{
    const auto* scene = current_scene(player);
    if(scene == nullptr)
    {
        return nullptr;
    }

    const auto normalized_key = mud_to_lower_ascii(key);
    for(const auto& loot_id : scene->ground_loot_ids)
    {
        const auto* loot = m_world->find_ground_loot(loot_id);
        const auto* item = loot == nullptr ? nullptr : m_world->find_item(loot->item_id);
        if(loot != nullptr &&
           (normalized_key.empty() || mud_to_lower_ascii(loot->loot_id) == normalized_key ||
            (item != nullptr && mud_to_lower_ascii(item->name) == normalized_key) ||
            mud_to_lower_ascii(loot->item_id) == normalized_key))
        {
            return loot;
        }
    }
    return nullptr;
}

int MudGameRuntime::find_inventory_index(const MudPlayerState& player, const std::string& key) const
{
    const auto normalized_key = mud_to_lower_ascii(key);
    for(size_t index = 0; index < player.inventory.size(); ++index)
    {
        const auto& item = player.inventory[index];
        const auto* config = m_world->find_item(item.item_id);
        const std::string item_name = config == nullptr ? item.item_id : config->name;
        if(mud_to_lower_ascii(item.item_id) == normalized_key || mud_to_lower_ascii(item_name) == normalized_key)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int MudGameRuntime::inventory_count(const MudPlayerState& player, const std::string& item_id) const
{
    int total = 0;
    for(const auto& item : player.inventory)
    {
        if(item.item_id == item_id)
        {
            total += item.quantity;
        }
    }
    return total;
}

MudQuestState* MudGameRuntime::find_quest_state(MudPlayerState* player, const std::string& quest_id) const
{
    if(player == nullptr)
    {
        return nullptr;
    }
    for(auto& quest : player->quests)
    {
        if(quest.quest_id == quest_id)
        {
            return &quest;
        }
    }
    return nullptr;
}

const MudQuestState* MudGameRuntime::find_quest_state(const MudPlayerState& player, const std::string& quest_id) const
{
    for(const auto& quest : player.quests)
    {
        if(quest.quest_id == quest_id)
        {
            return &quest;
        }
    }
    return nullptr;
}

MudSkillState* MudGameRuntime::find_skill_state(MudPlayerState* player, const std::string& skill_id) const
{
    if(player == nullptr)
    {
        return nullptr;
    }
    for(auto& skill : player->skills)
    {
        if(skill.skill_id == skill_id)
        {
            return &skill;
        }
    }
    return nullptr;
}

MudSpellState* MudGameRuntime::find_spell_state(MudPlayerState* player, const std::string& spell_id) const
{
    if(player == nullptr)
    {
        return nullptr;
    }
    for(auto& spell : player->spells)
    {
        if(spell.spell_id == spell_id)
        {
            return &spell;
        }
    }
    return nullptr;
}

MudRecipeState* MudGameRuntime::find_recipe_state(MudPlayerState* player, const std::string& recipe_id) const
{
    if(player == nullptr)
    {
        return nullptr;
    }
    for(auto& recipe : player->recipes)
    {
        if(recipe.recipe_id == recipe_id)
        {
            return &recipe;
        }
    }
    return nullptr;
}

void MudGameRuntime::add_inventory_item(MudPlayerState* player,
                                        const std::string& item_id,
                                        int quantity,
                                        bool equipped) const
{
    if(player == nullptr || item_id.empty() || quantity <= 0)
    {
        return;
    }
    const auto* config = m_world->find_item(item_id);
    if(config != nullptr && config->equipable)
    {
        for(int index = 0; index < quantity; ++index)
        {
            player->inventory.push_back(MudInventoryItemState{item_id, 1, equipped && index == 0});
        }
        return;
    }
    for(auto& item : player->inventory)
    {
        if(item.item_id == item_id)
        {
            item.quantity += quantity;
            item.equipped = item.equipped || equipped;
            return;
        }
    }
    player->inventory.push_back(MudInventoryItemState{item_id, quantity, equipped});
}

bool MudGameRuntime::remove_inventory_item(MudPlayerState* player,
                                           const std::string& item_id,
                                           int quantity) const
{
    if(player == nullptr || item_id.empty() || quantity <= 0)
    {
        return false;
    }

    if(inventory_count(*player, item_id) < quantity)
    {
        return false;
    }

    int remaining = quantity;
    for(auto iter = player->inventory.begin(); iter != player->inventory.end() && remaining > 0;)
    {
        if(iter->item_id != item_id)
        {
            ++iter;
            continue;
        }

        const int deduct = std::min(iter->quantity, remaining);
        iter->quantity -= deduct;
        remaining -= deduct;
        if(iter->quantity <= 0)
        {
            iter = player->inventory.erase(iter);
            continue;
        }
        ++iter;
    }
    return remaining == 0;
}

void MudGameRuntime::refresh_quest_progress(MudPlayerState* player) const
{
    if(player == nullptr)
    {
        return;
    }

    for(auto& quest_state : player->quests)
    {
        if(quest_state.status != "active")
        {
            continue;
        }

        const auto* quest = m_world->find_quest(quest_state.quest_id);
        if(quest == nullptr)
        {
            continue;
        }
        quest_state.progress = inventory_count(*player, quest->required_item_id);
    }
}

std::string MudGameRuntime::realm_name_for_stage(int stage) const
{
    if(m_world == nullptr)
    {
        return "炼气";
    }

    const auto& realm_names = m_world->defaults().realm_names;
    if(stage >= 0 && stage < static_cast<int>(realm_names.size()))
    {
        return realm_names[static_cast<size_t>(stage)];
    }
    if(!realm_names.empty())
    {
        return realm_names.back();
    }
    return "炼气";
}

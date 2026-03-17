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

namespace
{

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
                                                    const std::string& character_name) const
{
    return make_default_player(account, character_name);
}

MudPlayerState MudGameRuntime::make_default_player(const std::string& account,
                                                   const std::string& character_name) const
{
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
    for(const auto& starter_item : m_world->defaults().starter_inventory)
    {
        add_inventory_item(&player, starter_item.item_id, starter_item.quantity, starter_item.equipped);
    }
    refresh_quest_progress(&player);
    return player;
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
    }

    snapshot->clear_known_commands();
    snapshot->add_known_commands("look");
    snapshot->add_known_commands("map");
    snapshot->add_known_commands("go <direction>");
    snapshot->add_known_commands("talk <npc>");
    snapshot->add_known_commands("accept <quest>");
    snapshot->add_known_commands("submit <quest>");
    snapshot->add_known_commands("fight <target>");
    snapshot->add_known_commands("use <item>");
    snapshot->add_known_commands("flee");
    snapshot->add_known_commands("practice <skill>");
    snapshot->add_known_commands("breakthrough");
    snapshot->add_known_commands("buy <item>");
    snapshot->add_known_commands("sell <item>");
    snapshot->add_known_commands("join <sect>");
    snapshot->add_known_commands("chat <channel> <message>");
    snapshot->set_recommended_poll_interval_ms(1500);
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
    snapshot->clear_exits();
    snapshot->clear_npcs();
    snapshot->clear_monsters();
    snapshot->clear_shops();

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
        node->set_hint(npc->hint);
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
        }
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

void MudGameRuntime::build_bootstrap_response(const std::string& account,
                                              const std::optional<MudPlayerState>& player,
                                              mud::BootstrapResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    response->set_need_create_character(!player.has_value());
    if(!player.has_value())
    {
        MudPlayerState preview;
        preview.account = account;
        preview.location_scene_id = m_world->defaults().starting_scene_id;
        fill_scene_snapshot(preview, response->mutable_scene());
        http_code_message::gateway::set_code_message(response,
                                                     http_code_message::gateway::code::kSuccess,
                                                     "character not found");
        response->set_next_event_id(m_next_event_id == 0 ? 0 : (m_next_event_id - 1));
        return;
    }

    fill_player_snapshot(*player, response->mutable_player());
    fill_scene_snapshot(*player, response->mutable_scene());
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
    response->set_next_event_id(m_next_event_id == 0 ? 0 : (m_next_event_id - 1));
}

void MudGameRuntime::build_create_character_response(const MudPlayerState& player,
                                                     mud::CharacterCreateResponse* response)
{
    if(response == nullptr)
    {
        return;
    }

    std::vector<MudEventEnvelope> events;
    append_event(player.account,
                 "system",
                 "踏入修仙路",
                 player.character_name + "自七玄门山脚启程，正式踏上凡人修仙之路。",
                 &events);
    fill_player_snapshot(player, response->mutable_player());
    fill_scene_snapshot(player, response->mutable_scene());
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

    fill_player_snapshot(player, response->mutable_player());
    fill_scene_snapshot(player, response->mutable_scene());
    add_events_to_response(execution.events, response->mutable_events());
    response->set_next_event_id(execution.events.empty() ? (m_next_event_id == 0 ? 0 : (m_next_event_id - 1)) : execution.events.back().event_id);
    http_code_message::gateway::set_code_message(response,
                                                 execution.success ? http_code_message::gateway::code::kSuccess
                                                                   : http_code_message::gateway::code::kInvalidMudCommand,
                                                 execution.summary);
}

void MudGameRuntime::build_feed_response(const std::string& account,
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
    http_code_message::gateway::set_code_message(response,
                                                 http_code_message::gateway::code::kSuccess,
                                                 http_code_message::gateway::message::kOk);
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

    auto player = make_default_player(account, normalized_name);
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

    auto parsed = parse_mud_command(command_text);
    if(parsed.verb.empty())
    {
        execution.title = "命令为空";
        execution.summary = "请输入修仙指令。";
        execution.hints = {"例如：look", "例如：go north", "例如：fight 青木狼"};
        return execution;
    }

    if(parsed.verb == "look")
    {
        return execute_look(player);
    }
    if(parsed.verb == "map")
    {
        return execute_map(player);
    }
    if(parsed.verb == "go")
    {
        return execute_go(player, parsed.args);
    }
    if(parsed.verb == "talk")
    {
        return execute_talk(player, parsed.args);
    }
    if(parsed.verb == "accept")
    {
        return execute_accept(player, parsed.args);
    }
    if(parsed.verb == "submit")
    {
        return execute_submit(player, parsed.args);
    }
    if(parsed.verb == "fight")
    {
        return execute_fight(player, parsed.args);
    }
    if(parsed.verb == "use")
    {
        return execute_use(player, parsed.args);
    }
    if(parsed.verb == "flee")
    {
        return execute_flee();
    }
    if(parsed.verb == "practice")
    {
        return execute_practice(player, parsed.args);
    }
    if(parsed.verb == "breakthrough")
    {
        return execute_breakthrough(player);
    }
    if(parsed.verb == "buy")
    {
        return execute_buy(player, parsed.args);
    }
    if(parsed.verb == "sell")
    {
        return execute_sell(player, parsed.args);
    }
    if(parsed.verb == "join")
    {
        return execute_join(player, parsed.args);
    }
    if(parsed.verb == "chat")
    {
        return execute_chat(*player, parsed.raw_args);
    }

    execution.title = "未知指令";
    execution.summary = "未识别的指令：" + parsed.verb;
    execution.hints = {"可用指令：look / map / go / talk / accept / submit / fight / use / practice / breakthrough"};
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
        execution.hints.push_back(entry.first + " -> " + (target == nullptr ? entry.second : target->name));
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
        execution.summary = "请使用 go <direction>。";
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
        append_event(player->account,
                     "move",
                     execution.title,
                     execution.summary,
                     &execution.events);
        return execution;
    }

    execution.title = "路途不通";
    execution.summary = "当前方位没有道路可走。";
    execution.hints.push_back("先用 map 查看当前出口");
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
        execution.hints.push_back("先用 look 查看场景人物");
        return execution;
    }

    execution.success = true;
    execution.title = "与" + npc->name + "交谈";
    execution.summary = npc->dialogue.empty() ? (npc->name + "静静看着你。") : npc->dialogue;
    for(const auto& quest_id : npc->quest_ids)
    {
        const auto* quest = m_world->find_quest(quest_id);
        if(quest == nullptr)
        {
            continue;
        }
        execution.hints.push_back("可接任务：accept " + quest->quest_id + "（" + quest->title + "）");
    }
    if(!npc->sect_offer_id.empty())
    {
        if(const auto* sect = m_world->find_sect(npc->sect_offer_id); sect != nullptr)
        {
            execution.hints.push_back("若想拜入" + sect->name + "，可使用：join " + sect->sect_id);
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

    const auto* quest = args.empty() ? nullptr : match_scene_quest(*player, args.front());
    if(quest == nullptr)
    {
        execution.title = "未找到任务";
        execution.summary = "当前场景没有这个任务可提交。";
        return execution;
    }

    auto* quest_state = find_quest_state(player, quest->quest_id);
    if(quest_state == nullptr || quest_state->status != "active")
    {
        execution.title = "任务未开始";
        execution.summary = "你尚未接取这个任务。";
        return execution;
    }

    refresh_quest_progress(player);
    if(quest_state->progress < quest->required_item_count)
    {
        execution.title = "材料未齐";
        execution.summary = "任务还未达到提交条件。";
        execution.hints.push_back("需要：" + quest->required_item_id + " x" + std::to_string(quest->required_item_count));
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
    if(!quest->reward_item_id.empty() && quest->reward_item_count > 0)
    {
        add_inventory_item(player, quest->reward_item_id, quest->reward_item_count, false);
    }
    refresh_quest_progress(player);

    execution.success = true;
    execution.title = "任务完成";
    execution.summary = "你完成了「" + quest->title + "」，获得灵石 " +
                        std::to_string(quest->reward_spirit_stone) + "、修为 " +
                        std::to_string(quest->reward_exp) + "。";
    if(!quest->reward_item_id.empty() && quest->reward_item_count > 0)
    {
        execution.hints.push_back("奖励物品：" + quest->reward_item_id + " x" + std::to_string(quest->reward_item_count));
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
        execution.hints.push_back("先用 look 查看场景妖兽");
        return execution;
    }

    const int player_damage = std::max(1, player->attack_power + player->skill_level * 2 - monster->defense);
    const int monster_damage = std::max(1, monster->attack - std::max(1, player->defense_power / 2));
    const bool player_wins = player_damage >= monster->hp || (player->hp + player->attack_power + player->defense_power) >=
                                                          (monster->hp + monster->attack + monster->defense);

    if(player_wins)
    {
        player->hp = std::max(1, player->hp - std::max(1, monster_damage / 2));
        player->exp += monster->reward_exp;
        player->spirit_stone += monster->reward_spirit_stone;
        if(!monster->drop_item_id.empty() && monster->drop_item_count > 0)
        {
            add_inventory_item(player, monster->drop_item_id, monster->drop_item_count, false);
        }
        refresh_quest_progress(player);

        execution.success = true;
        execution.title = "战斗得胜";
        execution.summary = "你击败了「" + monster->name + "」，获得修为 " +
                            std::to_string(monster->reward_exp) + "、灵石 " +
                            std::to_string(monster->reward_spirit_stone) + "。";
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
    execution.hints.push_back("可使用 use small_recover_pill 恢复气血");
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
        execution.summary = "请使用 use <item>。";
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
    if(player->exp / 100 > player->skill_level)
    {
        ++player->skill_level;
        player->attack_power += 1;
        player->defense_power += 1;
    }

    execution.success = true;
    execution.title = "吐纳修炼";
    execution.summary = "你运转「" + player->primary_skill + "」，修为增加 " + std::to_string(gain) + "。";
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
        execution.hints.push_back("继续 practice " + player->primary_skill + " 或 fight <target>");
        return execution;
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
        execution.summary = "请使用 buy <item>。";
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
        execution.summary = "请使用 sell <item>。";
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
    remove_inventory_item(player, item_state.item_id, 1);
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

    bool has_completed_quest = false;
    for(const auto& quest : player->quests)
    {
        if(quest.status == "completed")
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
    append_event("",
                 "sect",
                 execution.title,
                 player->character_name + "加入了" + matched_sect->name + "。",
                 &execution.events);
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
        execution.summary = "请使用 chat <channel> <message>。";
        return execution;
    }

    const auto channel = mud_trim(trimmed.substr(0, split_pos));
    const auto message = mud_trim(trimmed.substr(split_pos + 1));
    if(channel.empty() || message.empty())
    {
        execution.title = "发言失败";
        execution.summary = "频道和消息都不能为空。";
        return execution;
    }

    execution.success = true;
    execution.title = "频道发言";
    execution.summary = "你向[" + channel + "]频道发送了消息。";
    append_event("",
                 "chat",
                 "[" + channel + "] " + player.character_name,
                 message,
                 &execution.events);
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

void MudGameRuntime::add_inventory_item(MudPlayerState* player,
                                        const std::string& item_id,
                                        int quantity,
                                        bool equipped) const
{
    if(player == nullptr || item_id.empty() || quantity <= 0)
    {
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

    for(auto iter = player->inventory.begin(); iter != player->inventory.end(); ++iter)
    {
        if(iter->item_id != item_id)
        {
            continue;
        }
        if(iter->quantity < quantity)
        {
            return false;
        }
        iter->quantity -= quantity;
        if(iter->quantity == 0)
        {
            player->inventory.erase(iter);
        }
        return true;
    }
    return false;
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

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

#include "mud_player_repository.h"
#include "mud_world.h"

#include "protocol/mud.pb.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class MudGameRuntime
{
public:
    MudGameRuntime(std::shared_ptr<MudWorld> world,
                   std::shared_ptr<IMudPlayerRepository> repository);

public:
    bool ready() const;
    std::string ready_error() const;
    void poll();
    bool verify_account_match(const std::string& jwt_account,
                              const std::string& requested_account,
                              std::string* resolved_account) const;
    MudPlayerState build_default_player(const std::string& account,
                                        const std::string& character_name,
                                        const std::string& origin_id = "",
                                        const std::string& background_id = "") const;
    void build_bootstrap_response(const std::string& account,
                                  const std::optional<MudPlayerState>& player,
                                  mud::BootstrapResponse* response);
    void build_create_character_response(const MudPlayerState& player,
                                         mud::CharacterCreateResponse* response);
    void build_command_response(const MudPlayerState& player,
                                const std::string& command,
                                const MudCommandExecution& execution,
                                mud::CommandExecuteResponse* response);
    void build_feed_response(const std::string& account,
                             const std::optional<MudPlayerState>& player,
                             uint64_t after_event_id,
                             int limit,
                             mud::FeedPullResponse* response);
    void build_codex_list_response(const MudPlayerState& player,
                                   const std::string& category,
                                   mud::CodexListResponse* response) const;
    void build_codex_detail_response(const MudPlayerState& player,
                                     const std::string& entry_id,
                                     mud::CodexDetailResponse* response) const;
    void build_rank_response(MudLeaderboardType leaderboard_type,
                             const std::vector<MudLeaderboardEntry>& entries,
                             mud::RankListResponse* response) const;
    void merge_persisted_events(const std::vector<MudEventEnvelope>& events);
    void restore_team_state(const std::vector<MudPlayerState>& team_members);
    void forget_team_state(const std::string& account);
    MudCommandExecution run_command(MudPlayerState* player,
                                    const std::string& command);
    bool normalize_player_state(MudPlayerState* player) const;

    coro_task_t bootstrap_async(const std::string& account,
                                mud::BootstrapResponse* response);
    coro_task_t create_character_async(const std::string& account,
                                       const std::string& character_name,
                                       const std::string& origin_id,
                                       const std::string& background_id,
                                       mud::CharacterCreateResponse* response);
    coro_task_t execute_command_async(const std::string& account,
                                      const std::string& command,
                                      mud::CommandExecuteResponse* response);
    coro_task_t pull_feed_async(const std::string& account,
                                uint64_t after_event_id,
                                int limit,
                                mud::FeedPullResponse* response);

private:
    struct OnlinePresenceState
    {
        MudPlayerState player;
        int64_t last_seen_ms = 0;
    };

private:
    MudPlayerState make_default_player(const std::string& account,
                                       const std::string& character_name,
                                       const std::string& origin_id,
                                       const std::string& background_id) const;
    void fill_player_snapshot(const MudPlayerState& player,
                              mud::PlayerSnapshot* snapshot) const;
    void fill_scene_snapshot(const MudPlayerState& player,
                             mud::SceneSnapshot* snapshot) const;
    void fill_origin_state(const MudPlayerState& player,
                           mud::RaceState* state) const;
    void fill_background_state(const MudPlayerState& player,
                               mud::BackgroundState* state) const;
    void fill_base_attributes(const MudBaseAttributeState& state,
                              mud::BaseAttributeState* output) const;
    void fill_status_attributes(const MudStatusAttributeState& state,
                                mud::StatusAttributeState* output) const;
    void fill_combat_attributes(const MudCombatAttributeState& state,
                                mud::CombatAttributeState* output) const;
    void fill_summary_entry(const MudSummaryEntry& entry,
                            mud::SummaryEntry* output) const;
    void fill_structured_panel(const MudStructuredPanelState& panel,
                               mud::StructuredPanel* output) const;
    void fill_route_summary(const MudRouteSummaryState& route,
                            mud::RouteSummary* output) const;
    void fill_weekly_event_summary(const MudWeeklyEventSummaryState& event,
                                   mud::WeeklyEventSummary* output) const;
    void fill_codex_summary(const MudPlayerState& player,
                            const MudCodexEntryConfig& entry,
                            mud::CodexSummary* output) const;
    void sync_origin_from_world(MudPlayerState* player) const;
    void sync_background_from_world(MudPlayerState* player) const;
    void derive_player_combat_state(MudPlayerState* player) const;
    std::vector<std::string> titles_for_player(const MudPlayerState& player) const;
    std::string current_chief_title_for_player(const MudPlayerState& player) const;
    void fill_command_catalog(const MudPlayerState& player,
                              google::protobuf::RepeatedPtrField<mud::CommandDefinition>* output) const;
    void fill_gameplay_guidance(const MudPlayerState& player,
                                mud::BootstrapResponse* response) const;
    bool unlock_codex_entry(MudPlayerState* player,
                            const std::string& entry_id,
                            MudCommandExecution* execution) const;
    void unlock_codex_by_trigger(MudPlayerState* player,
                                 const std::string& trigger,
                                 const std::string& target_id,
                                 MudCommandExecution* execution) const;
    bool is_codex_unlocked(const MudPlayerState& player, const std::string& entry_id) const;
    void remember_scene_presence(const MudPlayerState& player);
    void prune_scene_presence();
    void append_event(const std::string& target_account,
                      const std::string& type,
                      const std::string& title,
                      const std::string& content,
                      std::vector<MudEventEnvelope>* out_batch);
    void add_events_to_response(const std::vector<MudEventEnvelope>& events,
                                google::protobuf::RepeatedPtrField<mud::GameEvent>* out_events) const;
    void trim_events();
    std::vector<MudEventEnvelope> recent_events_for_account(const std::string& account, int limit) const;
    void fill_team_snapshot(const MudPlayerState& player,
                            mud::TeamState* snapshot) const;
    void maybe_emit_world_event();
    std::vector<std::string> unlocked_regions_for_player(const MudPlayerState& player) const;
    std::vector<std::string> unlocked_routes_for_player(const MudPlayerState& player) const;
    std::string progression_chapter_for_player(const MudPlayerState& player) const;
    int64_t sect_contribution_for_player(const MudPlayerState& player) const;
    std::string stage_label_for_player(const MudPlayerState& player) const;
    bool has_newbie_protection(const MudPlayerState& player) const;
    std::string newbie_protection_summary_for_player(const MudPlayerState& player) const;
    std::vector<MudRouteSummaryState> route_summaries_for_player(const MudPlayerState& player) const;
    std::vector<MudWeeklyEventSummaryState> weekly_events_for_player(const MudPlayerState& player) const;
    std::string recommended_loop_for_player(const MudPlayerState& player) const;
    std::string identity_track_for_player(const MudPlayerState& player) const;
    int rank_level_for_player(const MudPlayerState& player) const;
    std::string contribution_state_for_player(const MudPlayerState& player) const;
    std::string reputation_state_for_player(const MudPlayerState& player) const;
    int unread_board_count_for_player(const MudPlayerState& player) const;
    std::vector<MudSummaryEntry> board_entries_for_player(const MudPlayerState& player,
                                                          const MudSceneConfig* scene) const;
    std::vector<MudSummaryEntry> work_entries_for_player(const MudPlayerState& player,
                                                         const MudSceneConfig* scene) const;
    std::vector<MudSummaryEntry> duty_entries_for_player(const MudPlayerState& player,
                                                         const MudSceneConfig* scene) const;
    std::vector<MudSummaryEntry> wanted_entries_for_player(const MudPlayerState& player,
                                                           const MudSceneConfig* scene) const;
    std::vector<MudSummaryEntry> travel_entries_for_player(const MudPlayerState& player,
                                                           const MudSceneConfig* scene) const;
    std::vector<MudSummaryEntry> claim_entries_for_player(const MudPlayerState& player) const;
    std::string resource_refresh_summary_for_scene(const MudPlayerState& player,
                                                   const MudSceneConfig& scene) const;
    int64_t leaderboard_score(MudLeaderboardType leaderboard_type, const MudPlayerState& player) const;
    std::string leaderboard_extra(MudLeaderboardType leaderboard_type, const MudPlayerState& player) const;
    std::vector<const MudEventEnvelope*> board_posts_for_scene(const MudPlayerState& player,
                                                               const std::string& scene_id,
                                                               int limit) const;
    const MudHelpTopicConfig* match_help_topic(const std::string& key) const;
    MudCommandExecution execute_command(MudPlayerState* player,
                                        const std::string& command_text);
    MudCommandExecution execute_look(MudPlayerState* player) const;
    MudCommandExecution execute_map(MudPlayerState* player) const;
    MudCommandExecution execute_go(MudPlayerState* player,
                                   const std::vector<std::string>& args);
    MudCommandExecution execute_talk(MudPlayerState* player,
                                     const std::vector<std::string>& args) const;
    MudCommandExecution execute_ask(MudPlayerState* player,
                                    const std::vector<std::string>& args) const;
    MudCommandExecution execute_accept(MudPlayerState* player,
                                       const std::vector<std::string>& args);
    MudCommandExecution execute_submit(MudPlayerState* player,
                                       const std::vector<std::string>& args);
    MudCommandExecution execute_fight(MudPlayerState* player,
                                      const std::vector<std::string>& args);
    MudCommandExecution execute_use(MudPlayerState* player,
                                    const std::vector<std::string>& args);
    MudCommandExecution execute_flee() const;
    MudCommandExecution execute_practice(MudPlayerState* player,
                                         const std::vector<std::string>& args);
    MudCommandExecution execute_breakthrough(MudPlayerState* player);
    MudCommandExecution execute_buy(MudPlayerState* player,
                                    const std::vector<std::string>& args);
    MudCommandExecution execute_sell(MudPlayerState* player,
                                     const std::vector<std::string>& args);
    MudCommandExecution execute_join(MudPlayerState* player,
                                     const std::vector<std::string>& args);
    MudCommandExecution execute_team(MudPlayerState* player,
                                     const std::vector<std::string>& args);
    MudCommandExecution execute_event(const MudPlayerState& player) const;
    MudCommandExecution execute_chat(const MudPlayerState& player,
                                     const std::string& raw_args);
    MudCommandExecution execute_say(const MudPlayerState& player,
                                    const std::string& raw_args);
    MudCommandExecution execute_tell(MudPlayerState* player,
                                     const std::vector<std::string>& args,
                                     const std::string& raw_args);
    MudCommandExecution execute_reply(MudPlayerState* player,
                                      const std::string& raw_args);
    MudCommandExecution execute_emote(const MudPlayerState& player,
                                      const std::string& raw_args);
    MudCommandExecution execute_follow(MudPlayerState* player,
                                       const std::vector<std::string>& args);
    MudCommandExecution execute_guard(MudPlayerState* player,
                                      const std::vector<std::string>& args);
    MudCommandExecution execute_trade(MudPlayerState* player,
                                      const std::vector<std::string>& args);
    MudCommandExecution execute_challenge(MudPlayerState* player,
                                          const std::vector<std::string>& args);
    MudCommandExecution execute_score(const MudPlayerState& player) const;
    MudCommandExecution execute_help(const MudPlayerState& player,
                                     const std::vector<std::string>& args) const;
    MudCommandExecution execute_commands(const MudPlayerState& player) const;
    MudCommandExecution execute_newbie(const MudPlayerState& player) const;
    MudCommandExecution execute_hp(const MudPlayerState& player) const;
    MudCommandExecution execute_rank(const MudPlayerState& player,
                                     const std::vector<std::string>& args) const;
    MudCommandExecution execute_board(const MudPlayerState& player) const;
    MudCommandExecution execute_read(MudPlayerState* player,
                                     const std::vector<std::string>& args) const;
    MudCommandExecution execute_post(MudPlayerState* player,
                                     const std::string& raw_args);
    MudCommandExecution execute_discard(MudPlayerState* player,
                                        const std::vector<std::string>& args) const;
    MudCommandExecution execute_work(const MudPlayerState& player) const;
    MudCommandExecution execute_week(const MudPlayerState& player) const;
    MudCommandExecution execute_duty(const MudPlayerState& player) const;
    MudCommandExecution execute_wanted(const MudPlayerState& player) const;
    MudCommandExecution execute_travel(const MudPlayerState& player) const;
    MudCommandExecution execute_claim(MudPlayerState* player,
                                      const std::vector<std::string>& args);
    MudCommandExecution execute_contribute(MudPlayerState* player,
                                           const std::vector<std::string>& args);
    MudCommandExecution execute_tasks(const MudPlayerState& player) const;
    MudCommandExecution execute_skills(const MudPlayerState& player) const;
    MudCommandExecution execute_spells(const MudPlayerState& player) const;
    MudCommandExecution execute_family(const MudPlayerState& player) const;
    MudCommandExecution execute_who(const MudPlayerState& player) const;
    MudCommandExecution execute_rumor(const MudPlayerState& player) const;
    MudCommandExecution execute_listen(const MudPlayerState& player) const;
    MudCommandExecution execute_journal(const MudPlayerState& player) const;
    MudCommandExecution execute_bag(const MudPlayerState& player) const;
    MudCommandExecution execute_inspect(MudPlayerState* player,
                                        const std::vector<std::string>& args);
    MudCommandExecution execute_loot(MudPlayerState* player,
                                     const std::vector<std::string>& args);
    MudCommandExecution execute_harvest(MudPlayerState* player,
                                        const std::vector<std::string>& args);
    MudCommandExecution execute_cast(MudPlayerState* player,
                                     const std::vector<std::string>& args);
    MudCommandExecution execute_meditate(MudPlayerState* player);
    MudCommandExecution execute_brew(MudPlayerState* player,
                                     const std::vector<std::string>& args);
    MudCommandExecution execute_codex(MudPlayerState* player,
                                      const std::vector<std::string>& args);
    MudCommandExecution execute_save(const MudPlayerState& player) const;

    const MudSceneConfig* current_scene(const MudPlayerState& player) const;
    const MudQuestConfig* match_scene_quest(const MudPlayerState& player,
                                            const std::string& key) const;
    const MudNpcConfig* match_scene_npc(const MudPlayerState& player,
                                        const std::string& key) const;
    const MudMonsterConfig* match_scene_monster(const MudPlayerState& player,
                                                const std::string& key) const;
    const OnlinePresenceState* match_scene_player_presence(const MudPlayerState& player,
                                                           const std::string& key) const;
    const MudResourceNodeConfig* match_scene_resource_node(const MudPlayerState& player,
                                                           const std::string& key) const;
    const MudGroundLootConfig* match_scene_ground_loot(const MudPlayerState& player,
                                                       const std::string& key) const;
    int find_inventory_index(const MudPlayerState& player, const std::string& key) const;
    int inventory_count(const MudPlayerState& player, const std::string& item_id) const;
    MudQuestState* find_quest_state(MudPlayerState* player, const std::string& quest_id) const;
    const MudQuestState* find_quest_state(const MudPlayerState& player, const std::string& quest_id) const;
    MudSkillState* find_skill_state(MudPlayerState* player, const std::string& skill_id) const;
    MudSpellState* find_spell_state(MudPlayerState* player, const std::string& spell_id) const;
    MudRecipeState* find_recipe_state(MudPlayerState* player, const std::string& recipe_id) const;
    void add_inventory_item(MudPlayerState* player,
                            const std::string& item_id,
                            int quantity,
                            bool equipped = false) const;
    bool remove_inventory_item(MudPlayerState* player,
                               const std::string& item_id,
                               int quantity) const;
    void refresh_quest_progress(MudPlayerState* player) const;
    std::string realm_name_for_stage(int stage) const;

private:
    std::shared_ptr<MudWorld> m_world;
    std::shared_ptr<IMudPlayerRepository> m_repository;
    std::vector<MudEventEnvelope> m_events;
    std::unordered_map<std::string, std::string> m_character_names;
    std::unordered_map<std::string, OnlinePresenceState> m_online_presence;
    std::unordered_map<std::string, MudTeamState> m_teams;
    std::unordered_map<std::string, std::string> m_team_by_account;
    uint64_t m_next_event_id = 1;
    int64_t m_last_world_event_ms = 0;
    std::size_t m_world_event_cursor = 0;
    std::string m_ready_error;
};

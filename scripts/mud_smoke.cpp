//
// Lightweight protobuf-over-HTTP smoke client for the MUD flow.
// It talks directly to the existing manager/login/game services using raw TCP
// so it can run inside WSL without extra Python or Node dependencies.
//

#include "protocol/gateway.pb.h"
#include "protocol/mud.pb.h"
#include "mud_game_runtime.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{

struct HttpResponse
{
    int status = 0;
    std::string body;
};

std::string trim_copy(std::string value)
{
    auto not_space = [](unsigned char ch) {
        return !std::isspace(ch);
    };
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
                value.end());
    return value;
}

HttpResponse post_protobuf(const std::string& host,
                           uint16_t port,
                           const std::string& path,
                           const std::string& body,
                           const std::string& bearer_token)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        throw std::runtime_error("inet_pton() failed for host " + host);
    }

    int fd = -1;
    std::string connect_error;
    constexpr int kConnectAttempts = 20;
    for(int attempt = 0; attempt < kConnectAttempts; ++attempt)
    {
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0)
        {
            throw std::runtime_error("socket() failed");
        }

        if(::connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0)
        {
            connect_error.clear();
            break;
        }

        connect_error = std::strerror(errno);
        ::close(fd);
        fd = -1;
        if(errno != ECONNREFUSED && errno != ETIMEDOUT)
        {
            throw std::runtime_error("connect() failed: " + connect_error);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if(fd < 0)
    {
        throw std::runtime_error("connect() failed: " + connect_error);
    }

    std::ostringstream request;
    request << "POST " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Content-Type: application/x-protobuf\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "Connection: close\r\n";
    if(!bearer_token.empty())
    {
        request << "Authorization: Bearer " << bearer_token << "\r\n";
    }
    request << "\r\n";
    request << body;

    const std::string serialized = request.str();
    size_t written = 0;
    while(written < serialized.size())
    {
        const ssize_t result =
            ::send(fd, serialized.data() + static_cast<ssize_t>(written), serialized.size() - written, 0);
        if(result <= 0)
        {
            const std::string error = std::strerror(errno);
            ::close(fd);
            throw std::runtime_error("send() failed: " + error);
        }
        written += static_cast<size_t>(result);
    }

    std::string response;
    std::array<char, 8192> buffer{};
    while(true)
    {
        const ssize_t read_bytes = ::recv(fd, buffer.data(), buffer.size(), 0);
        if(read_bytes < 0)
        {
            const std::string error = std::strerror(errno);
            ::close(fd);
            throw std::runtime_error("recv() failed: " + error);
        }
        if(read_bytes == 0)
        {
            break;
        }
        response.append(buffer.data(), static_cast<size_t>(read_bytes));
    }
    ::close(fd);

    const size_t header_end = response.find("\r\n\r\n");
    if(header_end == std::string::npos)
    {
        throw std::runtime_error("invalid HTTP response");
    }

    const std::string header_text = response.substr(0, header_end);
    const std::string body_text = response.substr(header_end + 4);

    std::istringstream header_stream(header_text);
    std::string status_line;
    std::getline(header_stream, status_line);
    status_line = trim_copy(status_line);

    std::istringstream status_stream(status_line);
    std::string http_version;
    int status_code = 0;
    status_stream >> http_version >> status_code;
    if(status_code <= 0)
    {
        throw std::runtime_error("unable to parse HTTP status line: " + status_line);
    }

    return HttpResponse{status_code, body_text};
}

template <typename RequestMessage, typename ResponseMessage>
ResponseMessage call_endpoint(const std::string& host,
                              uint16_t port,
                              const std::string& path,
                              const RequestMessage& request,
                              const std::string& bearer_token = {})
{
    std::string body;
    if(!request.SerializeToString(&body))
    {
        throw std::runtime_error("SerializeToString failed for " + path);
    }

    const auto response = post_protobuf(host, port, path, body, bearer_token);
    if(response.status != 200)
    {
        throw std::runtime_error("HTTP " + std::to_string(response.status) + " for " + path);
    }

    ResponseMessage parsed;
    if(!parsed.ParseFromString(response.body))
    {
        throw std::runtime_error("ParseFromString failed for " + path);
    }
    return parsed;
}

std::string first_unlocked_codex_entry_id(const mud::CodexListResponse& response)
{
    for(const auto& entry : response.entries())
    {
        if(entry.unlocked())
        {
            return entry.entry_id();
        }
    }
    if(response.entries_size() > 0)
    {
        return response.entries(0).entry_id();
    }
    return {};
}

void require_true(bool condition, const std::string& message)
{
    if(!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_command_success(const mud::CommandExecuteResponse& response,
                             const std::string& label,
                             bool require_panel = true)
{
    require_true(response.code() == 0, label + " returned code=" + std::to_string(response.code()));
    require_true(response.result().success(), label + " result.success=false");
    if(require_panel)
    {
        require_true(response.result().panels_size() > 0, label + " returned no structured panels");
    }
}

bool panel_body_contains(const mud::StructuredPanel& panel,
                         std::string_view needle)
{
    return std::any_of(panel.body_lines().begin(),
                       panel.body_lines().end(),
                       [&](const std::string& line) { return line.find(needle) != std::string::npos; });
}

bool panel_body_contains_all(const mud::StructuredPanel& panel,
                             const std::vector<std::string>& needles)
{
    return std::all_of(needles.begin(),
                       needles.end(),
                       [&](const std::string& needle) { return panel_body_contains(panel, needle); });
}

bool string_contains(std::string_view haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string::npos;
}

bool hint_list_contains(const MudCommandExecution& execution, std::string_view needle)
{
    return std::any_of(execution.hints.begin(),
                       execution.hints.end(),
                       [&](const std::string& hint) { return string_contains(hint, needle); });
}

int local_inventory_count(const MudPlayerState& player, const std::string& item_id)
{
    int count = 0;
    for(const auto& item : player.inventory)
    {
        if(item.item_id == item_id)
        {
            count += std::max(0, item.quantity);
        }
    }
    return count;
}

void local_add_inventory_item(MudPlayerState* player, const std::string& item_id, int quantity)
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
            return;
        }
    }

    player->inventory.push_back({item_id, quantity, false});
}

void local_set_flag_int(MudPlayerState* player, const std::string& key, int value)
{
    if(player == nullptr || key.empty())
    {
        return;
    }
    player->flags[key] = std::to_string(value);
}

int local_flag_int(const MudPlayerState& player, const std::string& key, int fallback = 0)
{
    const auto found = player.flags.find(key);
    if(found == player.flags.end() || found->second.empty())
    {
        return fallback;
    }
    try
    {
        return std::stoi(found->second);
    }
    catch(const std::exception&)
    {
        return fallback;
    }
}

bool local_quest_completed(const MudPlayerState& player, const std::string& quest_id)
{
    return std::any_of(player.quests.begin(),
                       player.quests.end(),
                       [&](const MudQuestState& quest) {
                           return quest.quest_id == quest_id && quest.status == "completed";
                       });
}

class SmokeMudPlayerRepository final : public IMudPlayerRepository
{
public:
    bool ready() const override
    {
        return true;
    }

    void poll() override {}

    CoroAwaitable load_player(const std::string& /*account*/) override
    {
        return CoroAwaitable{nullptr, nullptr};
    }

    CoroAwaitable create_player(const MudPlayerState& /*player*/) override
    {
        return CoroAwaitable{nullptr, nullptr};
    }

    CoroAwaitable save_player(const MudPlayerState& /*player*/) override
    {
        return CoroAwaitable{nullptr, nullptr};
    }

    CoroAwaitable list_top_players(MudLeaderboardType /*leaderboard_type*/, int /*limit*/) override
    {
        return CoroAwaitable{nullptr, nullptr};
    }

    CoroAwaitable list_team_members(const std::string& /*team_id*/) override
    {
        return CoroAwaitable{nullptr, nullptr};
    }
};

std::shared_ptr<MudWorld> load_local_world_or_throw()
{
    auto world = std::make_shared<MudWorld>();
    std::string world_error;
    for(const auto& candidate : {std::string("doc/mud/world_data.json"),
                                 std::string("../doc/mud/world_data.json"),
                                 std::string("../../doc/mud/world_data.json"),
                                 std::string("../../../doc/mud/world_data.json")})
    {
        if(world->load_from_file(candidate, &world_error))
        {
            return world;
        }
    }

    throw std::runtime_error("load local mud world failed: " + world_error);
}

MudPlayerState make_local_story_player(MudGameRuntime* runtime,
                                       const std::string& account,
                                       int realm_stage,
                                       const std::string& realm_name)
{
    auto player = runtime->build_default_player(account, account);
    player.realm_stage = realm_stage;
    player.realm_name = realm_name;
    player.exp = player.next_breakthrough_exp;
    runtime->normalize_player_state(&player);
    return player;
}

std::string snapshot_progression_chapter(MudGameRuntime* runtime, MudPlayerState player)
{
    const auto execution = runtime->run_command(&player, "look");
    mud::CommandExecuteResponse response;
    runtime->build_command_response(player, "look", execution, &response);
    return response.player().progression_chapter();
}

} // namespace

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    try
    {
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
        const std::string suffix = std::to_string(now_ms % 100000000);
        const std::string account = "smoke" + suffix;
        const std::string password = "pass123456";
        const std::string post_subject = "烟测收药" + suffix;

        gateway::RouteLoginRequest route_request;
        route_request.set_client_version("mud_smoke_cpp");
        const auto route_response =
            call_endpoint<gateway::RouteLoginRequest, gateway::RouteLoginResponse>(
                "127.0.0.1", 18080, "/v1/route/login", route_request);

        gateway::AuthRegisterRequest register_request;
        register_request.set_account(account);
        register_request.set_password(password);
        const auto register_response =
            call_endpoint<gateway::AuthRegisterRequest, gateway::AuthRegisterResponse>(
                "127.0.0.1", 18081, "/v1/auth/register", register_request);

        gateway::AuthLoginRequest login_request;
        login_request.set_account(account);
        login_request.set_password(password);
        const auto login_response =
            call_endpoint<gateway::AuthLoginRequest, gateway::AuthLoginResponse>(
                "127.0.0.1", 18081, "/v1/auth/login", login_request);
        const std::string token = login_response.jwt();

        gateway::GameEnterRequest enter_request;
        enter_request.set_account(account);
        const auto enter_response =
            call_endpoint<gateway::GameEnterRequest, gateway::GameEnterResponse>(
                "127.0.0.1", 18082, "/v1/game/enter", enter_request, token);

        mud::BootstrapRequest bootstrap_request;
        bootstrap_request.set_account(account);
        const auto bootstrap_response =
            call_endpoint<mud::BootstrapRequest, mud::BootstrapResponse>(
                "127.0.0.1", 18082, "/v1/game/bootstrap", bootstrap_request, token);

        mud::CharacterCreateRequest create_request;
        create_request.set_account(account);
        create_request.set_character_name("烟测" + suffix.substr(suffix.size() >= 4 ? suffix.size() - 4 : 0));
        if(bootstrap_response.available_origins_size() > 0)
        {
            create_request.set_origin_id(bootstrap_response.available_origins(0).origin_id());
        }
        if(bootstrap_response.available_backgrounds_size() > 0)
        {
            create_request.set_background_id(bootstrap_response.available_backgrounds(0).background_id());
        }
        const auto create_response =
            call_endpoint<mud::CharacterCreateRequest, mud::CharacterCreateResponse>(
                "127.0.0.1", 18082, "/v1/game/character/create", create_request, token);

        mud::CommandExecuteRequest inspect_request;
        inspect_request.set_account(account);
        inspect_request.set_command("inspect 厉飞雨");
        const auto inspect_response =
            call_endpoint<mud::CommandExecuteRequest, mud::CommandExecuteResponse>(
                "127.0.0.1", 18082, "/v1/game/command/execute", inspect_request, token);

        auto run_command = [&](const std::string& command) {
            mud::CommandExecuteRequest request;
            request.set_account(account);
            request.set_command(command);
            return call_endpoint<mud::CommandExecuteRequest, mud::CommandExecuteResponse>(
                "127.0.0.1", 18082, "/v1/game/command/execute", request, token);
        };

        const auto board_response = run_command("board");
        const auto duty_response = run_command("duty");
        const auto wanted_response = run_command("wanted");
        const auto travel_response = run_command("travel");
        const auto claim_response = run_command("claim");
        const auto help_response = run_command("help newbie");
        const auto help_work_response = run_command("help work");
        const auto help_core_dan_response = run_command("help core_dan");
        const auto help_nascent_soul_response = run_command("help nascent_soul");
        const auto commands_response = run_command("commands");
        const auto work_response = run_command("work");
        const auto rank_response = run_command("rank");
        const auto rank_wealth_response = run_command("rank wealth");
        const auto rumor_response = run_command("ask 厉飞雨 about rumor");
        const auto post_response = run_command("post " + post_subject + "=七玄门外场长期收灰狼皮与止血草。");

        require_command_success(inspect_response, "inspect");
        require_command_success(board_response, "board");
        require_command_success(duty_response, "duty");
        require_command_success(wanted_response, "wanted");
        require_command_success(travel_response, "travel");
        require_true(claim_response.code() == 0, "claim returned code=" + std::to_string(claim_response.code()));
        require_command_success(help_response, "help newbie");
        require_command_success(help_work_response, "help work");
        require_command_success(help_core_dan_response, "help core_dan");
        require_command_success(help_nascent_soul_response, "help nascent_soul");
        require_command_success(commands_response, "commands");
        require_command_success(work_response, "work");
        require_command_success(rank_response, "rank");
        require_command_success(rank_wealth_response, "rank wealth");
        require_command_success(rumor_response, "ask rumor");
        require_command_success(post_response, "post", false);
        const auto& help_core_dan_panel = help_core_dan_response.result().panels(0);
        const auto& help_nascent_soul_panel = help_nascent_soul_response.result().panels(0);
        const bool mainline_outer_sea_present = panel_body_contains(help_core_dan_panel, "外海见闻") &&
                                                panel_body_contains(help_core_dan_panel, "结丹之门");
        const bool breakthrough_gold_core_hint = panel_body_contains_all(
            help_core_dan_panel,
            {"结丹灵丸", "青焰晶髓", "紫丹灵砂"});
        const bool breakthrough_nascent_soul_hint = panel_body_contains_all(
            help_nascent_soul_panel,
            {"凝婴前夜", "凝婴灵丹", "星海心珀", "养魂古玉", "世界见闻"});
        require_true(mainline_outer_sea_present, "help core_dan is missing late-game route anchors");
        require_true(breakthrough_gold_core_hint, "help core_dan is missing gold-core breakthrough checklist");
        require_true(breakthrough_nascent_soul_hint,
                     "help nascent_soul is missing nascent-soul breakthrough checklist");
        require_true(post_response.events_size() > 0, "post returned no board_post event");
        const auto board_post_event_id = post_response.events(post_response.events_size() - 1).event_id();
        const auto board_post_event_id_text = std::to_string(board_post_event_id);
        const auto read_response = run_command("read " + board_post_event_id_text);
        const auto discard_response = run_command("discard " + board_post_event_id_text);
        const auto board_after_discard_response = run_command("board");

        require_command_success(read_response, "read");
        require_true(read_response.result().panels(0).panel_kind() == "board_post",
                     "read returned unexpected panel_kind=" + read_response.result().panels(0).panel_kind());
        require_true(read_response.result().panels(0).document_id() == ("board_post:" + board_post_event_id_text),
                     "read returned unexpected document_id=" + read_response.result().panels(0).document_id());
        require_true(discard_response.code() == 0 && discard_response.result().success(),
                     "discard returned failure");
        if(board_after_discard_response.result().panels_size() > 0)
        {
            std::ostringstream board_titles_after_discard;
            for(const auto& entry : board_after_discard_response.result().panels(0).entries())
            {
                if(!board_titles_after_discard.str().empty())
                {
                    board_titles_after_discard << " | ";
                }
                board_titles_after_discard << entry.entry_id() << ":" << entry.title();
                require_true(entry.title().find(post_subject) == std::string::npos,
                             "discarded board post still visible on board: " + board_titles_after_discard.str());
            }
        }

        mud::CodexListRequest codex_list_request;
        codex_list_request.set_account(account);
        codex_list_request.set_category("人物志");
        const auto codex_list_response =
            call_endpoint<mud::CodexListRequest, mud::CodexListResponse>(
                "127.0.0.1", 18082, "/v1/game/codex/list", codex_list_request, token);

        mud::CodexDetailRequest codex_detail_request;
        codex_detail_request.set_account(account);
        codex_detail_request.set_entry_id(first_unlocked_codex_entry_id(codex_list_response));
        const auto codex_detail_response =
            call_endpoint<mud::CodexDetailRequest, mud::CodexDetailResponse>(
                "127.0.0.1", 18082, "/v1/game/codex/detail", codex_detail_request, token);
        const auto bootstrap_restore_response =
            call_endpoint<mud::BootstrapRequest, mud::BootstrapResponse>(
                "127.0.0.1", 18082, "/v1/game/bootstrap", bootstrap_request, token);

        std::cout << "restore_bootstrap_code=" << bootstrap_restore_response.code()
                  << " restore_need_create_character="
                  << (bootstrap_restore_response.need_create_character() ? "true" : "false")
                  << " restore_character=" << bootstrap_restore_response.player().character_name()
                  << "\n";

        require_true(bootstrap_restore_response.code() == 0, "restore bootstrap returned non-zero code");
        require_true(!bootstrap_restore_response.need_create_character(), "restore bootstrap unexpectedly needs character");
        require_true(!bootstrap_restore_response.player().character_name().empty(), "restore bootstrap returned empty character");

        const auto local_world = load_local_world_or_throw();
        auto local_repository = std::make_shared<SmokeMudPlayerRepository>();
        MudGameRuntime local_runtime(local_world, local_repository);
        require_true(local_runtime.ready(), "local mud runtime not ready: " + local_runtime.ready_error());

        auto gold_core_chapter_player =
            make_local_story_player(&local_runtime, "gold-core-chapter", 8, "筑基后期");
        gold_core_chapter_player.quests.push_back({"gold_core_gate", "completed", 1});
        const auto gold_core_progression_chapter =
            snapshot_progression_chapter(&local_runtime, gold_core_chapter_player);
        const bool progression_gold_core_gate_ok = gold_core_progression_chapter == "结丹之门";
        require_true(progression_gold_core_gate_ok,
                     "gold_core_gate should advance progression chapter to 结丹之门, got " +
                         gold_core_progression_chapter);

        auto core_ruin_chapter_player =
            make_local_story_player(&local_runtime, "core-ruin-chapter", 10, "结丹中期");
        core_ruin_chapter_player.quests.push_back({"core_ruin_heart", "completed", 1});
        const auto core_ruin_progression_chapter =
            snapshot_progression_chapter(&local_runtime, core_ruin_chapter_player);
        const bool progression_core_ruin_gate_ok = core_ruin_progression_chapter == "古修残环";
        require_true(progression_core_ruin_gate_ok,
                     "core_ruin_heart should advance progression chapter to 古修残环, got " +
                         core_ruin_progression_chapter);

        auto nascent_soul_chapter_player =
            make_local_story_player(&local_runtime, "nascent-soul-chapter", 11, "结丹后期");
        nascent_soul_chapter_player.quests.push_back({"nascent_soul_gate", "completed", 1});
        const auto nascent_soul_progression_chapter =
            snapshot_progression_chapter(&local_runtime, nascent_soul_chapter_player);
        const bool progression_nascent_soul_gate_ok = nascent_soul_progression_chapter == "凝婴前夜";
        require_true(progression_nascent_soul_gate_ok,
                     "nascent_soul_gate should advance progression chapter to 凝婴前夜, got " +
                         nascent_soul_progression_chapter);

        auto gold_core_breakthrough_player =
            make_local_story_player(&local_runtime, "gold-core-breakthrough", 8, "筑基后期");
        const auto direct_gold_core_breakthrough = local_runtime.run_command(&gold_core_breakthrough_player, "breakthrough");
        const bool direct_gold_core_breakthrough_ok =
            direct_gold_core_breakthrough.title == "结丹未备" &&
            string_contains(direct_gold_core_breakthrough.summary, "结丹") &&
            hint_list_contains(direct_gold_core_breakthrough, "结丹灵丸") &&
            hint_list_contains(direct_gold_core_breakthrough, "青焰晶髓");
        require_true(direct_gold_core_breakthrough_ok,
                     "direct breakthrough should expose the gold-core gate requirements");

        auto nascent_soul_breakthrough_player =
            make_local_story_player(&local_runtime, "nascent-soul-breakthrough", 11, "结丹后期");
        const auto direct_nascent_soul_breakthrough =
            local_runtime.run_command(&nascent_soul_breakthrough_player, "breakthrough");
        const bool direct_nascent_soul_breakthrough_ok =
            direct_nascent_soul_breakthrough.title == "凝婴未备" &&
            string_contains(direct_nascent_soul_breakthrough.summary, "元婴") &&
            hint_list_contains(direct_nascent_soul_breakthrough, "凝婴灵丹") &&
            hint_list_contains(direct_nascent_soul_breakthrough, "星海心珀") &&
            hint_list_contains(direct_nascent_soul_breakthrough, "世界见闻");
        require_true(direct_nascent_soul_breakthrough_ok,
                     "direct breakthrough should expose the nascent-soul gate requirements");

        auto late_route_player = make_local_story_player(&local_runtime, "late-route-replay", 8, "筑基后期");
        late_route_player.exp = 999999;
        late_route_player.next_breakthrough_exp = 0;

        late_route_player.location_scene_id = "chaos_sea_port";
        local_add_inventory_item(&late_route_player, "storm_route_chart", 1);
        const auto accept_outer_sea = local_runtime.run_command(&late_route_player, "accept outer_sea_trail");
        const auto submit_outer_sea = local_runtime.run_command(&late_route_player, "submit outer_sea_trail");
        require_true(accept_outer_sea.success && submit_outer_sea.success,
                     "late route replay could not finish outer_sea_trail: accept=" + accept_outer_sea.title +
                         "/" + accept_outer_sea.summary + ", submit=" + submit_outer_sea.title + "/" +
                         submit_outer_sea.summary + ", storm_route_chart=" +
                         std::to_string(local_inventory_count(late_route_player, "storm_route_chart")));
        require_true(local_quest_completed(late_route_player, "outer_sea_trail"),
                     "outer_sea_trail should be completed during the late route replay");
        require_true(local_inventory_count(late_route_player, "azure_flame_crystal") > 0,
                     "outer_sea_trail should award azure_flame_crystal");

        late_route_player.location_scene_id = "xutian_star_platform";
        local_add_inventory_item(&late_route_player, "xutian_tablet_rubbing", 1);
        const auto accept_xutian_star_map = local_runtime.run_command(&late_route_player, "accept xutian_star_map");
        const auto submit_xutian_star_map = local_runtime.run_command(&late_route_player, "submit xutian_star_map");
        require_true(accept_xutian_star_map.success && submit_xutian_star_map.success,
                     "late route replay could not finish xutian_star_map");
        require_true(local_quest_completed(late_route_player, "xutian_star_map"),
                     "xutian_star_map should be completed during the late route replay");
        require_true(local_inventory_count(late_route_player, "star_platform_notes") > 0,
                     "xutian_star_map should award star_platform_notes");

        late_route_player.location_scene_id = "chaos_sea_ship";
        const auto accept_gold_core_gate = local_runtime.run_command(&late_route_player, "accept gold_core_gate");
        const auto submit_gold_core_gate = local_runtime.run_command(&late_route_player, "submit gold_core_gate");
        require_true(accept_gold_core_gate.success && submit_gold_core_gate.success,
                     "late route replay could not finish gold_core_gate");
        require_true(local_quest_completed(late_route_player, "gold_core_gate"),
                     "gold_core_gate should be completed during the late route replay");
        require_true(local_inventory_count(late_route_player, "gold_core_pill") > 0,
                     "gold_core_gate should award gold_core_pill");

        late_route_player.location_scene_id = "xutian_hall";
        local_add_inventory_item(&late_route_player, "xutian_key_fragment", 1);
        const auto accept_xutian_key = local_runtime.run_command(&late_route_player, "accept xutian_key");
        const auto submit_xutian_key = local_runtime.run_command(&late_route_player, "submit xutian_key");
        require_true(accept_xutian_key.success && submit_xutian_key.success,
                     "late route replay could not finish xutian_key");
        require_true(local_quest_completed(late_route_player, "xutian_key"),
                     "xutian_key should be completed during the late route replay");

        local_add_inventory_item(&late_route_player, "treasure_cache_token", 1);
        const auto accept_core_ruin_heart = local_runtime.run_command(&late_route_player, "accept core_ruin_heart");
        const auto submit_core_ruin_heart = local_runtime.run_command(&late_route_player, "submit core_ruin_heart");
        require_true(accept_core_ruin_heart.success && submit_core_ruin_heart.success,
                     "late route replay could not finish core_ruin_heart");
        require_true(local_quest_completed(late_route_player, "core_ruin_heart"),
                     "core_ruin_heart should be completed during the late route replay");
        require_true(local_inventory_count(late_route_player, "purple_core_sand") > 0,
                     "core_ruin_heart should award purple_core_sand");
        require_true(local_flag_int(late_route_player, "major_world_witness", 0) >= 2,
                     "late route replay should accumulate the first two major world witnesses");
        require_true(local_flag_int(late_route_player, "loose_reputation", 0) >= 980,
                     "late route replay should accumulate enough loose reputation for gold-core breakthrough, got " +
                         std::to_string(local_flag_int(late_route_player, "loose_reputation", 0)) + " with sect_id=" +
                         late_route_player.sect_id);

        local_set_flag_int(&late_route_player, "gold_core_prep", 240);
        const auto gold_core_success = local_runtime.run_command(&late_route_player, "breakthrough");
        require_true(gold_core_success.success && late_route_player.realm_stage == 9,
                     "late route replay should break through to 结丹初期");

        local_add_inventory_item(&late_route_player, "gold_core_pill", 2);
        const auto stage_ten_success = local_runtime.run_command(&late_route_player, "breakthrough");
        require_true(stage_ten_success.success && late_route_player.realm_stage == 10,
                     "late route replay should advance from 结丹初期 to 结丹中期");
        const auto stage_eleven_success = local_runtime.run_command(&late_route_player, "breakthrough");
        require_true(stage_eleven_success.success && late_route_player.realm_stage == 11,
                     "late route replay should advance from 结丹中期 to 结丹后期");

        late_route_player.location_scene_id = "xutian_star_platform";
        local_add_inventory_item(&late_route_player, "void_guard_charm", 1);
        const auto accept_nascent_soul_gate =
            local_runtime.run_command(&late_route_player, "accept nascent_soul_gate");
        const auto submit_nascent_soul_gate =
            local_runtime.run_command(&late_route_player, "submit nascent_soul_gate");
        require_true(accept_nascent_soul_gate.success && submit_nascent_soul_gate.success,
                     "late route replay could not finish nascent_soul_gate");
        require_true(local_quest_completed(late_route_player, "nascent_soul_gate"),
                     "nascent_soul_gate should be completed during the late route replay");
        require_true(local_inventory_count(late_route_player, "nascent_soul_pill") > 0,
                     "nascent_soul_gate should award nascent_soul_pill");
        require_true(local_flag_int(late_route_player, "major_world_witness", 0) >= 3,
                     "late route replay should accumulate the third major world witness");

        local_add_inventory_item(&late_route_player, "star_sea_heart", 1);
        local_add_inventory_item(&late_route_player, "soul_warming_jade", 1);
        local_set_flag_int(&late_route_player, "nascent_soul_prep", 360);
        const auto nascent_soul_success = local_runtime.run_command(&late_route_player, "breakthrough");
        const bool late_route_replay_ok =
            nascent_soul_success.success && late_route_player.realm_stage == 12 &&
            string_contains(nascent_soul_success.summary, "元婴初期");
        require_true(late_route_replay_ok,
                     "late route replay should carry a 筑基后期 player through to 元婴初期");

        std::cout << "account=" << account << "\n";
        std::cout << "route_code=" << route_response.code()
                  << " login_endpoint=" << route_response.login_endpoint().host() << ":"
                  << route_response.login_endpoint().port()
                  << " game_endpoint=" << route_response.game_endpoint().host() << ":"
                  << route_response.game_endpoint().port() << "\n";
        std::cout << "register_code=" << register_response.code()
                  << " login_code=" << login_response.code()
                  << " enter_code=" << enter_response.code() << "\n";
        std::cout << "bootstrap_code=" << bootstrap_response.code()
                  << " need_create_character=" << (bootstrap_response.need_create_character() ? "true" : "false")
                  << " origins=" << bootstrap_response.available_origins_size()
                  << " backgrounds=" << bootstrap_response.available_backgrounds_size() << "\n";
        std::cout << "create_code=" << create_response.code()
                  << " scene=" << create_response.scene().scene_name()
                  << " room_layer=" << create_response.scene().room_layer()
                  << " npc_count=" << create_response.scene().npcs_size()
                  << " player_origin=" << create_response.player().race().origin_name()
                  << " player_background=" << create_response.player().background().name()
                  << " stage=" << create_response.player().stage_label()
                  << " newbie_protected=" << (create_response.player().newbie_protected() ? "true" : "false")
                  << " spells=" << create_response.player().spells_size()
                  << " recipes=" << create_response.player().recipes_size()
                  << " codex_summaries=" << create_response.player().codex_summaries_size()
                  << "\n";
        for(const auto& npc : create_response.scene().npcs())
        {
            std::cout << "scene_npc=" << npc.npc_id() << ":" << npc.name() << "\n";
        }
        std::cout << "inspect_code=" << inspect_response.code()
                  << " inspect_success=" << (inspect_response.result().success() ? "true" : "false")
                  << " inspect_title=" << inspect_response.result().title()
                  << " inspect_summary=" << inspect_response.result().summary() << "\n";
        std::cout << "board_code=" << board_response.code()
                  << " board_success=" << (board_response.result().success() ? "true" : "false")
                  << " board_panels=" << board_response.result().panels_size()
                  << " board_entries=" << (board_response.result().panels_size() > 0 ? board_response.result().panels(0).entries_size() : 0)
                  << "\n";
        std::cout << "duty_code=" << duty_response.code()
                  << " duty_success=" << (duty_response.result().success() ? "true" : "false")
                  << " duty_panels=" << duty_response.result().panels_size()
                  << "\n";
        std::cout << "wanted_code=" << wanted_response.code()
                  << " wanted_success=" << (wanted_response.result().success() ? "true" : "false")
                  << " wanted_entries=" << (wanted_response.result().panels_size() > 0 ? wanted_response.result().panels(0).entries_size() : 0)
                  << "\n";
        std::cout << "travel_code=" << travel_response.code()
                  << " travel_success=" << (travel_response.result().success() ? "true" : "false")
                  << " travel_entries=" << (travel_response.result().panels_size() > 0 ? travel_response.result().panels(0).entries_size() : 0)
                  << "\n";
        std::cout << "claim_code=" << claim_response.code()
                  << " claim_success=" << (claim_response.result().success() ? "true" : "false")
                  << " claim_entries=" << (claim_response.result().panels_size() > 0 ? claim_response.result().panels(0).entries_size() : 0)
                  << "\n";
        std::cout << "help_code=" << help_response.code()
                  << " help_success=" << (help_response.result().success() ? "true" : "false")
                  << " help_topic=" << (help_response.result().panels_size() > 0 ? help_response.result().panels(0).document_id() : "")
                  << "\n";
        std::cout << "help_work_code=" << help_work_response.code()
                  << " help_work_success=" << (help_work_response.result().success() ? "true" : "false")
                  << " help_work_topic="
                  << (help_work_response.result().panels_size() > 0 ? help_work_response.result().panels(0).document_id() : "")
                  << "\n";
        std::cout << "help_core_dan_code=" << help_core_dan_response.code()
                  << " help_core_dan_success=" << (help_core_dan_response.result().success() ? "true" : "false")
                  << " help_core_dan_topic="
                  << (help_core_dan_response.result().panels_size() > 0 ? help_core_dan_response.result().panels(0).document_id() : "")
                  << "\n";
        std::cout << "help_nascent_soul_code=" << help_nascent_soul_response.code()
                  << " help_nascent_soul_success=" << (help_nascent_soul_response.result().success() ? "true" : "false")
                  << " help_nascent_soul_topic="
                  << (help_nascent_soul_response.result().panels_size() > 0 ? help_nascent_soul_response.result().panels(0).document_id() : "")
                  << "\n";
        std::cout << "mainline_outer_sea_present=" << (mainline_outer_sea_present ? "true" : "false")
                  << " breakthrough_gold_core_hint=" << (breakthrough_gold_core_hint ? "true" : "false")
                  << " breakthrough_nascent_soul_hint=" << (breakthrough_nascent_soul_hint ? "true" : "false")
                  << "\n";
        std::cout << "progression_gold_core_gate_ok=" << (progression_gold_core_gate_ok ? "true" : "false")
                  << " progression_core_ruin_gate_ok=" << (progression_core_ruin_gate_ok ? "true" : "false")
                  << " progression_nascent_soul_gate_ok=" << (progression_nascent_soul_gate_ok ? "true" : "false")
                  << " direct_gold_core_breakthrough_ok=" << (direct_gold_core_breakthrough_ok ? "true" : "false")
                  << " direct_nascent_soul_breakthrough_ok="
                  << (direct_nascent_soul_breakthrough_ok ? "true" : "false")
                  << " late_route_replay_ok=" << (late_route_replay_ok ? "true" : "false") << "\n";
        std::cout << "commands_code=" << commands_response.code()
                  << " commands_success=" << (commands_response.result().success() ? "true" : "false")
                  << " commands_kind="
                  << (commands_response.result().panels_size() > 0 ? commands_response.result().panels(0).panel_kind() : "")
                  << "\n";
        std::cout << "work_code=" << work_response.code()
                  << " work_success=" << (work_response.result().success() ? "true" : "false")
                  << " work_entries=" << (work_response.result().panels_size() > 0 ? work_response.result().panels(0).entries_size() : 0)
                  << "\n";
        std::cout << "rank_code=" << rank_response.code()
                  << " rank_success=" << (rank_response.result().success() ? "true" : "false")
                  << " rank_lines="
                  << (rank_response.result().panels_size() > 0 ? rank_response.result().panels(0).ascii_lines_size() : 0)
                  << "\n";
        std::cout << "rank_wealth_code=" << rank_wealth_response.code()
                  << " rank_wealth_success=" << (rank_wealth_response.result().success() ? "true" : "false")
                  << " rank_wealth_lines="
                  << (rank_wealth_response.result().panels_size() > 0 ? rank_wealth_response.result().panels(0).ascii_lines_size() : 0)
                  << "\n";
        std::cout << "rumor_code=" << rumor_response.code()
                  << " rumor_success=" << (rumor_response.result().success() ? "true" : "false")
                  << " rumor_panels=" << rumor_response.result().panels_size()
                  << "\n";
        std::cout << "post_code=" << post_response.code()
                  << " post_success=" << (post_response.result().success() ? "true" : "false")
                  << " board_post_event_id=" << board_post_event_id
                  << "\n";
        std::cout << "read_code=" << read_response.code()
                  << " read_success=" << (read_response.result().success() ? "true" : "false")
                  << " read_document="
                  << (read_response.result().panels_size() > 0 ? read_response.result().panels(0).document_id() : "")
                  << "\n";
        std::cout << "discard_code=" << discard_response.code()
                  << " discard_success=" << (discard_response.result().success() ? "true" : "false")
                  << " board_after_discard_entries="
                  << (board_after_discard_response.result().panels_size() > 0 ? board_after_discard_response.result().panels(0).entries_size() : 0)
                  << "\n";
        for(const auto& unlocked : inspect_response.result().unlocked_codex_entries())
        {
            std::cout << "inspect_unlocked_codex=" << unlocked.title() << "\n";
        }
        int unlocked_count = 0;
        for(const auto& entry : codex_list_response.entries())
        {
            if(entry.unlocked())
            {
                ++unlocked_count;
            }
        }
        std::cout << "codex_list_code=" << codex_list_response.code()
                  << " entries=" << codex_list_response.entries_size()
                  << " unlocked=" << unlocked_count << "\n";
        std::cout << "codex_detail_code=" << codex_detail_response.code()
                  << " title=" << codex_detail_response.entry().title()
                  << " unlocked=" << (codex_detail_response.entry().unlocked() ? "true" : "false")
                  << " category=" << codex_detail_response.entry().category() << "\n";
        google::protobuf::ShutdownProtobufLibrary();
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << "mud_smoke failed: " << error.what() << "\n";
        google::protobuf::ShutdownProtobufLibrary();
        return 1;
    }
}

//
// Lightweight protobuf-over-HTTP smoke client for the MUD flow.
// It talks directly to the existing manager/login/game services using raw TCP
// so it can run inside WSL without extra Python or Node dependencies.
//

#include "protocol/gateway.pb.h"
#include "protocol/mud.pb.h"

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

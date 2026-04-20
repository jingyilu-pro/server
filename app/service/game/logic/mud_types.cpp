//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "mud_types.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <sstream>

int64_t mud_now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string mud_trim(const std::string& input)
{
    auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if(begin >= end)
    {
        return {};
    }
    return std::string(begin, end);
}

std::string mud_to_lower_ascii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

MudCommandParseResult parse_mud_command(const std::string& command)
{
    MudCommandParseResult parsed;
    auto trimmed = mud_trim(command);
    if(trimmed.empty())
    {
        return parsed;
    }

    std::istringstream input(trimmed);
    std::string token;
    if(!(input >> token))
    {
        return parsed;
    }

    parsed.verb = mud_to_lower_ascii(token);

    std::string arg;
    while(input >> arg)
    {
        parsed.args.push_back(arg);
    }

    const auto space_pos = trimmed.find_first_of(" \t");
    if(space_pos != std::string::npos)
    {
        parsed.raw_args = mud_trim(trimmed.substr(space_pos + 1));
    }

    return parsed;
}

MudLeaderboardType mud_parse_leaderboard_type(const std::string& value)
{
    const auto normalized = mud_to_lower_ascii(mud_trim(value));
    if(normalized == "alchemy" || normalized == "dan" || normalized == "dandao")
    {
        return MudLeaderboardType::alchemy;
    }
    if(normalized == "travel" || normalized == "explore" || normalized == "tour")
    {
        return MudLeaderboardType::travel;
    }
    if(normalized == "bounty" || normalized == "wanted")
    {
        return MudLeaderboardType::bounty;
    }
    if(normalized == "chief" || normalized == "first" || normalized == "sect")
    {
        return MudLeaderboardType::chief;
    }
    if(normalized == "wealth" || normalized == "money" || normalized == "stone")
    {
        return MudLeaderboardType::wealth;
    }
    if(normalized == "combat" || normalized == "power")
    {
        return MudLeaderboardType::combat;
    }
    return MudLeaderboardType::realm;
}

std::string mud_leaderboard_name(MudLeaderboardType type)
{
    switch(type)
    {
    case MudLeaderboardType::alchemy:
        return "alchemy";
    case MudLeaderboardType::travel:
        return "travel";
    case MudLeaderboardType::bounty:
        return "bounty";
    case MudLeaderboardType::chief:
        return "chief";
    case MudLeaderboardType::wealth:
        return "wealth";
    case MudLeaderboardType::combat:
        return "combat";
    case MudLeaderboardType::realm:
    default:
        return "realm";
    }
}

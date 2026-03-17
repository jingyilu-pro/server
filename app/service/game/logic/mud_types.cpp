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
    case MudLeaderboardType::wealth:
        return "wealth";
    case MudLeaderboardType::combat:
        return "combat";
    case MudLeaderboardType::realm:
    default:
        return "realm";
    }
}

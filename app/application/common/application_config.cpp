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

#include "application_config.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace
{

std::string trim(const std::string& input)
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

std::string unquote(std::string value)
{
    if(value.size() >= 2)
    {
        if((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))
        {
            value = value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::string strip_bom(std::string text)
{
    if(text.size() >= 3)
    {
        const unsigned char b0 = static_cast<unsigned char>(text[0]);
        const unsigned char b1 = static_cast<unsigned char>(text[1]);
        const unsigned char b2 = static_cast<unsigned char>(text[2]);
        if(b0 == 0xEF && b1 == 0xBB && b2 == 0xBF)
        {
            return text.substr(3);
        }
    }
    return text;
}

std::string to_lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool parse_bool_value(const std::string& value, bool* output)
{
    auto lowered = to_lower(trim(value));
    if(lowered == "true" || lowered == "yes" || lowered == "on" || lowered == "1")
    {
        *output = true;
        return true;
    }
    if(lowered == "false" || lowered == "no" || lowered == "off" || lowered == "0")
    {
        *output = false;
        return true;
    }
    return false;
}

bool parse_int_value(const std::string& value, int* output)
{
    try
    {
        *output = std::stoi(trim(value));
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool parse_uint16_value(const std::string& value, uint16_t* output)
{
    int parsed = 0;
    if(!parse_int_value(value, &parsed) || parsed < 0 || parsed > 65535)
    {
        return false;
    }
    *output = static_cast<uint16_t>(parsed);
    return true;
}

bool set_config_value(RuntimeConfig* config, const std::string& key, const std::string& value, std::string* error_message)
{
    if(key == "client_pressure.enabled")
    {
        bool enabled = false;
        if(!parse_bool_value(value, &enabled))
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid bool value for client_pressure.enabled";
            }
            return false;
        }
        config->client_pressure.enabled = enabled;
        return true;
    }
    if(key == "client_pressure.target.discovery_role")
    {
        config->client_pressure.target.discovery_role = trim(value);
        return true;
    }
    if(key == "client_pressure.target.manager_host")
    {
        config->client_pressure.target.manager_host = trim(value);
        return true;
    }
    if(key == "client_pressure.target.manager_port")
    {
        uint16_t port = 0;
        if(!parse_uint16_value(value, &port))
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid uint16 value for client_pressure.target.manager_port";
            }
            return false;
        }
        config->client_pressure.target.manager_port = port;
        return true;
    }

    if(key == "client_pressure.scenario.duration_sec")
    {
        return parse_int_value(value, &config->client_pressure.scenario.duration_sec);
    }
    if(key == "client_pressure.scenario.virtual_users")
    {
        return parse_int_value(value, &config->client_pressure.scenario.virtual_users);
    }
    if(key == "client_pressure.scenario.target_rps")
    {
        return parse_int_value(value, &config->client_pressure.scenario.target_rps);
    }
    if(key == "client_pressure.scenario.ramp_up_sec")
    {
        return parse_int_value(value, &config->client_pressure.scenario.ramp_up_sec);
    }
    if(key == "client_pressure.scenario.request_timeout_ms")
    {
        return parse_int_value(value, &config->client_pressure.scenario.request_timeout_ms);
    }
    if(key == "client_pressure.scenario.auto_relogin")
    {
        bool auto_relogin = true;
        if(!parse_bool_value(value, &auto_relogin))
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid bool value for client_pressure.scenario.auto_relogin";
            }
            return false;
        }
        config->client_pressure.scenario.auto_relogin = auto_relogin;
        return true;
    }

    if(key == "client_pressure.report.interval_sec")
    {
        return parse_int_value(value, &config->client_pressure.report.interval_sec);
    }
    if(key == "client_pressure.report.output")
    {
        config->client_pressure.report.output = trim(value);
        return true;
    }
    if(key == "client_pressure.report.json_path")
    {
        config->client_pressure.report.json_path = trim(value);
        return true;
    }

    if(key == "server.manager.host")
    {
        config->server.manager.host = trim(value);
        return true;
    }
    if(key == "server.manager.port")
    {
        uint16_t port = 0;
        if(!parse_uint16_value(value, &port))
        {
            return false;
        }
        config->server.manager.port = port;
        return true;
    }
    if(key == "server.login.host")
    {
        config->server.login.host = trim(value);
        return true;
    }
    if(key == "server.login.port")
    {
        uint16_t port = 0;
        if(!parse_uint16_value(value, &port))
        {
            return false;
        }
        config->server.login.port = port;
        return true;
    }
    if(key == "server.game.host")
    {
        config->server.game.host = trim(value);
        return true;
    }
    if(key == "server.game.port")
    {
        uint16_t port = 0;
        if(!parse_uint16_value(value, &port))
        {
            return false;
        }
        config->server.game.port = port;
        return true;
    }

    if(error_message != nullptr)
    {
        *error_message = "unknown config key: " + key;
    }
    return false;
}

} // namespace

bool load_runtime_config(const std::string& config_path, RuntimeConfig* config, std::string* error_message)
{
    if(config == nullptr)
    {
        if(error_message != nullptr)
        {
            *error_message = "runtime config output is null";
        }
        return false;
    }

    std::ifstream input(config_path);
    if(!input)
    {
        if(error_message != nullptr)
        {
            *error_message = "failed to open config file: " + config_path;
        }
        return false;
    }

    struct SectionState
    {
        int indent = -1;
        std::string key;
    };

    std::vector<SectionState> section_stack;
    section_stack.push_back(SectionState{-1, ""});

    int line_number = 0;
    std::string line;
    while(std::getline(input, line))
    {
        ++line_number;

        if(line_number == 1)
        {
            line = strip_bom(line);
        }

        auto hash_pos = line.find('#');
        if(hash_pos != std::string::npos)
        {
            line = line.substr(0, hash_pos);
        }

        if(trim(line).empty())
        {
            continue;
        }

        int indent = 0;
        while(indent < static_cast<int>(line.size()) && line[indent] == ' ')
        {
            ++indent;
        }

        auto stripped = trim(line.substr(indent));

        while(!section_stack.empty() && indent <= section_stack.back().indent)
        {
            section_stack.pop_back();
        }
        if(section_stack.empty())
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid indentation at line " + std::to_string(line_number);
            }
            return false;
        }

        const std::string& parent_path = section_stack.back().key;

        if(stripped.rfind("- ", 0) == 0)
        {
            if(parent_path != "client_pressure.scenario.login_account_pool")
            {
                if(error_message != nullptr)
                {
                    *error_message = "unsupported list section at line " + std::to_string(line_number);
                }
                return false;
            }
            auto list_value = unquote(trim(stripped.substr(2)));
            if(!list_value.empty())
            {
                config->client_pressure.scenario.login_account_pool.push_back(list_value);
            }
            continue;
        }

        auto colon_pos = stripped.find(':');
        if(colon_pos == std::string::npos)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid yaml line " + std::to_string(line_number);
            }
            return false;
        }

        auto key = trim(stripped.substr(0, colon_pos));
        auto value = trim(stripped.substr(colon_pos + 1));
        key = unquote(key);
        value = unquote(value);

        std::string full_key = parent_path.empty() ? key : (parent_path + "." + key);
        if(value.empty())
        {
            section_stack.push_back(SectionState{indent, full_key});
            continue;
        }

        std::string set_error;
        if(!set_config_value(config, full_key, value, &set_error))
        {
            if(error_message != nullptr)
            {
                if(set_error.empty())
                {
                    *error_message = "invalid value at line " + std::to_string(line_number) + " for key " + full_key;
                }
                else
                {
                    *error_message = "line " + std::to_string(line_number) + ": " + set_error;
                }
            }
            return false;
        }
    }

    if(config->client_pressure.scenario.virtual_users <= 0)
    {
        config->client_pressure.scenario.virtual_users = 1;
    }
    if(config->client_pressure.scenario.target_rps <= 0)
    {
        config->client_pressure.scenario.target_rps = 1;
    }
    if(config->client_pressure.scenario.duration_sec <= 0)
    {
        config->client_pressure.scenario.duration_sec = 1;
    }
    if(config->client_pressure.scenario.request_timeout_ms <= 0)
    {
        config->client_pressure.scenario.request_timeout_ms = 500;
    }
    if(config->client_pressure.report.interval_sec <= 0)
    {
        config->client_pressure.report.interval_sec = 5;
    }

    if(config->client_pressure.scenario.login_account_pool.empty())
    {
        config->client_pressure.scenario.login_account_pool = {
            "user_0001",
            "user_0002",
            "user_0003",
            "user_0004"};
    }

    return true;
}

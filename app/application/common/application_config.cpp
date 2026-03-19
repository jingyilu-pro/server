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
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <unordered_set>

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

bool is_supported_pressure_scenario(const std::string& scenario)
{
    return scenario == "full_chain" ||
           scenario == "manager_only" ||
           scenario == "login_only" ||
           scenario == "game_only";
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

bool parse_double_value(const std::string& value, double* output)
{
    try
    {
        *output = std::stod(trim(value));
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

void apply_env_override(RuntimeConfig* config)
{
    if(config == nullptr)
    {
        return;
    }

    if(const char* mysql_password = std::getenv("GAME_MYSQL_PASSWORD"); mysql_password != nullptr)
    {
        config->mysql.password = mysql_password;
    }

    if(const char* jwt_secret = std::getenv("GAME_JWT_SECRET"); jwt_secret != nullptr)
    {
        config->jwt.secret = jwt_secret;
    }

    if(const char* redis_password = std::getenv("GAME_REDIS_PASSWORD"); redis_password != nullptr)
    {
        config->redis.password = redis_password;
    }
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
    if(key == "client_pressure.scenario.scenario")
    {
        config->client_pressure.scenario.scenario = to_lower(trim(value));
        return true;
    }
    if(key == "client_pressure.scenario.warmup_sec")
    {
        return parse_int_value(value, &config->client_pressure.scenario.warmup_sec);
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
    if(key == "client_pressure.scenario.timeout_ms" || key == "client_pressure.scenario.request_timeout_ms")
    {
        return parse_int_value(value, &config->client_pressure.scenario.timeout_ms);
    }
    if(key == "client_pressure.scenario.account_pool_size")
    {
        return parse_int_value(value, &config->client_pressure.scenario.account_pool_size);
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
    if(key == "client_pressure.report.output_dir")
    {
        config->client_pressure.report.output_dir = trim(value);
        return true;
    }
    if(key == "client_pressure.report.prefix")
    {
        config->client_pressure.report.prefix = trim(value);
        return true;
    }
    if(key == "client_pressure.guard.enabled")
    {
        bool enabled = true;
        if(!parse_bool_value(value, &enabled))
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid bool value for client_pressure.guard.enabled";
            }
            return false;
        }
        config->client_pressure.guard.enabled = enabled;
        return true;
    }
    if(key == "client_pressure.guard.min_samples")
    {
        int value_int = 0;
        if(!parse_int_value(value, &value_int) || value_int < 1)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for client_pressure.guard.min_samples";
            }
            return false;
        }
        config->client_pressure.guard.min_samples = value_int;
        return true;
    }
    if(key == "client_pressure.guard.min_success_rate")
    {
        double value_double = 0.0;
        if(!parse_double_value(value, &value_double) || value_double < 0.0 || value_double > 1.0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid ratio value for client_pressure.guard.min_success_rate";
            }
            return false;
        }
        config->client_pressure.guard.min_success_rate = value_double;
        return true;
    }
    if(key == "client_pressure.guard.max_timeout_rate")
    {
        double value_double = 0.0;
        if(!parse_double_value(value, &value_double) || value_double < 0.0 || value_double > 1.0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid ratio value for client_pressure.guard.max_timeout_rate";
            }
            return false;
        }
        config->client_pressure.guard.max_timeout_rate = value_double;
        return true;
    }
    if(key == "client_pressure.guard.max_p95_ms")
    {
        double value_double = 0.0;
        if(!parse_double_value(value, &value_double) || value_double <= 0.0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive value for client_pressure.guard.max_p95_ms";
            }
            return false;
        }
        config->client_pressure.guard.max_p95_ms = value_double;
        return true;
    }
    if(key == "client_pressure.guard.max_p99_ms")
    {
        double value_double = 0.0;
        if(!parse_double_value(value, &value_double) || value_double <= 0.0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive value for client_pressure.guard.max_p99_ms";
            }
            return false;
        }
        config->client_pressure.guard.max_p99_ms = value_double;
        return true;
    }
    if(key == "client_pressure.http.coro_workers")
    {
        int workers = 0;
        if(!parse_int_value(value, &workers) || workers <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for client_pressure.http.coro_workers";
            }
            return false;
        }
        config->client_pressure.http.coro_workers = workers;
        return true;
    }

    if(key == "server.manager.host")
    {
        config->server.manager.host = trim(value);
        return true;
    }
    if(key == "server.manager.bind_host")
    {
        config->server.manager.bind_host = trim(value);
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
    if(key == "server.login.bind_host")
    {
        config->server.login.bind_host = trim(value);
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
    if(key == "server.game.bind_host")
    {
        config->server.game.bind_host = trim(value);
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

    if(key == "redis.host")
    {
        config->redis.host = trim(value);
        return true;
    }
    if(key == "redis.port")
    {
        uint16_t port = 0;
        if(!parse_uint16_value(value, &port))
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid uint16 value for redis.port";
            }
            return false;
        }
        config->redis.port = port;
        return true;
    }
    if(key == "redis.password")
    {
        config->redis.password = value;
        return true;
    }
    if(key == "redis.db")
    {
        int parsed = 0;
        if(!parse_int_value(value, &parsed) || parsed < 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid non-negative int value for redis.db";
            }
            return false;
        }
        config->redis.db = parsed;
        return true;
    }
    if(key == "redis.key_prefix")
    {
        config->redis.key_prefix = trim(value);
        return true;
    }
    if(key == "redis.ttl_sec")
    {
        int parsed = 0;
        if(!parse_int_value(value, &parsed) || parsed <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for redis.ttl_sec";
            }
            return false;
        }
        config->redis.ttl_sec = parsed;
        return true;
    }
    if(key == "redis.refresh_sec")
    {
        int parsed = 0;
        if(!parse_int_value(value, &parsed) || parsed <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for redis.refresh_sec";
            }
            return false;
        }
        config->redis.refresh_sec = parsed;
        return true;
    }
    if(key == "redis.op_timeout_ms")
    {
        int timeout_ms = 0;
        if(!parse_int_value(value, &timeout_ms) || timeout_ms <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for redis.op_timeout_ms";
            }
            return false;
        }
        config->redis.op_timeout_ms = timeout_ms;
        return true;
    }
    if(key == "redis.coro_workers")
    {
        int workers = 0;
        if(!parse_int_value(value, &workers) || workers <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for redis.coro_workers";
            }
            return false;
        }
        config->redis.coro_workers = workers;
        return true;
    }
    if(key == "redis.account_cache_ttl_sec")
    {
        int ttl_sec = 0;
        if(!parse_int_value(value, &ttl_sec) || ttl_sec <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for redis.account_cache_ttl_sec";
            }
            return false;
        }
        config->redis.account_cache_ttl_sec = ttl_sec;
        return true;
    }

    if(key == "mysql.host")
    {
        config->mysql.host = trim(value);
        return true;
    }
    if(key == "mysql.port")
    {
        uint16_t port = 0;
        if(!parse_uint16_value(value, &port))
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid uint16 value for mysql.port";
            }
            return false;
        }
        config->mysql.port = port;
        return true;
    }
    if(key == "mysql.user")
    {
        config->mysql.user = trim(value);
        return true;
    }
    if(key == "mysql.password")
    {
        config->mysql.password = value;
        return true;
    }
    if(key == "mysql.database")
    {
        config->mysql.database = trim(value);
        return true;
    }
    if(key == "mysql.connect_timeout_ms")
    {
        int timeout_ms = 0;
        if(!parse_int_value(value, &timeout_ms) || timeout_ms <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for mysql.connect_timeout_ms";
            }
            return false;
        }
        config->mysql.connect_timeout_ms = timeout_ms;
        return true;
    }
    if(key == "mysql.coro_workers")
    {
        int workers = 0;
        if(!parse_int_value(value, &workers) || workers <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for mysql.coro_workers";
            }
            return false;
        }
        config->mysql.coro_workers = workers;
        return true;
    }
    if(key == "mysql.password_hash_iterations")
    {
        int iterations = 0;
        if(!parse_int_value(value, &iterations) || iterations <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for mysql.password_hash_iterations";
            }
            return false;
        }
        config->mysql.password_hash_iterations = iterations;
        return true;
    }

    if(key == "jwt.issuer")
    {
        config->jwt.issuer = trim(value);
        return true;
    }
    if(key == "jwt.secret")
    {
        config->jwt.secret = value;
        return true;
    }
    if(key == "jwt.expire_sec")
    {
        int expire_sec = 0;
        if(!parse_int_value(value, &expire_sec) || expire_sec <= 0)
        {
            if(error_message != nullptr)
            {
                *error_message = "invalid positive int value for jwt.expire_sec";
            }
            return false;
        }
        config->jwt.expire_sec = expire_sec;
        return true;
    }

    if(key == "mud.event_ttl_sec")
    {
        return parse_int_value(value, &config->mud.event_ttl_sec);
    }
    if(key == "mud.event_index_max")
    {
        return parse_int_value(value, &config->mud.event_index_max);
    }
    if(key == "mud.bootstrap_recent_event_limit")
    {
        return parse_int_value(value, &config->mud.bootstrap_recent_event_limit);
    }
    if(key == "mud.world_event_interval_sec")
    {
        return parse_int_value(value, &config->mud.world_event_interval_sec);
    }
    if(key == "mud.chat_rate_limit_count")
    {
        return parse_int_value(value, &config->mud.chat_rate_limit_count);
    }
    if(key == "mud.chat_rate_limit_window_sec")
    {
        return parse_int_value(value, &config->mud.chat_rate_limit_window_sec);
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

    *config = RuntimeConfig{};

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
            if(parent_path != "client_pressure.scenario.login_account_pool" &&
               parent_path != "mud.gm_accounts")
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
                if(parent_path == "client_pressure.scenario.login_account_pool")
                {
                    config->client_pressure.scenario.login_account_pool.push_back(list_value);
                }
                else if(parent_path == "mud.gm_accounts")
                {
                    config->mud.gm_accounts.push_back(list_value);
                }
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
    if(config->client_pressure.scenario.timeout_ms <= 0)
    {
        config->client_pressure.scenario.timeout_ms = 500;
    }
    if(config->client_pressure.scenario.warmup_sec < 0)
    {
        config->client_pressure.scenario.warmup_sec = 0;
    }
    if(config->client_pressure.scenario.ramp_up_sec < 0)
    {
        config->client_pressure.scenario.ramp_up_sec = 0;
    }
    if(config->client_pressure.scenario.account_pool_size < 0)
    {
        config->client_pressure.scenario.account_pool_size = 0;
    }
    if(config->client_pressure.scenario.scenario.empty())
    {
        config->client_pressure.scenario.scenario = "full_chain";
    }
    if(!is_supported_pressure_scenario(config->client_pressure.scenario.scenario))
    {
        config->client_pressure.scenario.scenario = "full_chain";
    }
    if(config->client_pressure.report.interval_sec <= 0)
    {
        config->client_pressure.report.interval_sec = 5;
    }
    if(config->client_pressure.report.output_dir.empty())
    {
        config->client_pressure.report.output_dir = "reports/pressure";
    }
    if(config->client_pressure.report.prefix.empty())
    {
        config->client_pressure.report.prefix = "client_pressure";
    }
    if(config->client_pressure.report.json_path.empty())
    {
        config->client_pressure.report.json_path = config->client_pressure.report.output_dir + "/" +
                                                   config->client_pressure.report.prefix + ".json";
    }
    if(config->client_pressure.guard.min_samples <= 0)
    {
        config->client_pressure.guard.min_samples = 100;
    }
    config->client_pressure.guard.min_success_rate =
        std::clamp(config->client_pressure.guard.min_success_rate, 0.0, 1.0);
    config->client_pressure.guard.max_timeout_rate =
        std::clamp(config->client_pressure.guard.max_timeout_rate, 0.0, 1.0);
    if(config->client_pressure.guard.max_p95_ms <= 0.0)
    {
        config->client_pressure.guard.max_p95_ms = 150.0;
    }
    if(config->client_pressure.guard.max_p99_ms <= 0.0)
    {
        config->client_pressure.guard.max_p99_ms = 300.0;
    }
    if(config->redis.account_cache_ttl_sec <= 0)
    {
        config->redis.account_cache_ttl_sec = 300;
    }
    if(config->mud.event_ttl_sec <= 0)
    {
        config->mud.event_ttl_sec = 86400;
    }
    if(config->mud.event_index_max <= 0)
    {
        config->mud.event_index_max = 2000;
    }
    if(config->mud.bootstrap_recent_event_limit <= 0)
    {
        config->mud.bootstrap_recent_event_limit = 50;
    }
    if(config->mud.world_event_interval_sec <= 0)
    {
        config->mud.world_event_interval_sec = 45;
    }
    if(config->mud.chat_rate_limit_count <= 0)
    {
        config->mud.chat_rate_limit_count = 8;
    }
    if(config->mud.chat_rate_limit_window_sec <= 0)
    {
        config->mud.chat_rate_limit_window_sec = 10;
    }

    if(config->client_pressure.scenario.login_account_pool.empty())
    {
        const int account_pool_size = std::max(4, config->client_pressure.scenario.account_pool_size);
        config->client_pressure.scenario.login_account_pool.reserve(static_cast<size_t>(account_pool_size));
        for(int index = 1; index <= account_pool_size; ++index)
        {
            char account_buffer[32] = {};
            std::snprintf(account_buffer, sizeof(account_buffer), "user_%04d", index);
            config->client_pressure.scenario.login_account_pool.emplace_back(account_buffer);
        }
    }
    else if(config->client_pressure.scenario.account_pool_size <= 0)
    {
        config->client_pressure.scenario.account_pool_size = static_cast<int>(config->client_pressure.scenario.login_account_pool.size());
    }

    if(config->client_pressure.scenario.account_pool_size > static_cast<int>(config->client_pressure.scenario.login_account_pool.size()))
    {
        std::unordered_set<std::string> accounts(config->client_pressure.scenario.login_account_pool.begin(),
                                                 config->client_pressure.scenario.login_account_pool.end());
        int index = 1;
        while(static_cast<int>(config->client_pressure.scenario.login_account_pool.size()) <
              config->client_pressure.scenario.account_pool_size)
        {
            char account_buffer[32] = {};
            std::snprintf(account_buffer, sizeof(account_buffer), "user_%04d", index++);
            std::string candidate(account_buffer);
            if(accounts.insert(candidate).second)
            {
                config->client_pressure.scenario.login_account_pool.push_back(std::move(candidate));
            }
        }
    }

    apply_env_override(config);

    return true;
}

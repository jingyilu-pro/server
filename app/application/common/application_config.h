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

#include <cstdint>
#include <string>
#include <vector>

struct EndpointConfig
{
    std::string host = "127.0.0.1";
    uint16_t port = 0;
};

struct ServerConfig
{
    EndpointConfig manager{"127.0.0.1", 18080};
    EndpointConfig login{"127.0.0.1", 18081};
    EndpointConfig game{"127.0.0.1", 18082};
};

struct RedisConfig
{
    std::string host = "127.0.0.1";
    uint16_t port = 6379;
    std::string password;
    int db = 0;
    std::string key_prefix = "svc";
    int ttl_sec = 15;
    int refresh_sec = 5;
};

struct MySqlConfig
{
    std::string host = "127.0.0.1";
    uint16_t port = 3306;
    std::string user = "game_app";
    std::string password;
    std::string database = "game";
    int connect_timeout_ms = 2000;
};

struct JwtConfig
{
    std::string issuer = "game-login";
    std::string secret;
    int expire_sec = 7200;
};

struct ClientPressureTargetConfig
{
    std::string discovery_role = "manager";
    std::string manager_host;
    uint16_t manager_port = 0;
};

struct ClientPressureScenarioConfig
{
    int duration_sec = 30;
    int virtual_users = 20;
    int target_rps = 120;
    int ramp_up_sec = 5;
    int request_timeout_ms = 2000;
    std::vector<std::string> login_account_pool;
    bool auto_relogin = true;
};

struct ClientPressureReportConfig
{
    int interval_sec = 5;
    std::string output = "log";
    std::string json_path = "client_pressure_report.json";
};

struct ClientPressureConfig
{
    bool enabled = false;
    ClientPressureTargetConfig target;
    ClientPressureScenarioConfig scenario;
    ClientPressureReportConfig report;
};

struct RuntimeConfig
{
    ServerConfig server;
    RedisConfig redis;
    MySqlConfig mysql;
    JwtConfig jwt;
    ClientPressureConfig client_pressure;
};

bool load_runtime_config(const std::string& config_path, RuntimeConfig* config, std::string* error_message = nullptr);

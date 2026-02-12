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

#include "application_config.h"

#include <chrono>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

enum class StageType
{
    manager,
    login,
    game
};

const char* stage_name(StageType stage);

struct StageSample
{
    StageType stage = StageType::manager;
    bool success = false;
    int status_code = 0;
    int64_t latency_us = 0;
    bool timeout = false;
    std::string error_reason;
};

struct WorkerCycleResult
{
    bool success = false;
    bool timeout = false;
    int64_t chain_latency_us = 0;
    std::vector<StageSample> stages;
    std::string failure_reason;
};

struct StageMetrics
{
    uint64_t request_total = 0;
    uint64_t success_total = 0;
    uint64_t timeout_total = 0;
    std::unordered_map<std::string, uint64_t> failure_reason_count;
    std::unordered_map<int, uint64_t> status_code_count;
    std::vector<int64_t> latency_us;
};

struct PressureMetricsSnapshot
{
    uint64_t total_request = 0;
    uint64_t total_success = 0;
    uint64_t total_timeout = 0;
    std::vector<int64_t> chain_latency_us;
    std::map<StageType, StageMetrics> stage_metrics;
    std::unordered_map<std::string, uint64_t> failure_reason_count;
    std::chrono::steady_clock::time_point begin_time;
    std::chrono::steady_clock::time_point end_time;
};

struct ClientPressureTask
{
    int worker_id = 0;
    std::string scenario = "full_chain";
    std::string account;
    EndpointConfig manager_endpoint;
    EndpointConfig login_endpoint;
    EndpointConfig game_endpoint;
    int request_timeout_ms = 2000;
    bool auto_relogin = true;
    std::string jwt_token;
    bool has_token = false;
};

//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
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

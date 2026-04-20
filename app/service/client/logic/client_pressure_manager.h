//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "client_pressure_types.h"
#include "client_worker.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class ClientPressureManager
{
public:
    explicit ClientPressureManager(const RuntimeConfig& config);
    ~ClientPressureManager();

    bool start();
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time);
    void stop();
    bool completed() const;

private:
    struct WorkerSlot
    {
        ClientPressureTask task;
        std::thread thread;
        std::condition_variable cv;
        std::mutex mutex;
        bool stopping = false;
        bool running = false;
        uint64_t pending = 0;
        uint64_t launch_count = 0;
    };

    bool should_dispatch() const;
    bool should_stop_by_timeout_guard();
    bool dispatch_one(int worker_index);
    void worker_loop(int worker_index);
    void on_cycle_result(const WorkerCycleResult& result);
    void report_if_due(bool force = false);
    void write_json_report() const;

    static int64_t percentile_us(const std::vector<int64_t>& values, double percentile);
    static double safe_ratio(uint64_t numerator, uint64_t denominator);

private:
    RuntimeConfig m_config;
    std::vector<std::unique_ptr<WorkerSlot>> m_workers;

    mutable std::mutex m_metrics_mutex;
    mutable std::mutex m_warmup_metrics_mutex;
    PressureMetricsSnapshot m_metrics;
    PressureMetricsSnapshot m_warmup_metrics;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop_requested{false};
    std::atomic<bool> m_finished{false};
    std::atomic<bool> m_timeout_guard_triggered{false};
    std::string m_early_stop_reason;

    std::chrono::steady_clock::time_point m_start_time;
    std::chrono::steady_clock::time_point m_warmup_end_time;
    std::chrono::steady_clock::time_point m_end_time;
    std::chrono::steady_clock::time_point m_last_dispatch_time;
    std::chrono::steady_clock::time_point m_last_report_time;

    std::atomic<double> m_token_bucket{0.0};
    double m_bucket_capacity = 1.0;
    std::atomic<uint64_t> m_dispatch_round_robin{0};
    std::atomic<int> m_inflight_count{0};

    std::chrono::steady_clock::time_point m_last_guard_eval_time{};
    uint64_t m_last_guard_eval_samples = 0;
    double m_last_guard_p95_ms = 0.0;
    double m_last_guard_p99_ms = 0.0;
    bool m_guard_latency_cache_ready = false;
};

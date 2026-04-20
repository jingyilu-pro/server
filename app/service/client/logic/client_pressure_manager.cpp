//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "client_pressure_manager.h"

#include "log/glogger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace
{

std::string first_failure(const WorkerCycleResult& result)
{
    for(const auto& stage : result.stages)
    {
        if(!stage.success)
        {
            if(!stage.error_reason.empty())
            {
                return stage.error_reason;
            }
            return std::string(stage_name(stage.stage)) + "_failed";
        }
    }
    return result.failure_reason.empty() ? "unknown_failure" : result.failure_reason;
}

std::string json_escape(const std::string& text)
{
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for(char ch : text)
    {
        switch(ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::string format_reason_distribution(const std::unordered_map<std::string, uint64_t>& reason_map)
{
    if(reason_map.empty())
    {
        return "{}";
    }

    std::vector<std::pair<std::string, uint64_t>> items(reason_map.begin(), reason_map.end());
    std::sort(items.begin(), items.end(), [](const auto& lhs, const auto& rhs) {
        if(lhs.second == rhs.second)
        {
            return lhs.first < rhs.first;
        }
        return lhs.second > rhs.second;
    });

    std::ostringstream oss;
    oss << "{";
    for(size_t i = 0; i < items.size(); ++i)
    {
        if(i > 0)
        {
            oss << ", ";
        }
        oss << items[i].first << ":" << items[i].second;
    }
    oss << "}";
    return oss.str();
}

std::string format_status_distribution(const std::unordered_map<int, uint64_t>& status_map)
{
    if(status_map.empty())
    {
        return "{}";
    }

    std::vector<std::pair<int, uint64_t>> items(status_map.begin(), status_map.end());
    std::sort(items.begin(), items.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    std::ostringstream oss;
    oss << "{";
    for(size_t i = 0; i < items.size(); ++i)
    {
        if(i > 0)
        {
            oss << ", ";
        }
        oss << items[i].first << ":" << items[i].second;
    }
    oss << "}";
    return oss.str();
}

} // namespace

ClientPressureManager::ClientPressureManager(const RuntimeConfig& config)
    : m_config(config)
{
}

ClientPressureManager::~ClientPressureManager()
{
    stop();
}

bool ClientPressureManager::start()
{
    if(m_running.load())
    {
        return true;
    }

    const auto& scenario = m_config.client_pressure.scenario;

    m_workers.clear();
    m_workers.reserve(static_cast<size_t>(scenario.virtual_users));

    for(int i = 0; i < scenario.virtual_users; ++i)
    {
        auto slot = std::make_unique<WorkerSlot>();
        slot->task.worker_id = i;
        slot->task.scenario = scenario.scenario;
        slot->task.account = scenario.login_account_pool[static_cast<size_t>(i) % scenario.login_account_pool.size()];
        slot->task.manager_endpoint = m_config.server.manager;
        slot->task.login_endpoint = m_config.server.login;
        slot->task.game_endpoint = m_config.server.game;
        slot->task.request_timeout_ms = scenario.timeout_ms;
        slot->task.auto_relogin = scenario.auto_relogin;

        if(!m_config.client_pressure.target.manager_host.empty())
        {
            slot->task.manager_endpoint.host = m_config.client_pressure.target.manager_host;
        }
        if(m_config.client_pressure.target.manager_port > 0)
        {
            slot->task.manager_endpoint.port = m_config.client_pressure.target.manager_port;
        }
        m_workers.push_back(std::move(slot));
    }

    for(size_t i = 0; i < m_workers.size(); ++i)
    {
        auto& slot = m_workers[i];
        if(slot)
        {
            slot->thread = std::thread(&ClientPressureManager::worker_loop, this, static_cast<int>(i));
        }
    }

    auto now = std::chrono::steady_clock::now();
    m_start_time = now;
    m_warmup_end_time = m_start_time + std::chrono::seconds(std::max(0, scenario.warmup_sec));
    m_end_time = m_warmup_end_time + std::chrono::seconds(std::max(1, scenario.duration_sec));
    m_last_dispatch_time = now;
    m_last_report_time = now;

    m_warmup_metrics = PressureMetricsSnapshot{};
    m_warmup_metrics.begin_time = m_start_time;
    m_warmup_metrics.end_time = m_start_time;

    m_metrics = PressureMetricsSnapshot{};
    m_metrics.begin_time = m_warmup_end_time;
    m_metrics.end_time = m_warmup_end_time;

    m_bucket_capacity = static_cast<double>(std::max(1, scenario.target_rps));
    m_token_bucket.store(m_bucket_capacity);

    m_inflight_count.store(0);
    m_stop_requested.store(false);
    m_finished.store(false);
    m_timeout_guard_triggered.store(false);
    m_early_stop_reason.clear();
    m_last_guard_eval_time = now;
    m_last_guard_eval_samples = 0;
    m_last_guard_p95_ms = 0.0;
    m_last_guard_p99_ms = 0.0;
    m_guard_latency_cache_ready = false;
    m_running.store(true);

    spdlog::info("client pressure manager start: scenario={}, vus={}, rps={}, warmup={}s, duration={}s, ramp={}s",
                 scenario.scenario,
                 scenario.virtual_users,
                 scenario.target_rps,
                 scenario.warmup_sec,
                 scenario.duration_sec,
                 scenario.ramp_up_sec);
    return true;
}

void ClientPressureManager::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    (void)delta_time;
    (void)last_tick_time;

    if(!m_running.load() || m_finished.load())
    {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    const auto elapsed = now - m_start_time;
    if(should_stop_by_timeout_guard())
    {
        spdlog::warn("[client-pressure] sla guard triggered reason={}, stop pressure early", m_early_stop_reason);
        stop();
        return;
    }

    if(now >= m_end_time)
    {
        stop();
        return;
    }

    const auto seconds_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    const auto delta_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(now - m_last_dispatch_time).count();
    m_last_dispatch_time = now;

    const auto target_rps = std::max(1, m_config.client_pressure.scenario.target_rps);
    double ramp_multiplier = 1.0;
    const auto ramp_up_sec = std::max(0, m_config.client_pressure.scenario.ramp_up_sec);
    if(ramp_up_sec > 0)
    {
        ramp_multiplier = std::min(1.0, seconds_elapsed / static_cast<double>(ramp_up_sec));
        ramp_multiplier = std::max(ramp_multiplier, 0.05);
    }

    const double produced = static_cast<double>(target_rps) * ramp_multiplier * delta_seconds;
    double bucket = m_token_bucket.load();
    bucket = std::min(m_bucket_capacity, bucket + produced);
    m_token_bucket.store(bucket);

    int launched = 0;
    const int worker_count = std::max(1, static_cast<int>(m_workers.size()));
    int dispatch_limit = worker_count;
    while(m_token_bucket.load() >= 1.0 && launched < dispatch_limit)
    {
        bool dispatched = false;
        for(int attempt = 0; attempt < worker_count; ++attempt)
        {
            const auto next_index = static_cast<int>(m_dispatch_round_robin.fetch_add(1) % m_workers.size());
            if(dispatch_one(next_index))
            {
                m_token_bucket.store(m_token_bucket.load() - 1.0);
                ++launched;
                dispatched = true;
                break;
            }
        }

        if(!dispatched)
        {
            break;
        }
    }

    report_if_due(false);
}

void ClientPressureManager::stop()
{
    if(!m_running.load() && m_workers.empty())
    {
        return;
    }

    m_stop_requested.store(true);

    for(auto& slot : m_workers)
    {
        if(slot)
        {
            {
                std::lock_guard guard(slot->mutex);
                slot->stopping = true;
                slot->pending = 0;
            }
            slot->cv.notify_all();
        }
    }

    for(auto& slot : m_workers)
    {
        if(slot && slot->thread.joinable())
        {
            slot->thread.join();
        }
    }
    m_workers.clear();

    m_running.store(false);
    m_finished.store(true);
    {
        std::lock_guard lock(m_warmup_metrics_mutex);
        m_warmup_metrics.end_time = std::chrono::steady_clock::now();
    }
    {
        std::lock_guard lock(m_metrics_mutex);
        m_metrics.end_time = std::chrono::steady_clock::now();
    }
    report_if_due(true);
}

bool ClientPressureManager::completed() const
{
    return m_finished.load();
}

bool ClientPressureManager::should_dispatch() const
{
    return m_running.load() && !m_stop_requested.load() && std::chrono::steady_clock::now() < m_end_time;
}

bool ClientPressureManager::should_stop_by_timeout_guard()
{
    if(m_timeout_guard_triggered.load())
    {
        return true;
    }

    uint64_t total_request = 0;
    uint64_t total_success = 0;
    uint64_t total_timeout = 0;
    {
        std::lock_guard lock(m_metrics_mutex);
        total_request = m_metrics.total_request;
        total_success = m_metrics.total_success;
        total_timeout = m_metrics.total_timeout;
    }

    const uint64_t configured_min_samples = static_cast<uint64_t>(std::max(1, m_config.client_pressure.guard.min_samples));
    const uint64_t adaptive_min_samples = static_cast<uint64_t>(
        std::max<int>(
            static_cast<int>(configured_min_samples),
            std::max(1, m_config.client_pressure.scenario.target_rps) * 5));
    if(total_request < adaptive_min_samples)
    {
        return false;
    }

    if(!m_config.client_pressure.guard.enabled)
    {
        return false;
    }

    const double success_rate = safe_ratio(total_success, total_request);
    const double timeout_rate = safe_ratio(total_timeout, total_request);

    const auto now = std::chrono::steady_clock::now();
    const bool enough_new_samples = total_request >= (m_last_guard_eval_samples + 128);
    const bool reached_eval_interval = now - m_last_guard_eval_time >= std::chrono::seconds(1);
    if(!m_guard_latency_cache_ready || enough_new_samples || reached_eval_interval)
    {
        std::vector<int64_t> latencies_us;
        {
            std::lock_guard lock(m_metrics_mutex);
            latencies_us = m_metrics.chain_latency_us;
            m_last_guard_eval_samples = m_metrics.total_request;
        }
        m_last_guard_p95_ms = static_cast<double>(percentile_us(latencies_us, 0.95)) / 1000.0;
        m_last_guard_p99_ms = static_cast<double>(percentile_us(latencies_us, 0.99)) / 1000.0;
        m_last_guard_eval_time = now;
        m_guard_latency_cache_ready = true;
    }

    const double p95_ms = m_last_guard_p95_ms;
    const double p99_ms = m_last_guard_p99_ms;

    if(success_rate < m_config.client_pressure.guard.min_success_rate)
    {
        m_early_stop_reason = "success_rate_below_threshold";
        m_timeout_guard_triggered.store(true);
        return true;
    }
    if(timeout_rate > m_config.client_pressure.guard.max_timeout_rate)
    {
        m_early_stop_reason = "timeout_rate_above_threshold";
        m_timeout_guard_triggered.store(true);
        return true;
    }
    if(p95_ms > m_config.client_pressure.guard.max_p95_ms)
    {
        m_early_stop_reason = "p95_above_threshold";
        m_timeout_guard_triggered.store(true);
        return true;
    }
    if(p99_ms > m_config.client_pressure.guard.max_p99_ms)
    {
        m_early_stop_reason = "p99_above_threshold";
        m_timeout_guard_triggered.store(true);
        return true;
    }

    return false;
}

bool ClientPressureManager::dispatch_one(int worker_index)
{
    if(worker_index < 0 || worker_index >= static_cast<int>(m_workers.size()))
    {
        return false;
    }
    auto& slot = m_workers[static_cast<size_t>(worker_index)];
    if(!slot)
    {
        return false;
    }

    {
        std::lock_guard guard(slot->mutex);
        if(slot->stopping)
        {
            return false;
        }

        if(slot->running || slot->pending > 0)
        {
            return false;
        }

        slot->pending += 1;
        slot->launch_count += 1;
    }
    slot->cv.notify_one();
    return true;
}

void ClientPressureManager::worker_loop(int worker_index)
{
    if(worker_index < 0 || worker_index >= static_cast<int>(m_workers.size()))
    {
        return;
    }

    auto& slot = m_workers[static_cast<size_t>(worker_index)];
    if(!slot)
    {
        return;
    }

    ClientWorker worker(std::max(1, m_config.client_pressure.http.coro_workers));
    while(true)
    {
        {
            std::unique_lock lock(slot->mutex);
            slot->cv.wait(lock, [&]() {
                return slot->stopping || slot->pending > 0;
            });

            if(slot->stopping)
            {
                break;
            }

            if(slot->pending == 0)
            {
                continue;
            }

            slot->pending -= 1;
            slot->running = true;
        }

        m_inflight_count.fetch_add(1);
        auto result = worker.run(&slot->task);
        on_cycle_result(result);
        m_inflight_count.fetch_sub(1);

        {
            std::lock_guard guard(slot->mutex);
            slot->running = false;
        }
    }
}

void ClientPressureManager::on_cycle_result(const WorkerCycleResult& result)
{
    const auto now = std::chrono::steady_clock::now();
    const bool in_warmup = now < m_warmup_end_time;

    auto* metrics_mutex = in_warmup ? &m_warmup_metrics_mutex : &m_metrics_mutex;
    auto* metrics = in_warmup ? &m_warmup_metrics : &m_metrics;

    std::lock_guard lock(*metrics_mutex);
    metrics->total_request += 1;
    metrics->end_time = now;
    metrics->chain_latency_us.push_back(result.chain_latency_us);

    if(result.success)
    {
        metrics->total_success += 1;
    }
    if(result.timeout)
    {
        metrics->total_timeout += 1;
    }
    if(!result.success)
    {
        metrics->failure_reason_count[first_failure(result)] += 1;
    }

    for(const auto& stage : result.stages)
    {
        auto& stage_metrics = metrics->stage_metrics[stage.stage];
        stage_metrics.request_total += 1;
        if(stage.success)
        {
            stage_metrics.success_total += 1;
        }
        if(stage.timeout)
        {
            stage_metrics.timeout_total += 1;
        }
        stage_metrics.latency_us.push_back(stage.latency_us);
        stage_metrics.status_code_count[stage.status_code] += 1;
        if(!stage.success)
        {
            auto reason = stage.error_reason.empty() ? "unknown_stage_error" : stage.error_reason;
            stage_metrics.failure_reason_count[reason] += 1;
        }
    }
}

void ClientPressureManager::report_if_due(bool force)
{
    auto now = std::chrono::steady_clock::now();
    if(!force)
    {
        if(now - m_last_report_time < std::chrono::seconds(std::max(1, m_config.client_pressure.report.interval_sec)))
        {
            return;
        }
    }

    PressureMetricsSnapshot snapshot;
    PressureMetricsSnapshot warmup_snapshot;
    {
        std::lock_guard lock(m_warmup_metrics_mutex);
        warmup_snapshot = m_warmup_metrics;
    }
    {
        std::lock_guard lock(m_metrics_mutex);
        snapshot = m_metrics;
    }

    const auto elapsed_seconds = std::max(0.001,
                                          std::chrono::duration_cast<std::chrono::duration<double>>(snapshot.end_time - snapshot.begin_time).count());
    const auto qps = static_cast<double>(snapshot.total_request) / elapsed_seconds;
    const auto success_rate = safe_ratio(snapshot.total_success, snapshot.total_request) * 100.0;
    const auto timeout_rate = safe_ratio(snapshot.total_timeout, snapshot.total_request) * 100.0;
    const auto p50 = percentile_us(snapshot.chain_latency_us, 0.50) / 1000.0;
    const auto p95 = percentile_us(snapshot.chain_latency_us, 0.95) / 1000.0;
    const auto p99 = percentile_us(snapshot.chain_latency_us, 0.99) / 1000.0;

    spdlog::info("[client-pressure] total={}, success={}, success_rate={:.2f}%, timeout_rate={:.2f}%, qps={:.2f}, p50={:.2f}ms, p95={:.2f}ms, p99={:.2f}ms, inflight={}",
                 snapshot.total_request,
                 snapshot.total_success,
                 success_rate,
                 timeout_rate,
                 qps,
                 p50,
                 p95,
                 p99,
                 m_inflight_count.load());
    if(m_config.client_pressure.scenario.warmup_sec > 0)
    {
        spdlog::info("[client-pressure] warmup_total={}, warmup_success={}, warmup_timeout={}",
                     warmup_snapshot.total_request,
                     warmup_snapshot.total_success,
                     warmup_snapshot.total_timeout);
    }
    if(!snapshot.failure_reason_count.empty())
    {
        spdlog::info("[client-pressure] failure_reason_distribution={}", format_reason_distribution(snapshot.failure_reason_count));
    }

    for(const auto& [stage, metrics] : snapshot.stage_metrics)
    {
        const auto stage_success_rate = safe_ratio(metrics.success_total, metrics.request_total) * 100.0;
        const auto stage_timeout_rate = safe_ratio(metrics.timeout_total, metrics.request_total) * 100.0;
        const auto stage_p95 = percentile_us(metrics.latency_us, 0.95) / 1000.0;
        spdlog::info("[client-pressure][stage={}] total={}, success_rate={:.2f}%, timeout_rate={:.2f}%, p95={:.2f}ms",
                     stage_name(stage),
                     metrics.request_total,
                     stage_success_rate,
                     stage_timeout_rate,
                     stage_p95);
        if(!metrics.failure_reason_count.empty())
        {
            spdlog::info("[client-pressure][stage={}] failure_reason_distribution={}",
                         stage_name(stage),
                         format_reason_distribution(metrics.failure_reason_count));
        }
        if(!metrics.status_code_count.empty())
        {
            spdlog::info("[client-pressure][stage={}] status_code_distribution={}",
                         stage_name(stage),
                         format_status_distribution(metrics.status_code_count));
        }
    }

    m_last_report_time = now;

    if(force && m_config.client_pressure.report.output == "json")
    {
        write_json_report();
    }
}

void ClientPressureManager::write_json_report() const
{
    PressureMetricsSnapshot warmup_snapshot;
    {
        std::lock_guard lock(m_warmup_metrics_mutex);
        warmup_snapshot = m_warmup_metrics;
    }

    PressureMetricsSnapshot snapshot;
    {
        std::lock_guard lock(m_metrics_mutex);
        snapshot = m_metrics;
    }

    const auto elapsed_seconds = std::max(0.001,
                                          std::chrono::duration_cast<std::chrono::duration<double>>(snapshot.end_time - snapshot.begin_time).count());
    const auto qps = static_cast<double>(snapshot.total_request) / elapsed_seconds;

    const std::filesystem::path json_path(m_config.client_pressure.report.json_path);
    if(json_path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(json_path.parent_path(), ec);
        if(ec)
        {
            spdlog::error("failed to create json report directory '{}': {}", json_path.parent_path().string(), ec.message());
            return;
        }
    }

    std::ofstream output(json_path, std::ios::trunc);
    if(!output)
    {
        spdlog::error("failed to open json report path: {}", m_config.client_pressure.report.json_path);
        return;
    }

    output << "{\n";
    output << "  \"qps\": " << std::fixed << std::setprecision(3) << qps << ",\n";
    output << "  \"success_rate\": " << std::fixed << std::setprecision(6) << safe_ratio(snapshot.total_success, snapshot.total_request) << ",\n";
    output << "  \"timeout_rate\": " << std::fixed << std::setprecision(6) << safe_ratio(snapshot.total_timeout, snapshot.total_request) << ",\n";
    output << "  \"p50\": " << percentile_us(snapshot.chain_latency_us, 0.50) << ",\n";
    output << "  \"p95\": " << percentile_us(snapshot.chain_latency_us, 0.95) << ",\n";
    output << "  \"p99\": " << percentile_us(snapshot.chain_latency_us, 0.99) << ",\n";
    const bool early_stopped = m_timeout_guard_triggered.load();
    const bool early_stopped_by_timeout = early_stopped && m_early_stop_reason == "timeout_rate_above_threshold";
    output << "  \"early_stopped_by_timeout_guard\": "
           << (early_stopped_by_timeout ? "true" : "false")
           << ",\n";
    output << "  \"early_stopped_by_sla_guard\": "
           << (early_stopped ? "true" : "false")
           << ",\n";
    output << "  \"early_stop_reason\": \"" << json_escape(m_early_stop_reason) << "\",\n";
    output << "  \"warmup\": {\n";
    output << "    \"duration_sec\": " << std::max(0, m_config.client_pressure.scenario.warmup_sec) << ",\n";
    output << "    \"total\": " << warmup_snapshot.total_request << ",\n";
    output << "    \"success\": " << warmup_snapshot.total_success << ",\n";
    output << "    \"timeout\": " << warmup_snapshot.total_timeout << "\n";
    output << "  },\n";

    output << "  \"failure_reasons\": {\n";
    bool first_reason = true;
    for(const auto& [reason, count] : snapshot.failure_reason_count)
    {
        if(!first_reason)
        {
            output << ",\n";
        }
        output << "    \"" << json_escape(reason) << "\": " << count;
        first_reason = false;
    }
    output << "\n  },\n";

    output << "  \"stage_breakdown\": {\n";
    bool first_stage = true;
    for(const auto& [stage, metrics] : snapshot.stage_metrics)
    {
        if(!first_stage)
        {
            output << ",\n";
        }
        output << "    \"" << stage_name(stage) << "\": {\n";
        output << "      \"request_total\": " << metrics.request_total << ",\n";
        output << "      \"success_total\": " << metrics.success_total << ",\n";
        output << "      \"timeout_total\": " << metrics.timeout_total << ",\n";
        output << "      \"success_rate\": " << std::fixed << std::setprecision(6)
               << safe_ratio(metrics.success_total, metrics.request_total) << ",\n";
        output << "      \"timeout_rate\": " << std::fixed << std::setprecision(6)
               << safe_ratio(metrics.timeout_total, metrics.request_total) << ",\n";
        output << "      \"p95\": " << percentile_us(metrics.latency_us, 0.95) << ",\n";

        output << "      \"failure_reasons\": {";
        bool first_stage_reason = true;
        for(const auto& [reason, count] : metrics.failure_reason_count)
        {
            if(!first_stage_reason)
            {
                output << ", ";
            }
            output << "\"" << json_escape(reason) << "\": " << count;
            first_stage_reason = false;
        }
        output << "},\n";

        output << "      \"status_code_distribution\": {";
        bool first_status = true;
        for(const auto& [status_code, count] : metrics.status_code_count)
        {
            if(!first_status)
            {
                output << ", ";
            }
            output << "\"" << status_code << "\": " << count;
            first_status = false;
        }
        output << "}\n";

        output << "    }";
        first_stage = false;
    }
    output << "\n  }\n";
    output << "}\n";

    spdlog::info("client pressure json report generated: {}", m_config.client_pressure.report.json_path);
}

int64_t ClientPressureManager::percentile_us(const std::vector<int64_t>& values, double percentile)
{
    if(values.empty())
    {
        return 0;
    }
    auto copy = values;
    std::sort(copy.begin(), copy.end());
    percentile = std::clamp(percentile, 0.0, 1.0);
    const auto index = static_cast<size_t>(std::round((copy.size() - 1) * percentile));
    return copy[index];
}

double ClientPressureManager::safe_ratio(uint64_t numerator, uint64_t denominator)
{
    if(denominator == 0)
    {
        return 0.0;
    }
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

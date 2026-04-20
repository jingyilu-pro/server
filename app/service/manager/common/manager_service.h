//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "basic_http_service.h"
#include "corocoroutine.h"

#include "service_discovery.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

class ManagerService : public BasicHttpService
{
public:
    ManagerService(const RuntimeConfig& config, std::shared_ptr<IServiceDiscovery> discovery);
    ~ManagerService() override;

public:
    bool start() override;
    bool stop() override;
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time) override;

protected:
    void on_event_loop_tick() override;

private:
    EndpointConfig choose_weighted_endpoint(const std::vector<ServiceInstance>& instances,
                                            const EndpointConfig& fallback,
                                            std::size_t* round_robin_counter);
    coro_task_t register_instance_async();
    coro_task_t unregister_instance_async();
    coro_task_t heartbeat_async();
    coro_task_t route_login_async(evhttp_request* request);

private:
    RuntimeConfig m_config;
    std::shared_ptr<IServiceDiscovery> m_discovery;
    ServiceInstance m_local_instance;
    std::chrono::steady_clock::time_point m_last_heartbeat;
    std::size_t m_login_round_robin_counter = 0;
    std::size_t m_game_round_robin_counter = 0;
    bool m_heartbeat_inflight = false;
    std::atomic<bool> m_register_inflight{false};
    std::atomic<bool> m_registered{false};
    std::atomic<bool> m_stopping{false};

    std::mutex m_lifecycle_mutex;
    std::condition_variable m_lifecycle_cv;
    bool m_start_register_waiting = false;
    bool m_start_register_done = false;
    bool m_start_register_success = false;
    std::string m_start_register_error;
    bool m_stop_unregister_waiting = false;
    bool m_stop_unregister_requested = false;
    bool m_stop_unregister_done = false;
    bool m_stop_unregister_success = false;
    std::string m_stop_unregister_error;
};

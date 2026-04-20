//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "account_cache_store.h"
#include "basic_http_service.h"
#include "corocoroutine.h"

#include "account_repository.h"
#include "session_store.h"
#include "service_discovery.h"
#include "token_provider.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

class LoginService : public BasicHttpService
{
public:
    LoginService(const RuntimeConfig& config,
                 std::shared_ptr<IServiceDiscovery> discovery,
                 std::shared_ptr<IAccountRepository> account_repository,
                 std::shared_ptr<IAccountCacheStore> account_cache_store,
                 std::shared_ptr<ITokenProvider> token_provider,
                 std::shared_ptr<ISessionStore> session_store);
    ~LoginService() override;

public:
    bool start() override;
    bool stop() override;
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time) override;

protected:
    void on_event_loop_tick() override;

private:
    EndpointConfig choose_weighted_game_endpoint(const std::vector<ServiceInstance>& instances);
    coro_task_t register_instance_async();
    coro_task_t unregister_instance_async();
    coro_task_t heartbeat_async();
    coro_task_t refresh_game_instances_async();
    coro_task_t register_async(evhttp_request* request);
    coro_task_t login_async(evhttp_request* request);

private:
    RuntimeConfig m_config;
    std::shared_ptr<IServiceDiscovery> m_discovery;
    std::shared_ptr<IAccountRepository> m_account_repository;
    std::shared_ptr<IAccountCacheStore> m_account_cache_store;
    std::shared_ptr<ITokenProvider> m_token_provider;
    std::shared_ptr<ISessionStore> m_session_store;
    ServiceInstance m_local_instance;
    std::chrono::steady_clock::time_point m_last_heartbeat;
    std::size_t m_game_round_robin_counter = 0;
    bool m_heartbeat_inflight = false;
    bool m_game_instances_refresh_inflight = false;
    std::chrono::steady_clock::time_point m_last_game_instances_refresh;
    std::vector<ServiceInstance> m_cached_game_instances;
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

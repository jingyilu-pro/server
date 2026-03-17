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
#include "basic_http_service.h"
#include "corocoroutine.h"
#include "mud_event_store.h"
#include "mud_game_runtime.h"

#include "session_store.h"
#include "service_discovery.h"
#include "token_provider.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

class GameService : public BasicHttpService
{
public:
    GameService(const RuntimeConfig& config,
                std::shared_ptr<IServiceDiscovery> discovery,
                std::shared_ptr<ITokenProvider> token_provider,
                std::shared_ptr<ISessionStore> session_store,
                std::shared_ptr<IMudPlayerRepository> mud_player_repository,
                std::shared_ptr<IMudEventStore> mud_event_store);
    ~GameService() override;

public:
    bool start() override;
    bool stop() override;
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time) override;

protected:
    void on_event_loop_tick() override;

private:
    coro_task_t register_instance_async();
    coro_task_t unregister_instance_async();
    coro_task_t heartbeat_async();
    void emit_world_event();
    coro_task_t enter_game_async(evhttp_request* request);
    coro_task_t bootstrap_async(evhttp_request* request);
    coro_task_t create_character_async(evhttp_request* request);
    coro_task_t execute_command_async(evhttp_request* request);
    coro_task_t pull_feed_async(evhttp_request* request);
    coro_task_t rank_list_async(evhttp_request* request);

private:
    RuntimeConfig m_config;
    std::shared_ptr<IServiceDiscovery> m_discovery;
    std::shared_ptr<ITokenProvider> m_token_provider;
    std::shared_ptr<ISessionStore> m_session_store;
    std::shared_ptr<IMudPlayerRepository> m_mud_player_repository;
    std::shared_ptr<IMudEventStore> m_mud_event_store;
    std::shared_ptr<MudWorld> m_mud_world;
    std::unique_ptr<MudGameRuntime> m_mud_runtime;
    ServiceInstance m_local_instance;
    std::chrono::steady_clock::time_point m_last_heartbeat;
    bool m_heartbeat_inflight = false;
    bool m_world_event_inflight = false;
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

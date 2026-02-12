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

#include "account_repository.h"
#include "service_discovery.h"
#include "token_provider.h"

#include <atomic>
#include <chrono>
#include <memory>

class LoginService : public BasicHttpService
{
public:
    LoginService(const RuntimeConfig& config,
                 std::shared_ptr<IServiceDiscovery> discovery,
                 std::shared_ptr<IAccountRepository> account_repository,
                 std::shared_ptr<ITokenProvider> token_provider);
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
    coro_task_t heartbeat_async();
    coro_task_t register_async(evhttp_request* request);
    coro_task_t login_async(evhttp_request* request);

private:
    RuntimeConfig m_config;
    std::shared_ptr<IServiceDiscovery> m_discovery;
    std::shared_ptr<IAccountRepository> m_account_repository;
    std::shared_ptr<ITokenProvider> m_token_provider;
    ServiceInstance m_local_instance;
    std::chrono::steady_clock::time_point m_last_heartbeat;
    std::size_t m_game_round_robin_counter = 0;
    bool m_heartbeat_inflight = false;
    std::atomic<bool> m_register_inflight{false};
    std::atomic<bool> m_registered{false};
};

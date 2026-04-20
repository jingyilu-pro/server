//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "client_pressure_types.h"
#include "corocoroutine.h"
#include "coromanager.h"

#include <memory>

#include <string>
#include <vector>

struct HttpResponse;

class ClientWorker
{
public:
    explicit ClientWorker(int http_coro_workers = 1);
    ~ClientWorker();

    WorkerCycleResult run(ClientPressureTask* task) const;

private:
    class HttpClientManager;

private:
    bool run_http_awaitable(CoroAwaitable awaitable, class HttpResponse* out_response, int timeout_ms) const;
    CoroAwaitable http_post_async(const EndpointConfig& endpoint,
                                  const char* path,
                                  const std::string& protobuf_body,
                                  int timeout_ms,
                                  const std::vector<std::string>& headers) const;

    bool do_manager_route(ClientPressureTask* task, StageSample* sample) const;
    bool do_login(ClientPressureTask* task, StageSample* sample) const;
    bool do_register(ClientPressureTask* task, std::string* error_reason) const;
    bool do_enter_game(const ClientPressureTask& task, StageSample* sample) const;

private:
    mutable std::unique_ptr<HttpClientManager> m_http_manager;
    int m_http_coro_workers = 1;
};

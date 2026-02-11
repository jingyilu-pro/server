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

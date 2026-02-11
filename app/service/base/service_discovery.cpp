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

#include "service_discovery.h"

#include <chrono>
#include <memory>
#include <thread>

namespace
{

struct ServiceDiscoveryWaitState
{
    bool done = false;
    bool has_result = false;
    ServiceDiscoveryOpResult snapshot;
};

coro_task_t capture_service_discovery_result(CoroAwaitable awaitable,
                                             std::shared_ptr<ServiceDiscoveryWaitState> state)
{
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await awaitable);
    if(!state)
    {
        co_return;
    }

    if(result != nullptr)
    {
        state->snapshot.op_type = result->op_type;
        state->snapshot.request_instance = result->request_instance;
        state->snapshot.request_role = result->request_role;
        state->snapshot.success = result->success;
        state->snapshot.error = result->error;
        state->snapshot.instances = result->instances;
        state->has_result = true;
    }
    state->done = true;
}

} // namespace

bool wait_service_discovery_result(IServiceDiscovery* discovery,
                                   CoroAwaitable awaitable,
                                   ServiceDiscoveryOpResult* out_result,
                                   int timeout_ms)
{
    if(out_result == nullptr)
    {
        return false;
    }
    out_result->clear();

    auto state = std::make_shared<ServiceDiscoveryWaitState>();
    capture_service_discovery_result(awaitable, state);

    if(timeout_ms <= 0)
    {
        timeout_ms = 1000;
    }

    auto begin = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - begin)
              .count() <= timeout_ms)
    {
        if(discovery != nullptr)
        {
            discovery->poll();
        }

        if(state->done)
        {
            if(state->has_result)
            {
                out_result->op_type = state->snapshot.op_type;
                out_result->request_instance = state->snapshot.request_instance;
                out_result->request_role = state->snapshot.request_role;
                out_result->success = state->snapshot.success;
                out_result->error = state->snapshot.error;
                out_result->instances = state->snapshot.instances;
            }
            return state->has_result;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}

//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "session_store.h"

#include <chrono>
#include <memory>
#include <thread>

namespace
{

struct SessionStoreWaitState
{
    bool done = false;
    bool has_result = false;
    SessionStoreOpResult snapshot;
};

coro_task_t capture_session_store_result(CoroAwaitable awaitable,
                                         std::shared_ptr<SessionStoreWaitState> state)
{
    auto* result = dynamic_cast<SessionStoreOpResult*>(co_await awaitable);
    if(!state)
    {
        co_return;
    }

    if(result != nullptr)
    {
        state->snapshot.op_type = result->op_type;
        state->snapshot.request_account = result->request_account;
        state->snapshot.request_session = result->request_session;
        state->snapshot.request_ttl_sec = result->request_ttl_sec;
        state->snapshot.success = result->success;
        state->snapshot.hit = result->hit;
        state->snapshot.touch_ok = result->touch_ok;
        state->snapshot.upsert_ok = result->upsert_ok;
        state->snapshot.remove_ok = result->remove_ok;
        state->snapshot.error = result->error;
        state->snapshot.session = result->session;
        state->has_result = true;
    }
    state->done = true;
}

} // namespace

bool wait_session_store_result(ISessionStore* store,
                               CoroAwaitable awaitable,
                               SessionStoreOpResult* out_result,
                               int timeout_ms)
{
    if(out_result == nullptr)
    {
        return false;
    }
    out_result->clear();

    auto state = std::make_shared<SessionStoreWaitState>();
    capture_session_store_result(awaitable, state);

    if(timeout_ms <= 0)
    {
        timeout_ms = 1000;
    }

    auto begin = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - begin)
              .count() <= timeout_ms)
    {
        if(store != nullptr)
        {
            store->poll();
        }

        if(state->done)
        {
            if(state->has_result)
            {
                out_result->op_type = state->snapshot.op_type;
                out_result->request_account = state->snapshot.request_account;
                out_result->request_session = state->snapshot.request_session;
                out_result->request_ttl_sec = state->snapshot.request_ttl_sec;
                out_result->success = state->snapshot.success;
                out_result->hit = state->snapshot.hit;
                out_result->touch_ok = state->snapshot.touch_ok;
                out_result->upsert_ok = state->snapshot.upsert_ok;
                out_result->remove_ok = state->snapshot.remove_ok;
                out_result->error = state->snapshot.error;
                out_result->session = state->snapshot.session;
            }
            return state->has_result;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}


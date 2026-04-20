//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "account_cache_store.h"

#include <chrono>
#include <memory>
#include <thread>

namespace
{

struct AccountCacheWaitState
{
    bool done = false;
    bool has_result = false;
    AccountCacheOpResult snapshot;
};

coro_task_t capture_account_cache_result(CoroAwaitable awaitable,
                                         std::shared_ptr<AccountCacheWaitState> state)
{
    auto* result = dynamic_cast<AccountCacheOpResult*>(co_await awaitable);
    if(!state)
    {
        co_return;
    }

    if(result != nullptr)
    {
        state->snapshot.op_type = result->op_type;
        state->snapshot.request_account = result->request_account;
        state->snapshot.request_record = result->request_record;
        state->snapshot.request_ttl_sec = result->request_ttl_sec;
        state->snapshot.success = result->success;
        state->snapshot.hit = result->hit;
        state->snapshot.put_ok = result->put_ok;
        state->snapshot.erase_ok = result->erase_ok;
        state->snapshot.error = result->error;
        state->snapshot.record = result->record;
        state->has_result = true;
    }
    state->done = true;
}

} // namespace

bool wait_account_cache_store_result(IAccountCacheStore* store,
                                     CoroAwaitable awaitable,
                                     AccountCacheOpResult* out_result,
                                     int timeout_ms)
{
    if(out_result == nullptr)
    {
        return false;
    }
    out_result->clear();

    auto state = std::make_shared<AccountCacheWaitState>();
    capture_account_cache_result(awaitable, state);

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
                out_result->request_record = state->snapshot.request_record;
                out_result->request_ttl_sec = state->snapshot.request_ttl_sec;
                out_result->success = state->snapshot.success;
                out_result->hit = state->snapshot.hit;
                out_result->put_ok = state->snapshot.put_ok;
                out_result->erase_ok = state->snapshot.erase_ok;
                out_result->error = state->snapshot.error;
                out_result->record = state->snapshot.record;
            }
            return state->has_result;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}


//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include <coroutine>

typedef std::coroutine_handle<> coro_handle;
typedef std::suspend_never sus_never;

/**
 * 协程等待体
 * 支持返回类型
 */
template <typename T = int>
struct coro_awaitable
{
    virtual ~coro_awaitable() = default;
    [[nodiscard]] virtual bool await_ready() const { return false; }
    virtual T await_resume() = 0;
    virtual void await_suspend(coro_handle handle) = 0;
};

/**
 * 协程任务类型
 * 暂不支持返回值
 */
template <typename T = void>
struct coro_task
{
    struct promise_type
    {
        static coro_task<T> get_return_object_on_allocation_failure() { return {}; }
        static coro_task<T> get_return_object() { return {}; }
        static sus_never initial_suspend() { return {}; }
        static sus_never final_suspend() noexcept { return {}; }
        static void return_void() {}
        static void unhandled_exception() {}
    };
};
typedef coro_task<> coro_task_t;
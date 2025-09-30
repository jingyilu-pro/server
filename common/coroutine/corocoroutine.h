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

#include <coroutine>

typedef std::coroutine_handle<> coro_handle;
typedef std::suspend_never sus_never;


/**
 * 协程等待体
 * 支持返回类型
 */
template<typename T = int>
struct coro_awaitable
{
	virtual ~coro_awaitable()= default;
	[[nodiscard]] virtual bool await_ready() const { return false; }
	virtual T await_resume() = 0;
	virtual void await_suspend(coro_handle handle) = 0;
};

/**
 * 协程任务类型
 * 暂不支持返回值
 */
template<typename T = void>
struct coro_task
{
	struct promise_type {
		static coro_task<T> get_return_object_on_allocation_failure() { return {}; }
		static coro_task<T> get_return_object() { return {}; }
		static sus_never initial_suspend() { return {}; }
		static sus_never final_suspend() noexcept { return {}; }
		static void return_void() {}
		static void unhandled_exception() {}
	};
};
typedef coro_task<> coro_task_t;
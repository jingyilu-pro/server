//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "coromanager.h"
#include "define.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>

CoroManager::CoroManager(int worker_count)
    : m_worker_count(worker_count), m_update_time(time(nullptr)), m_recycle_time(0), m_results{}
{
}

CoroManager::~CoroManager()
{
    /* never delete until program is closed ...

    for(auto it = m_worker_threads.begin(); it != m_worker_threads.end(); ++it)
    {
        delete (*it);
    }   
    m_worker_threads.clear();
    */
}

void CoroManager::init()
{
#ifdef __DEBUG__
    m_worker_count = 1;
#endif
    for(int i = 0; i < m_worker_count; ++i)
    {
        auto th = new WorkerThread();
        th->init();
        m_worker_threads.push_back(th);
    }
}

void CoroManager::update()
{
    ensure_owner_thread("update");
    for(const auto& var : m_worker_threads)
    {
        const size_t co = var->get_results().try_dequeue_bulk(m_results, block_size);
        if(co == 0)
            continue;

        for(size_t i = 0; i < co; ++i)
        {
            auto* result = m_results[i];
            result->resume();
            release(result);
        }
    }

    // racy
    recycle();
}

void CoroManager::ensure_owner_thread(const char* api_name)
{
    const auto current_thread_id = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(m_owner_thread_mutex);
    if(m_owner_thread_id == std::thread::id{})
    {
        m_owner_thread_id = current_thread_id;
        return;
    }

    if(m_owner_thread_id == current_thread_id)
    {
        return;
    }

    const auto owner_hash = std::hash<std::thread::id>{}(m_owner_thread_id);
    const auto current_hash = std::hash<std::thread::id>{}(current_thread_id);
    std::fprintf(stderr,
                 "CoroManager thread-affinity violation at %s, owner=%zu current=%zu\n",
                 api_name == nullptr ? "unknown" : api_name,
                 owner_hash,
                 current_hash);
    std::abort();
}

CoroAwaitable CoroManager::start_coroutine(CoroManager* manager)
{
    CoroResult* result = manager->alloc();
    return CoroAwaitable{manager, result};
}

CoroAwaitable CoroManager::await_suspend_handle(CoroManager* manager, CoroResult* result)
{
    return CoroAwaitable{manager, result};
}

void CoroManager::recycle()
{
    m_update_time = time(nullptr);
    if(m_recycle_time > m_update_time)
        return;
    m_recycle_time = m_update_time + coro_result_recycle_interval;

    // gwarn("m_worker_threads={} m_result_pool={}", m_worker_threads.size(), m_result_pool.size());

    // 小于64没必要回收
    if(m_result_pool.size() < 64)
        return;

    do
    {
        size_t half_sz = m_result_pool.size() / 2;
        auto it = m_result_pool.begin();
        std::advance(it, half_sz);

        auto opt = *it;
        if(opt->alloc_time() < m_update_time)
        {
            m_recycle_list.splice(m_recycle_list.begin(), m_result_pool, it, m_result_pool.end());
        }

    } while(false);

    if(m_recycle_list.empty())
        return;
    for(auto opt : m_recycle_list)
    {
        SAFE_DELETE(opt);
    }
    m_recycle_list.clear();
}

TestCoroManager::~TestCoroManager() = default;

//////////
CoroAwaitable TestCoroManager::awaitable(const string& mask_word)
{
    auto* result = dynamic_cast<TestCoroResult*>(alloc());
    result->init(mask_word);
    return CoroAwaitable{this, result};
}

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

#include "corocoroutine.h"
#include "singleton.h"
#include <vector>
#include "workerthread.h"
#include <list>
#include "cororesult.h"

using namespace std;

constexpr int coro_result_recycle_interval = 5 * 60;
constexpr int block_size = 32;

struct CoroAwaitable;

class CoroManager
{
    friend struct CoroAwaitable;
public:
    explicit CoroManager(int worker_count = 1);
    virtual ~CoroManager();

    virtual void init();
    virtual void update();

public:
    static CoroAwaitable start_coroutine(CoroManager* manager);
    static CoroAwaitable await_suspend_handle(CoroManager* manager, CoroResult* result);
protected:
    virtual CoroResult* alloc() = 0;
    template<class T>
    void expand()
    {
        if(!pool_empty()) return;
        for(int i = 0; i < 16; ++i)
        {
            release(new(std::nothrow) T(), true);
        }
    }

    [[nodiscard]] bool pool_empty() const { return m_result_pool.empty(); }
    CoroResult* inner_alloc()
    {
        if(!m_result_pool.empty())
        {
            CoroResult* result = m_result_pool.front();
            m_result_pool.pop_front();
            result->set_valid(true);
            result->set_uid(m_global_index++);
            result->set_handle(nullptr);
            result->set_alloc_time(m_update_time + 2 * coro_result_recycle_interval);
            return result;
        }
        return nullptr;
    }
    void release(CoroResult* result, bool init = false)
    {
        if(!init)
            result->clear();
        m_result_pool.push_back(result);
    }
    void add_async_result(CoroResult* result) const {
        m_worker_threads[result->uid() % m_worker_count]->insert(result);
    }
    void recycle();
protected:
    uint64 m_global_index = 0;
    int m_worker_count = 1;
    std::vector<WorkerThread*> m_worker_threads;
    list<CoroResult*> m_result_pool;
    list<CoroResult*> m_recycle_list;
    time_t m_update_time;
    time_t m_recycle_time;
    CoroResult* m_results[block_size];
};

struct CoroAwaitable : public coro_awaitable<CoroResult*>
{
    CoroManager* m_manager = nullptr;
    CoroResult* m_result = nullptr;

    CoroAwaitable(CoroManager* manager, CoroResult* result) : m_manager{manager}, m_result{result} {}
    CoroResult* await_resume() override {return m_result;}
    void await_suspend(coro_handle handle) override {
        m_result->set_handle(handle);
        m_manager->add_async_result(m_result);
    }
};


// for example
class TestCoroManager : public CoroManager
{
public:
    TestCoroManager() = default;
    ~TestCoroManager() override;

    CoroAwaitable awaitable(const string& mask_word);
public:
    virtual CoroResult* alloc()
    {
        expand<TestCoroResult>();
        return inner_alloc();
    }

};


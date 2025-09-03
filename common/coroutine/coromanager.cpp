#include "coromanager.h"
#include "define.h"
#include <iostream>

CoroManager::CoroManager(int worker_count)
    :m_worker_count(worker_count)
    ,m_update_time(time(nullptr))
	,m_recycle_time(0)
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

CoroAwaitable CoroManager::start_coroutine(CoroManager* manager)
{
    CoroResult* result = manager->alloc();
    return CoroAwaitable{manager, result};
}

CoroAwaitable CoroManager::await_suspend_handle(CoroManager* manager, CoroResult* result)
{
    return CoroAwaitable{manager, result};
}

void CoroManager::update()
{
    CoroResult* result = nullptr;
    for(auto& var : m_worker_threads)
    {
        if(var->get_results().try_dequeue(result))
        {
            result->resume();
            release(result);
            
            result = nullptr;
        }
    }

    // recy
	recycle();
}

void CoroManager::recycle()
{
	m_update_time = time(nullptr);
	if(m_recycle_time > m_update_time) return;
	m_recycle_time = m_update_time + coro_result_recycle_interval;

	// gwarn("m_worker_threads={} m_result_pool={}", m_worker_threads.size(), m_result_pool.size());

	// 小于64没必要回收
	if(m_result_pool.size() < 64) return;

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
		
	} while (false);

	if(m_recycle_list.empty()) return;
	for(auto it = m_recycle_list.begin(); it != m_recycle_list.end(); ++it)
	{
		auto opt = *it;
		SAFE_DELETE(opt);
	}
	m_recycle_list.clear();
}


//////////
CoroAwaitable TestCoroManager::awaitable(const string& mask_word)
{
    TestCoroResult* result = (TestCoroResult*)alloc();
    result->init(mask_word);
    return CoroAwaitable{this, result};
}
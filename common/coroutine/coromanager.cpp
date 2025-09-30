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


#include "coromanager.h"
#include "define.h"
#include <iostream>

CoroManager::CoroManager(int worker_count)
	: m_worker_count(worker_count)
	  , m_update_time(time(nullptr))
	  , m_recycle_time(0), m_results{} {
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
	for(const auto& var : m_worker_threads)
	{
		const size_t co = var->get_results().try_dequeue_bulk(m_results, block_size);
		if(co == 0) continue;

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
	auto* result = dynamic_cast<TestCoroResult *>(alloc());
	result->init(mask_word);
	return CoroAwaitable{this, result};
}

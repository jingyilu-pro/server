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


#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include "corocoroutine.h"

#include "blockingconcurrentqueue.h"
#include "concurrentqueue.h"
using namespace moodycamel;

using namespace std;


class CoroResult;

class WorkerThread
{
public:
    WorkerThread();

    virtual ~WorkerThread();

    virtual void init();
    virtual void worker();

    void insert(CoroResult* opt);
    auto& get_results() { return m_results; } 
protected:
    thread m_thread;
    mutex m_mutex;
    condition_variable m_cv;

    BlockingConcurrentQueue<CoroResult*> m_opts;
    ConcurrentQueue<CoroResult*> m_results;
};
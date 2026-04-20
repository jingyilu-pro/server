//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
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
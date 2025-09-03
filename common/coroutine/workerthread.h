#pragma once


#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
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
    ~WorkerThread();

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
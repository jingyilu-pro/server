#include "workerthread.h"
#include "cororesult.h"


WorkerThread::WorkerThread()
{
}

WorkerThread::~WorkerThread()
{
    // never delete until program is closed ...
    m_thread.join();
}

void WorkerThread::init()
{
    m_thread = std::thread(&WorkerThread::worker, this);
    m_thread.detach();
}

void WorkerThread::worker()
{
    // std::cout << "WorkerThread thread id " << std::this_thread::get_id() << std::endl;

    CoroResult* opt = nullptr;
    while (true)
    {
        m_opts.wait_dequeue(opt);
        opt->worker();

        m_results.enqueue(opt);

        opt = nullptr;
      
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        usleep(25);
    }    
}

void WorkerThread::insert(CoroResult* opt)
{
    m_opts.enqueue(opt);
}
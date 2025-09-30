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
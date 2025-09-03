#include "application.h"
#include <iostream>


Application::Application(int thread_count/* = 1*/)
: m_check_maskword_manager(this, thread_count)
{

}

Application::~Application()
{

}

void Application::update()
{
    m_check_maskword_manager.update();

    static size_t stick = 0;
    size_t tick = time(nullptr);
    if(stick > tick) return;
    stick = tick + 1;

    // 此为自动测试代码
    // static uint64 sidx = 0;
    // for(int i = 0; i < 1000; ++i)
    // {
    //     check_maskword(to_string(sidx++));
    // }

    static bool start = false;
    if(!start)
    {
        start = true;
        for(int i = 0; i < 10000000; ++i)
        {
            check_maskword("check_maskword");
        }
    }

    static int total_idx = 0;
    static size_t souttick = 0;
    if(souttick < tick)
    {
        souttick = tick + 60;
        for(auto [id, co] : m_thread_statistics)
        {
            std::cout << " id=" << id << " co=" << co / 1000 << "k" << std::endl;
        }

        if(++total_idx == 110)
        {
            usleep(100000000000);
        }
    }
    
}

coro_task_t Application::check_maskword(const string& str)
{
    auto result = (CheckMaskWordResult*)co_await m_check_maskword_manager.awaitable(str);

    m_thread_statistics[std::this_thread::get_id()]++;

    check_maskword("check_maskword");
    
    // if(result->mask())
    // {
    //     std::cout << "thread=" << std::this_thread::get_id() << " str=" << result->str() << " mask=" << result->mask() << std::endl;
    // }
}


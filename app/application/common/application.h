#pragma once

#include "define.h"
#include "checkmaskwordmanager.h"
#include "corocoroutine.h"

#include <map>

class Application
{
public:
    Application(int thread_count = 1);
    ~Application();

    void update();

public:
    // 屏蔽字检测
    coro_task_t check_maskword(const string& str);
private:
    CheckMaskWordManager m_check_maskword_manager;
    
    map<std::thread::id, uint64> m_thread_statistics;
};

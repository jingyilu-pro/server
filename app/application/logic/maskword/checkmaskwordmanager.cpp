#include "checkmaskwordmanager.h"
#include "application.h"

CheckMaskWordManager::CheckMaskWordManager(Application* app, int thread_count/* = 1*/)
    : CoroManager(thread_count)
    , m_app(app)
{
    init();
}

CheckMaskWordManager::~CheckMaskWordManager()
{

}

CoroAwaitable CheckMaskWordManager::awaitable(const string& mask_word)
{
    // 会自动回收对象
    CheckMaskWordResult* result = (CheckMaskWordResult*)alloc();
    result->init(mask_word);
    return CoroAwaitable{this, result};
}


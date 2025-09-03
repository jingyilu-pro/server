#pragma once

#include "define.h"
#include <set>
#include "corocoroutine.h"
#include "coromanager.h"
#include "checkmaskwordresult.h"

class Application;

class CheckMaskWordManager : public CoroManager
{
public:
    CheckMaskWordManager(Application* app, int thread_count = 1);
    ~CheckMaskWordManager();

    CoroAwaitable awaitable(const string& mask_word);

public:
    virtual CoroResult* alloc()
    {
        expand<CheckMaskWordResult>();
        return inner_alloc();
    }
private:
    PROPERTY(Application*, app)
};


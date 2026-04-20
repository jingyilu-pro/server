//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "define.h"
#include "corocoroutine.h"

#include <map>

class AppTool
{
public:
    explicit AppTool(int thread_count = 1);

    ~AppTool();

    bool start();
    bool stop();
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time);
};

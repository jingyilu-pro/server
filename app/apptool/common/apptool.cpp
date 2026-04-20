//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "apptool.h"
#include <iostream>
#include <ranges>

#include "log/glogger.h"

AppTool::AppTool(int thread_count /* = 1*/)
{
}

AppTool::~AppTool() = default;

bool AppTool::start()
{
    Glogger::init();
    Glogger::set_log_level(0);

    return true;
}

bool AppTool::stop()
{

    return true;
}

void AppTool::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
}

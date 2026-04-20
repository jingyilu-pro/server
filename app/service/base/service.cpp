//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "service.h"

const char* Service::name() const
{
    return "service";
}

bool Service::start()
{
    return true;
}

bool Service::stop()
{
    return true;
}

void Service::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
}

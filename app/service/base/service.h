//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "define.h"

class Service
{
public:
    Service() = default;
    virtual ~Service() = default;

public:
    virtual const char* name() const;
    virtual bool start();
    virtual bool stop();
    virtual void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time);
};

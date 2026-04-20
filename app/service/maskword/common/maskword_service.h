//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include <service.h>
#include "maskword_manager.h"

class MaskWordService : public Service
{
public:
    MaskWordService();
    virtual ~MaskWordService();

    auto& maskword_manager() { return m_maskword_manager; }

public:
    virtual bool start();
    virtual bool stop();
    virtual void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time);

private:
    MaskWordManager m_maskword_manager;
};

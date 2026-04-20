//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "maskword_service.h"
#include "protocol/base.pb.h"
#include "log/glogger.h"

MaskWordService::MaskWordService()
    : m_maskword_manager(this)
{
}

MaskWordService::~MaskWordService()
{
}

bool MaskWordService::start()
{
    base::Person person;
    person.set_id(111);

    spdlog::info("person={}", person.id());

    return true;
}

bool MaskWordService::stop()
{
    return true;
}

void MaskWordService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
}

//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include <service.h>

#include "application_config.h"
#include "client_pressure_manager.h"

#include <memory>

class ClientPressureService : public Service
{
public:
    explicit ClientPressureService(const RuntimeConfig& config);
    ~ClientPressureService() override;

public:
    const char* name() const override;
    bool start() override;
    bool stop() override;
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time) override;
    bool completed() const;

private:
    RuntimeConfig m_config;
    std::unique_ptr<ClientPressureManager> m_manager;
};

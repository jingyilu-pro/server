//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "client_pressure_service.h"

#include "log/glogger.h"

ClientPressureService::ClientPressureService(const RuntimeConfig& config)
    : m_config(config), m_manager(std::make_unique<ClientPressureManager>(config))
{
}

ClientPressureService::~ClientPressureService() = default;

const char* ClientPressureService::name() const
{
    return "client_pressure";
}

bool ClientPressureService::start()
{
    if(!m_config.client_pressure.enabled)
    {
        spdlog::warn("client pressure service started in disabled mode; no traffic will be generated");
        return true;
    }

    return m_manager->start();
}

bool ClientPressureService::stop()
{
    if(m_manager)
    {
        m_manager->stop();
    }
    return true;
}

void ClientPressureService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    if(!m_config.client_pressure.enabled)
    {
        return;
    }
    if(m_manager)
    {
        m_manager->update(delta_time, last_tick_time);
    }
}

bool ClientPressureService::completed() const
{
    if(!m_config.client_pressure.enabled)
    {
        return true;
    }
    if(!m_manager)
    {
        return true;
    }
    return m_manager->completed();
}

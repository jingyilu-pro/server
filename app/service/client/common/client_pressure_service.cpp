//
// Copyright (c) 2024-2025 JingyiLu jingyilupro@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
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

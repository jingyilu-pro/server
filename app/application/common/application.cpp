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

#include "application.h"

#include <algorithm>
#include <cctype>
#include <ranges>

#include "client_pressure_service.h"
#include "game_service.h"
#include "login_service.h"
#include "manager_service.h"
#include "service.h"

#include "log/glogger.h"

namespace
{

int service_slot_index(AppMode mode)
{
    switch(mode)
    {
    case AppMode::manager:
        return 0;
    case AppMode::login:
        return 1;
    case AppMode::game:
        return 2;
    case AppMode::client:
        return 3;
    case AppMode::all:
        break;
    }
    return -1;
}

} // namespace

Application::Application(int thread_count /* = 1*/)
    : m_thread_count(thread_count)
{
}

Application::~Application() = default;

const char* to_string(AppMode mode)
{
    switch(mode)
    {
    case AppMode::all:
        return "all";
    case AppMode::manager:
        return "manager";
    case AppMode::login:
        return "login";
    case AppMode::game:
        return "game";
    case AppMode::client:
        return "client";
    }
    return "unknown";
}

std::optional<AppMode> parse_app_mode(std::string_view mode_text)
{
    std::string text(mode_text.begin(), mode_text.end());
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if(text == "all")
    {
        return AppMode::all;
    }
    if(text == "manager")
    {
        return AppMode::manager;
    }
    if(text == "login")
    {
        return AppMode::login;
    }
    if(text == "game")
    {
        return AppMode::game;
    }
    if(text == "client")
    {
        return AppMode::client;
    }
    return std::nullopt;
}

bool Application::start(const ApplicationOptions& options)
{
    Glogger::init();
    Glogger::set_log_level(0);

    m_options = options;

    std::string config_error;
    if(!load_runtime_config(options.config_path, &m_runtime_config, &config_error))
    {
        spdlog::error("failed to load config '{}': {}", options.config_path, config_error);
        return false;
    }

    spdlog::info("application start mode={}, config={}", to_string(options.mode), options.config_path);

    if(!start_services_by_mode())
    {
        spdlog::error("service start failed in mode={}", to_string(options.mode));
        stop();
        return false;
    }

    return true;
}

bool Application::start_services_by_mode()
{
    switch(m_options.mode)
    {
    case AppMode::all:
        if(!start_non_client_services())
        {
            return false;
        }
        return start_client_service();
    case AppMode::manager:
        return register_service(std::make_unique<ManagerService>(m_runtime_config));
    case AppMode::login:
        return register_service(std::make_unique<LoginService>(m_runtime_config));
    case AppMode::game:
        return register_service(std::make_unique<GameService>(m_runtime_config));
    case AppMode::client:
        return start_client_service();
    }
    return false;
}

bool Application::start_non_client_services()
{
    if(!register_service(std::make_unique<ManagerService>(m_runtime_config)))
    {
        return false;
    }
    if(!register_service(std::make_unique<LoginService>(m_runtime_config)))
    {
        return false;
    }
    if(!register_service(std::make_unique<GameService>(m_runtime_config)))
    {
        return false;
    }
    return true;
}

bool Application::start_client_service()
{
    return register_service(std::make_unique<ClientPressureService>(m_runtime_config));
}

bool Application::register_service(std::unique_ptr<Service> service)
{
    if(!service)
    {
        return false;
    }
    const char* service_name = service->name();
    if(!service->start())
    {
        spdlog::error("service '{}' start failed", service_name);
        return false;
    }

    int index = static_cast<int>(m_services.size());
    if(service_name != nullptr)
    {
        if(std::string_view(service_name) == "manager")
        {
            index = service_slot_index(AppMode::manager);
        }
        else if(std::string_view(service_name) == "login")
        {
            index = service_slot_index(AppMode::login);
        }
        else if(std::string_view(service_name) == "game")
        {
            index = service_slot_index(AppMode::game);
        }
        else if(std::string_view(service_name) == "client_pressure")
        {
            index = service_slot_index(AppMode::client);
        }
    }

    if(auto* client_service = dynamic_cast<ClientPressureService*>(service.get()))
    {
        m_client_service = client_service;
    }

    m_services[index] = std::move(service);
    spdlog::info("service '{}' started", service_name == nullptr ? "unknown" : service_name);

    return true;
}

bool Application::stop()
{
    for(auto& service : m_services | views::values)
    {
        if(service)
        {
            service->stop();
        }
    }
    m_services.clear();
    m_client_service = nullptr;
    m_should_exit = true;

    return true;
}

void Application::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    for(auto& val : m_services | views::values)
    {
        if(val)
        {
            val->update(delta_time, last_tick_time);
        }
    }

    if(m_options.mode == AppMode::client && m_client_service != nullptr && m_client_service->completed())
    {
        spdlog::info("client pressure completed, application exiting");
        stop();
    }
}

bool Application::should_exit() const
{
    return m_should_exit;
}

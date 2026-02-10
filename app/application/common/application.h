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

#pragma once

#include "define.h"
#include "corocoroutine.h"
#include "application_config.h"

#include <map>
#include <memory>
#include <optional>
#include <string_view>

class Service;
class ClientPressureService;

enum class AppMode
{
    all,
    manager,
    login,
    game,
    client
};

struct ApplicationOptions
{
    AppMode mode = AppMode::all;
    std::string config_path = "all.yaml";
};

const char* to_string(AppMode mode);
std::optional<AppMode> parse_app_mode(std::string_view mode_text);

class Application
{
public:
    explicit Application(int thread_count = 1);

    ~Application();

    bool start(const ApplicationOptions& options);
    bool stop();
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time);
    bool should_exit() const;

private:
    bool start_services_by_mode();
    bool start_non_client_services();
    bool start_client_service();
    bool register_service(std::unique_ptr<Service> service);

private:
    int m_thread_count = 1;
    ApplicationOptions m_options;
    RuntimeConfig m_runtime_config;
    bool m_should_exit = false;
    ClientPressureService* m_client_service = nullptr;
    std::map<int, std::unique_ptr<Service>> m_services;
};

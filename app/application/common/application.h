//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
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
struct ServerContext;

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
    std::shared_ptr<ServerContext> m_server_context;
    bool m_should_exit = false;
    ClientPressureService* m_client_service = nullptr;
    std::map<int, std::unique_ptr<Service>> m_services;
};

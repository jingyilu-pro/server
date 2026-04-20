//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "glogger.h"
// #include "fmt/fmt.h"

void Glogger::init(const std::string& service, const std::string& file_name, const int file_count, const int max_file_size)
{
    spdlog::cfg::load_env_levels();

    // auto old_logger = spdlog::default_logger();
    // auto glogger = spdlog::rotating_logger_mt(service, std::format("logs/{}.log", file_name), max_file_size, file_count);

    // spdlog::set_default_logger(glogger);

    spdlog::flush_every(std::chrono::seconds(3));

    spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
}

void Glogger::set_log_level(int level)
{
    spdlog::set_level((spdlog::level::level_enum)level);

    spdlog::info("set log level to {}", level);
}
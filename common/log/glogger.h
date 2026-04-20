//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include <string>

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/sinks/rotating_file_sink.h"

/*
// Log level enum
namespace level {
enum level_enum : int {
    trace = SPDLOG_LEVEL_TRACE, 0
    debug = SPDLOG_LEVEL_DEBUG, 1
    info = SPDLOG_LEVEL_INFO, 2
    warn = SPDLOG_LEVEL_WARN, 3
    err = SPDLOG_LEVEL_ERROR, 4
    critical = SPDLOG_LEVEL_CRITICAL, 5
    off = SPDLOG_LEVEL_OFF, 6
    n_levels
};
*/
class Glogger
{
public:
    static void init(const std::string& service = "service", const std::string& file_name = "log", const int file_count = 128, const int max_file_size = 16 * 1024 * 1024);
    static void set_log_level(int level);
};

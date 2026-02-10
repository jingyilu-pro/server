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

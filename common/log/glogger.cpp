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


#include "glogger.h"
// #include "fmt/fmt.h"

void Glogger::init(const std::string& service, const std::string& file_name, const int file_count, const int max_file_size)
{
    spdlog::cfg::load_env_levels();

    auto old_logger = spdlog::default_logger();
    auto glogger = spdlog::rotating_logger_mt(service, std::format("logs/{}.log", file_name), max_file_size, file_count);

    spdlog::set_default_logger(glogger);
    
    spdlog::flush_every(std::chrono::seconds(3));

    spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
}

void Glogger::set_log_level(int level)
{
    spdlog::set_level((spdlog::level::level_enum)level);

    spdlog::info("set log level to {}", level);
}
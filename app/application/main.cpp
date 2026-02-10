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

#include <chrono>
#include <cstdlib>
#include <iostream>
#include "application.h"
#include "log/glogger.h"
#include "protocol/base.pb.h"

using namespace std;

namespace
{

void print_usage(const char* app)
{
    std::cout << "Usage: " << app << " [--mode all|manager|login|game|client] [--config path]" << std::endl;
}

bool parse_cli(int argc, char* argv[], ApplicationOptions* options)
{
    if(options == nullptr)
    {
        return false;
    }

    for(int index = 1; index < argc; ++index)
    {
        std::string arg = argv[index];
        if(arg == "--mode")
        {
            if(index + 1 >= argc)
            {
                std::cerr << "missing value for --mode" << std::endl;
                return false;
            }
            auto parsed_mode = parse_app_mode(argv[++index]);
            if(!parsed_mode)
            {
                std::cerr << "invalid mode: " << argv[index] << std::endl;
                return false;
            }
            options->mode = *parsed_mode;
            continue;
        }

        if(arg == "--config")
        {
            if(index + 1 >= argc)
            {
                std::cerr << "missing value for --config" << std::endl;
                return false;
            }
            options->config_path = argv[++index];
            continue;
        }

        if(arg == "-h" || arg == "--help")
        {
            print_usage(argv[0]);
            std::exit(0);
        }

        std::cerr << "unknown argument: " << arg << std::endl;
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    // std::cout << "main thead=" << std::this_thread::get_id() << " hardware_concurrency=" << std::thread::hardware_concurrency() << std::endl;

    ApplicationOptions options;
    if(!parse_cli(argc, argv, &options))
    {
        print_usage(argv[0]);
        return 1;
    }

    spdlog::info("hardware_concurrency={}", std::thread::hardware_concurrency());
    spdlog::info("application run mode={}, config={}", to_string(options.mode), options.config_path);

    Application app(std::thread::hardware_concurrency());

    base::Person person;
    person.set_id(222);

    if(!app.start(options))
    {
        return 1;
    }

    auto last_tick = std::chrono::steady_clock::now();
    auto last_tick_time = std::chrono::duration_cast<std::chrono::milliseconds>(1_tick);
    while(true)
    {
        auto now_tick = std::chrono::steady_clock::now();
        auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now_tick - last_tick);
        app.update(delta_time, last_tick_time);
        last_tick_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - now_tick);

        if(last_tick_time < 1_tick)
        {
            // Stretch tick time until it's at least 1 tick:
            std::this_thread::sleep_for(1_tick - last_tick_time);
        }

        last_tick = now_tick;

        if(app.should_exit())
        {
            break;
        }
    }

    app.stop();

    return 0;
};

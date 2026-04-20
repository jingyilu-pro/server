//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include <chrono>
#include <iostream>
#include "apptool.h"
#include "log/glogger.h"
#include "protocol/base.pb.h"

using namespace std;

int main(int argc, char* argv[])
{
    // std::cout << "main thead=" << std::this_thread::get_id() << " hardware_concurrency=" << std::thread::hardware_concurrency() << std::endl;

    spdlog::info("hardware_concurrency={}", std::thread::hardware_concurrency());

    AppTool app(std::thread::hardware_concurrency());

    app.start();

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
    }

    app.stop();

    return 0;
};

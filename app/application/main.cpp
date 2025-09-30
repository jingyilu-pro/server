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
#include <iostream>
#include "application.h"

using namespace std;


int main(int argc, char *argv[])
 {
    std::cout << "main thead=" << std::this_thread::get_id() << " hardware_concurrency=" <<
            std::thread::hardware_concurrency() << std::endl;


    Application app(std::thread::hardware_concurrency());

    app.start();

    while (true)
    {
        app.update();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    app.stop();

    return 0;
};

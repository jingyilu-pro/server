#include <time.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <set>
#include <map>
#include <iostream>
#include "utils.hpp"
#include <fstream>
#include <stdlib.h>
#include "application.h"

#include "concurrentqueue.h"

using namespace moodycamel;
using namespace std;


int main(int argc, char* argv[])
{

    std::cout << "main thead=" << std::this_thread::get_id() << " hardware_concurrency=" << std::thread::hardware_concurrency() << std::endl;
   
    // coro_http_client client{};
    // async_simple::coro::syncAwait(get_post(client));

    Application app(6);

    while(true)
    {
        app.update();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

   
    return 0;
};

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

#include "application_config.h"

#include <string>
#include <vector>

struct ServiceInstance
{
    std::string role;
    EndpointConfig endpoint;
    int weight = 1;
    std::string instance_id;
};

class IServiceDiscovery
{
public:
    virtual ~IServiceDiscovery() = default;

public:
    virtual bool register_instance(const ServiceInstance& instance) = 0;
    virtual bool heartbeat(const ServiceInstance& instance) = 0;
    virtual std::vector<ServiceInstance> list_instances(const std::string& role) = 0;
    virtual bool unregister_instance(const ServiceInstance& instance) = 0;
};


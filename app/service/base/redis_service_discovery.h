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

#include "service_discovery.h"

#include <hiredis/hiredis.h>

#include <mutex>

class RedisServiceDiscovery : public IServiceDiscovery
{
public:
    explicit RedisServiceDiscovery(const RedisConfig& config);
    ~RedisServiceDiscovery() override;

public:
    bool register_instance(const ServiceInstance& instance) override;
    bool heartbeat(const ServiceInstance& instance) override;
    std::vector<ServiceInstance> list_instances(const std::string& role) override;
    bool unregister_instance(const ServiceInstance& instance) override;

private:
    bool ensure_connected();
    bool set_instance_hash(const ServiceInstance& instance);
    bool expire_instance(const ServiceInstance& instance);
    std::string make_instance_key(const ServiceInstance& instance) const;
    std::string make_role_set_key(const std::string& role) const;

private:
    RedisConfig m_config;
    redisContext* m_context = nullptr;
    std::mutex m_mutex;
};


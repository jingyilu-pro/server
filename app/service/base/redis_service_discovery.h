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

#include <memory>
#include <mutex>

class RedisDiscoveryCoroManager : public CoroManager
{
public:
    explicit RedisDiscoveryCoroManager(int worker_count);
    ~RedisDiscoveryCoroManager() override;

public:
    CoroResult* alloc() override;
};

class RedisServiceDiscovery : public IServiceDiscovery
{
public:
    explicit RedisServiceDiscovery(const RedisConfig& config);
    ~RedisServiceDiscovery() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable register_instance(const ServiceInstance& instance) override;
    CoroAwaitable heartbeat(const ServiceInstance& instance) override;
    CoroAwaitable list_instances(const std::string& role) override;
    CoroAwaitable unregister_instance(const ServiceInstance& instance) override;

private:
    ServiceDiscoveryOpResult* alloc_result();
    void execute_operation(ServiceDiscoveryOpResult* result);
    bool ensure_connected();
    bool set_instance_hash(const ServiceInstance& instance);
    bool expire_instance(const ServiceInstance& instance);
    std::vector<ServiceInstance> query_instances_by_role(const std::string& role);
    std::string make_instance_key(const ServiceInstance& instance) const;
    std::string make_role_set_key(const std::string& role) const;

private:
    RedisConfig m_config;
    redisContext* m_context = nullptr;
    bool m_ready = false;
    std::unique_ptr<RedisDiscoveryCoroManager> m_manager;
    mutable std::mutex m_mutex;
};


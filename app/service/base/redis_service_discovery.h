//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
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

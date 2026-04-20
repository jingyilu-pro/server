//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "coromanager.h"

#include <functional>
#include <memory>

#include <string>
#include <vector>

struct ServiceInstance
{
    std::string role;
    EndpointConfig endpoint;
    int weight = 1;
    std::string instance_id;
};

enum class ServiceDiscoveryOpType
{
    register_instance,
    heartbeat,
    list_instances,
    unregister_instance
};

class ServiceDiscoveryOpResult : public CoroResult
{
public:
    ServiceDiscoveryOpResult() = default;
    ~ServiceDiscoveryOpResult() override = default;

    void init(ServiceDiscoveryOpType op,
              ServiceInstance instance,
              std::string role,
              std::function<void(ServiceDiscoveryOpResult*)> worker_fn)
    {
        op_type = op;
        request_instance = std::move(instance);
        request_role = std::move(role);
        success = false;
        error.clear();
        instances.clear();
        m_worker_fn = std::move(worker_fn);
    }

    void worker() override
    {
        if(m_worker_fn)
        {
            m_worker_fn(this);
            return;
        }

        success = false;
        error = "missing service discovery worker";
    }

    void clear() override
    {
        request_instance = {};
        request_role.clear();
        success = false;
        error.clear();
        instances.clear();
        m_worker_fn = nullptr;
    }

public:
    ServiceDiscoveryOpType op_type = ServiceDiscoveryOpType::list_instances;
    ServiceInstance request_instance;
    std::string request_role;
    bool success = false;
    std::string error;
    std::vector<ServiceInstance> instances;

private:
    std::function<void(ServiceDiscoveryOpResult*)> m_worker_fn;
};

class IServiceDiscovery;
bool wait_service_discovery_result(IServiceDiscovery* discovery,
                                   CoroAwaitable awaitable,
                                   ServiceDiscoveryOpResult* out_result,
                                   int timeout_ms);

class IServiceDiscovery
{
public:
    virtual ~IServiceDiscovery() = default;

public:
    virtual bool ready() const = 0;
    virtual void poll() = 0;
    virtual CoroAwaitable register_instance(const ServiceInstance& instance) = 0;
    virtual CoroAwaitable heartbeat(const ServiceInstance& instance) = 0;
    virtual CoroAwaitable list_instances(const std::string& role) = 0;
    virtual CoroAwaitable unregister_instance(const ServiceInstance& instance) = 0;
};

class NoopServiceDiscovery final : public IServiceDiscovery
{
public:
    NoopServiceDiscovery();
    ~NoopServiceDiscovery() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable register_instance(const ServiceInstance& instance) override;
    CoroAwaitable heartbeat(const ServiceInstance& instance) override;
    CoroAwaitable list_instances(const std::string& role) override;
    CoroAwaitable unregister_instance(const ServiceInstance& instance) override;

private:
    class NoopDiscoveryManager;

private:
    std::unique_ptr<NoopDiscoveryManager> m_manager;
};

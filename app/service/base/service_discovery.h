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
#include "coromanager.h"

#include <functional>

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

//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "service_discovery.h"

#include <chrono>
#include <memory>
#include <thread>

namespace
{

class NoopDiscoveryOpManager : public CoroManager
{
public:
    NoopDiscoveryOpManager()
        : CoroManager(1)
    {
        init();
    }
    ~NoopDiscoveryOpManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<ServiceDiscoveryOpResult>();
        return inner_alloc();
    }
};

} // namespace

namespace
{

struct ServiceDiscoveryWaitState
{
    bool done = false;
    bool has_result = false;
    ServiceDiscoveryOpResult snapshot;
};

coro_task_t capture_service_discovery_result(CoroAwaitable awaitable,
                                             std::shared_ptr<ServiceDiscoveryWaitState> state)
{
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(co_await awaitable);
    if(!state)
    {
        co_return;
    }

    if(result != nullptr)
    {
        state->snapshot.op_type = result->op_type;
        state->snapshot.request_instance = result->request_instance;
        state->snapshot.request_role = result->request_role;
        state->snapshot.success = result->success;
        state->snapshot.error = result->error;
        state->snapshot.instances = result->instances;
        state->has_result = true;
    }
    state->done = true;
}

} // namespace

bool wait_service_discovery_result(IServiceDiscovery* discovery,
                                   CoroAwaitable awaitable,
                                   ServiceDiscoveryOpResult* out_result,
                                   int timeout_ms)
{
    if(out_result == nullptr)
    {
        return false;
    }
    out_result->clear();

    auto state = std::make_shared<ServiceDiscoveryWaitState>();
    capture_service_discovery_result(awaitable, state);

    if(timeout_ms <= 0)
    {
        timeout_ms = 1000;
    }

    auto begin = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - begin)
              .count() <= timeout_ms)
    {
        if(discovery != nullptr)
        {
            discovery->poll();
        }

        if(state->done)
        {
            if(state->has_result)
            {
                out_result->op_type = state->snapshot.op_type;
                out_result->request_instance = state->snapshot.request_instance;
                out_result->request_role = state->snapshot.request_role;
                out_result->success = state->snapshot.success;
                out_result->error = state->snapshot.error;
                out_result->instances = state->snapshot.instances;
            }
            return state->has_result;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}

class NoopServiceDiscovery::NoopDiscoveryManager : public NoopDiscoveryOpManager
{
};

NoopServiceDiscovery::NoopServiceDiscovery()
{
    m_manager = std::make_unique<NoopDiscoveryManager>();
}

NoopServiceDiscovery::~NoopServiceDiscovery() = default;

bool NoopServiceDiscovery::ready() const
{
    return true;
}

void NoopServiceDiscovery::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

CoroAwaitable NoopServiceDiscovery::register_instance(const ServiceInstance& instance)
{
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::register_instance,
                 instance,
                 instance.role,
                 [](ServiceDiscoveryOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopServiceDiscovery::heartbeat(const ServiceInstance& instance)
{
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::heartbeat,
                 instance,
                 instance.role,
                 [](ServiceDiscoveryOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopServiceDiscovery::list_instances(const std::string& role)
{
    ServiceInstance instance;
    instance.role = role;

    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::list_instances,
                 instance,
                 role,
                 [](ServiceDiscoveryOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                     op->instances.clear();
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable NoopServiceDiscovery::unregister_instance(const ServiceInstance& instance)
{
    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::unregister_instance,
                 instance,
                 instance.role,
                 [](ServiceDiscoveryOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }
                     op->success = true;
                 });
    return CoroAwaitable{m_manager.get(), result};
}

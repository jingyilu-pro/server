//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "redis_service_discovery.h"

#include "log/glogger.h"

#include <algorithm>
#include <sstream>

namespace
{

std::string safe_instance_id(const ServiceInstance& instance)
{
    if(!instance.instance_id.empty())
    {
        return instance.instance_id;
    }

    std::ostringstream output;
    output << instance.endpoint.host << ":" << instance.endpoint.port;
    return output.str();
}

} // namespace

RedisDiscoveryCoroManager::RedisDiscoveryCoroManager(int worker_count)
    : CoroManager(worker_count)
{
    CoroManager::init();
}

RedisDiscoveryCoroManager::~RedisDiscoveryCoroManager() = default;

CoroResult* RedisDiscoveryCoroManager::alloc()
{
    expand<ServiceDiscoveryOpResult>();
    return inner_alloc();
}

RedisServiceDiscovery::RedisServiceDiscovery(const RedisConfig& config)
    : m_config(config)
{
    m_manager = std::make_unique<RedisDiscoveryCoroManager>(std::max(1, m_config.coro_workers));

    std::lock_guard lock(m_mutex);
    m_ready = ensure_connected();
}

RedisServiceDiscovery::~RedisServiceDiscovery()
{
    std::lock_guard lock(m_mutex);
    if(m_context != nullptr)
    {
        redisFree(m_context);
        m_context = nullptr;
    }
}

bool RedisServiceDiscovery::ready() const
{
    return m_ready;
}

void RedisServiceDiscovery::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

ServiceDiscoveryOpResult* RedisServiceDiscovery::alloc_result()
{
    if(m_manager == nullptr)
    {
        return nullptr;
    }

    auto* result = dynamic_cast<ServiceDiscoveryOpResult*>(m_manager->alloc());
    return result;
}

CoroAwaitable RedisServiceDiscovery::register_instance(const ServiceInstance& instance)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::register_instance,
                 instance,
                 instance.role,
                 [this](ServiceDiscoveryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisServiceDiscovery::heartbeat(const ServiceInstance& instance)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::heartbeat,
                 instance,
                 instance.role,
                 [this](ServiceDiscoveryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisServiceDiscovery::list_instances(const std::string& role)
{
    ServiceInstance dummy_instance;
    dummy_instance.role = role;

    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::list_instances,
                 dummy_instance,
                 role,
                 [this](ServiceDiscoveryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable RedisServiceDiscovery::unregister_instance(const ServiceInstance& instance)
{
    auto* result = alloc_result();
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(ServiceDiscoveryOpType::unregister_instance,
                 instance,
                 instance.role,
                 [this](ServiceDiscoveryOpResult* op) {
                     execute_operation(op);
                 });
    return CoroAwaitable{m_manager.get(), result};
}

void RedisServiceDiscovery::execute_operation(ServiceDiscoveryOpResult* result)
{
    if(result == nullptr)
    {
        return;
    }

    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        result->success = false;
        result->error = "redis_unavailable";
        return;
    }

    switch(result->op_type)
    {
    case ServiceDiscoveryOpType::register_instance:
    {
        const bool ok = set_instance_hash(result->request_instance) &&
                        expire_instance(result->request_instance);
        if(!ok)
        {
            result->success = false;
            result->error = "redis_register_hash_or_expire_failed";
            break;
        }

        auto role_set_key = make_role_set_key(result->request_instance.role);
        auto instance_key = make_instance_key(result->request_instance);
        auto* reply = static_cast<redisReply*>(redisCommand(m_context,
                                                            "SADD %s %s",
                                                            role_set_key.c_str(),
                                                            instance_key.c_str()));
        if(reply == nullptr)
        {
            result->success = false;
            result->error = "redis_register_sadd_failed";
            break;
        }
        freeReplyObject(reply);
        result->success = true;
        break;
    }
    case ServiceDiscoveryOpType::heartbeat:
    {
        result->success = set_instance_hash(result->request_instance) && expire_instance(result->request_instance);
        if(!result->success)
        {
            result->error = "redis_heartbeat_failed";
        }
        break;
    }
    case ServiceDiscoveryOpType::list_instances:
    {
        result->instances = query_instances_by_role(result->request_role);
        result->success = true;
        break;
    }
    case ServiceDiscoveryOpType::unregister_instance:
    {
        auto role_set_key = make_role_set_key(result->request_instance.role);
        auto instance_key = make_instance_key(result->request_instance);

        auto* remove_reply = static_cast<redisReply*>(redisCommand(m_context,
                                                                   "SREM %s %s",
                                                                   role_set_key.c_str(),
                                                                   instance_key.c_str()));
        if(remove_reply != nullptr)
        {
            freeReplyObject(remove_reply);
        }

        auto* del_reply = static_cast<redisReply*>(redisCommand(m_context, "DEL %s", instance_key.c_str()));
        if(del_reply == nullptr)
        {
            result->success = false;
            result->error = "redis_unregister_del_failed";
            break;
        }
        freeReplyObject(del_reply);
        result->success = true;
        break;
    }
    default:
        result->success = false;
        result->error = "redis_unknown_operation";
        break;
    }
}

std::vector<ServiceInstance> RedisServiceDiscovery::query_instances_by_role(const std::string& role)
{
    std::vector<ServiceInstance> out;

    auto role_set_key = make_role_set_key(role);
    auto* set_reply = static_cast<redisReply*>(redisCommand(m_context, "SMEMBERS %s", role_set_key.c_str()));
    if(set_reply == nullptr)
    {
        return out;
    }

    if(set_reply->type != REDIS_REPLY_ARRAY)
    {
        freeReplyObject(set_reply);
        return out;
    }

    for(size_t index = 0; index < set_reply->elements; ++index)
    {
        auto* key_reply = set_reply->element[index];
        if(key_reply == nullptr || key_reply->str == nullptr)
        {
            continue;
        }

        auto instance_key = std::string(key_reply->str, key_reply->len);
        auto* ttl_reply = static_cast<redisReply*>(redisCommand(m_context, "TTL %s", instance_key.c_str()));
        if(ttl_reply == nullptr)
        {
            continue;
        }
        const auto ttl = ttl_reply->type == REDIS_REPLY_INTEGER ? ttl_reply->integer : -2;
        freeReplyObject(ttl_reply);
        if(ttl <= 0)
        {
            auto* remove_reply = static_cast<redisReply*>(redisCommand(m_context,
                                                                       "SREM %s %s",
                                                                       role_set_key.c_str(),
                                                                       instance_key.c_str()));
            if(remove_reply != nullptr)
            {
                freeReplyObject(remove_reply);
            }
            continue;
        }

        auto* hash_reply = static_cast<redisReply*>(redisCommand(m_context, "HGETALL %s", instance_key.c_str()));
        if(hash_reply == nullptr)
        {
            continue;
        }

        if(hash_reply->type != REDIS_REPLY_ARRAY || hash_reply->elements < 2)
        {
            freeReplyObject(hash_reply);
            continue;
        }

        ServiceInstance instance;
        instance.role = role;
        for(size_t i = 0; i + 1 < hash_reply->elements; i += 2)
        {
            auto* field_reply = hash_reply->element[i];
            auto* value_reply = hash_reply->element[i + 1];
            if(field_reply == nullptr || value_reply == nullptr ||
               field_reply->str == nullptr || value_reply->str == nullptr)
            {
                continue;
            }

            auto field = std::string(field_reply->str, field_reply->len);
            auto value = std::string(value_reply->str, value_reply->len);

            if(field == "role")
            {
                instance.role = value;
            }
            else if(field == "host")
            {
                instance.endpoint.host = value;
            }
            else if(field == "port")
            {
                try
                {
                    instance.endpoint.port = static_cast<uint16_t>(std::stoi(value));
                }
                catch(...)
                {
                    instance.endpoint.port = 0;
                }
            }
            else if(field == "weight")
            {
                try
                {
                    instance.weight = std::max(1, std::stoi(value));
                }
                catch(...)
                {
                    instance.weight = 1;
                }
            }
            else if(field == "instance_id")
            {
                instance.instance_id = value;
            }
        }

        if(instance.endpoint.port > 0)
        {
            if(instance.instance_id.empty())
            {
                instance.instance_id = safe_instance_id(instance);
            }
            out.push_back(instance);
        }

        freeReplyObject(hash_reply);
    }

    freeReplyObject(set_reply);
    return out;
}

bool RedisServiceDiscovery::ensure_connected()
{
    if(m_context != nullptr && m_context->err == 0)
    {
        return true;
    }

    if(m_context != nullptr)
    {
        redisFree(m_context);
        m_context = nullptr;
    }

    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 500 * 1000;

    m_context = redisConnectWithTimeout(m_config.host.c_str(), m_config.port, timeout);
    if(m_context == nullptr || m_context->err != 0)
    {
        spdlog::error("redis connect failed host={} port={} err={}",
                      m_config.host,
                      m_config.port,
                      m_context == nullptr ? "null context" : m_context->errstr);
        if(m_context != nullptr)
        {
            redisFree(m_context);
            m_context = nullptr;
        }
        return false;
    }

    if(!m_config.password.empty())
    {
        auto* auth_reply = static_cast<redisReply*>(redisCommand(m_context, "AUTH %s", m_config.password.c_str()));

        if(auth_reply == nullptr)
        {
            spdlog::error("redis auth command failed");
            redisFree(m_context);
            m_context = nullptr;
            return false;
        }

        const bool ok = auth_reply->type == REDIS_REPLY_STATUS && auth_reply->str != nullptr && std::string(auth_reply->str) == "OK";
        freeReplyObject(auth_reply);
        if(!ok)
        {
            spdlog::error("redis auth failed");
            redisFree(m_context);
            m_context = nullptr;
            return false;
        }
    }

    if(m_config.db > 0)
    {
        auto* select_reply = static_cast<redisReply*>(redisCommand(m_context, "SELECT %d", m_config.db));
        if(select_reply == nullptr)
        {
            spdlog::error("redis select db failed");
            redisFree(m_context);
            m_context = nullptr;
            return false;
        }

        const bool ok = select_reply->type == REDIS_REPLY_STATUS && select_reply->str != nullptr && std::string(select_reply->str) == "OK";
        freeReplyObject(select_reply);
        if(!ok)
        {
            spdlog::error("redis select db={} rejected", m_config.db);
            redisFree(m_context);
            m_context = nullptr;
            return false;
        }
    }

    return true;
}

bool RedisServiceDiscovery::set_instance_hash(const ServiceInstance& instance)
{
    auto instance_key = make_instance_key(instance);
    auto instance_id = safe_instance_id(instance);

    auto* reply = static_cast<redisReply*>(redisCommand(m_context,
                                                        "HSET %s role %s host %s port %u weight %d instance_id %s",
                                                        instance_key.c_str(),
                                                        instance.role.c_str(),
                                                        instance.endpoint.host.c_str(),
                                                        static_cast<unsigned>(instance.endpoint.port),
                                                        std::max(1, instance.weight),
                                                        instance_id.c_str()));
    if(reply == nullptr)
    {
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool RedisServiceDiscovery::expire_instance(const ServiceInstance& instance)
{
    auto instance_key = make_instance_key(instance);
    auto* reply = static_cast<redisReply*>(redisCommand(m_context, "EXPIRE %s %d", instance_key.c_str(), m_config.ttl_sec));
    if(reply == nullptr)
    {
        return false;
    }

    const bool ok = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    freeReplyObject(reply);
    return ok;
}

std::string RedisServiceDiscovery::make_instance_key(const ServiceInstance& instance) const
{
    std::ostringstream output;
    output << m_config.key_prefix
           << ":"
           << instance.role
           << ":"
           << safe_instance_id(instance);
    return output.str();
}

std::string RedisServiceDiscovery::make_role_set_key(const std::string& role) const
{
    std::ostringstream output;
    output << m_config.key_prefix << ":role:" << role;
    return output.str();
}

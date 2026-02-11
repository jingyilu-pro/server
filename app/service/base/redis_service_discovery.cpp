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

RedisServiceDiscovery::RedisServiceDiscovery(const RedisConfig& config)
    : m_config(config)
{
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

bool RedisServiceDiscovery::register_instance(const ServiceInstance& instance)
{
    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return false;
    }

    if(!set_instance_hash(instance) || !expire_instance(instance))
    {
        return false;
    }

    auto role_set_key = make_role_set_key(instance.role);
    auto instance_key = make_instance_key(instance);
    auto* reply = static_cast<redisReply*>(redisCommand(m_context,
                                                        "SADD %s %s",
                                                        role_set_key.c_str(),
                                                        instance_key.c_str()));
    if(reply == nullptr)
    {
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool RedisServiceDiscovery::heartbeat(const ServiceInstance& instance)
{
    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return false;
    }

    return set_instance_hash(instance) && expire_instance(instance);
}

std::vector<ServiceInstance> RedisServiceDiscovery::list_instances(const std::string& role)
{
    std::vector<ServiceInstance> out;

    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return out;
    }

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
            const auto* key = hash_reply->element[i];
            const auto* value = hash_reply->element[i + 1];
            if(key == nullptr || value == nullptr || key->str == nullptr || value->str == nullptr)
            {
                continue;
            }

            const std::string field(key->str, key->len);
            const std::string field_value(value->str, value->len);
            if(field == "host")
            {
                instance.endpoint.host = field_value;
            }
            else if(field == "port")
            {
                try
                {
                    instance.endpoint.port = static_cast<uint16_t>(std::stoi(field_value));
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
                    instance.weight = std::max(1, std::stoi(field_value));
                }
                catch(...)
                {
                    instance.weight = 1;
                }
            }
            else if(field == "instance_id")
            {
                instance.instance_id = field_value;
            }
        }

        freeReplyObject(hash_reply);
        if(instance.endpoint.port != 0)
        {
            if(instance.instance_id.empty())
            {
                instance.instance_id = safe_instance_id(instance);
            }
            out.push_back(instance);
        }
    }

    freeReplyObject(set_reply);
    return out;
}

bool RedisServiceDiscovery::unregister_instance(const ServiceInstance& instance)
{
    std::lock_guard lock(m_mutex);
    if(!ensure_connected())
    {
        return false;
    }

    auto role_set_key = make_role_set_key(instance.role);
    auto instance_key = make_instance_key(instance);

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
        return false;
    }
    freeReplyObject(del_reply);
    return true;
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

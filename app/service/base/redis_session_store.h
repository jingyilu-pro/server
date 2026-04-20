//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "session_store.h"

#include <hiredis/hiredis.h>

#include <memory>
#include <mutex>

class RedisSessionManager : public CoroManager
{
public:
    explicit RedisSessionManager(int worker_count);
    ~RedisSessionManager() override;

public:
    CoroResult* alloc() override;
};

class RedisSessionStore final : public ISessionStore
{
public:
    explicit RedisSessionStore(const RedisConfig& config);
    ~RedisSessionStore() override;

public:
    bool ready() const override;
    void poll() override;
    CoroAwaitable get_session(const std::string& account) override;
    CoroAwaitable upsert_session(const SessionRecord& session, int ttl_sec) override;
    CoroAwaitable touch_session(const std::string& account, int ttl_sec) override;
    CoroAwaitable remove_session(const std::string& account) override;

private:
    SessionStoreOpResult* alloc_result();
    void execute_operation(SessionStoreOpResult* result);
    bool ensure_connected(redisContext** context, std::string* error = nullptr);
    redisContext* ensure_worker_connection(std::string* error = nullptr);
    std::string make_session_key(const std::string& account) const;

private:
    RedisConfig m_config;
    redisContext* m_context = nullptr;
    bool m_ready = false;
    std::unique_ptr<RedisSessionManager> m_manager;
    mutable std::mutex m_mutex;
};

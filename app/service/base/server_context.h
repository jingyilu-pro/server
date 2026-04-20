//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "application_config.h"
#include "account_cache_store.h"
#include "account_repository.h"
#include "mud_event_store.h"
#include "mud_player_repository.h"
#include "session_store.h"
#include "service_discovery.h"
#include "token_provider.h"

#include <memory>

struct ServerContext
{
    bool ready = false;
    std::string error;

    std::shared_ptr<IServiceDiscovery> manager_discovery;
    std::shared_ptr<IServiceDiscovery> login_discovery;
    std::shared_ptr<IServiceDiscovery> game_discovery;

    std::shared_ptr<IAccountCacheStore> login_account_cache_store;
    std::shared_ptr<ISessionStore> login_session_store;
    std::shared_ptr<ISessionStore> game_session_store;

    std::shared_ptr<IAccountRepository> login_account_repository;
    std::shared_ptr<IMudPlayerRepository> game_mud_player_repository;
    std::shared_ptr<IMudEventStore> game_mud_event_store;

    std::shared_ptr<ITokenProvider> login_token_provider;
    std::shared_ptr<ITokenProvider> game_token_provider;
};

ServerContext create_server_context(const RuntimeConfig& config,
                                    bool require_manager_discovery,
                                    bool require_login_repository,
                                    bool require_game_repository);

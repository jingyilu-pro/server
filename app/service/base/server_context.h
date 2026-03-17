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
#include "account_cache_store.h"
#include "account_repository.h"
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

    std::shared_ptr<ITokenProvider> login_token_provider;
    std::shared_ptr<ITokenProvider> game_token_provider;
};

ServerContext create_server_context(const RuntimeConfig& config,
                                    bool require_manager_discovery,
                                    bool require_login_repository,
                                    bool require_game_repository);

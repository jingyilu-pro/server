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

#include "server_context.h"

#include "jwt_token_provider.h"
#include "mysql_account_repository.h"
#include "mud_player_repository.h"
#include "noop_account_cache_store.h"
#include "noop_session_store.h"
#include "redis_account_cache_store.h"
#include "redis_service_discovery.h"
#include "redis_session_store.h"

#include "log/glogger.h"

ServerContext create_server_context(const RuntimeConfig& config,
                                    bool require_manager_discovery,
                                    bool require_login_repository,
                                    bool require_game_repository)
{
    ServerContext context;

    if(require_manager_discovery)
    {
        context.manager_discovery = std::make_shared<RedisServiceDiscovery>(config.redis);
    }
    else
    {
        context.manager_discovery = std::make_shared<NoopServiceDiscovery>();
    }

    auto login_discovery = std::make_shared<RedisServiceDiscovery>(config.redis);
    auto game_discovery = std::make_shared<RedisServiceDiscovery>(config.redis);
    context.login_discovery = login_discovery;
    context.game_discovery = game_discovery;

    context.login_account_cache_store = std::make_shared<RedisAccountCacheStore>(config.redis);
    context.login_session_store = std::make_shared<RedisSessionStore>(config.redis);
    context.game_session_store = std::make_shared<RedisSessionStore>(config.redis);

    std::shared_ptr<IAccountRepository> mysql_repository;
    if(require_login_repository)
    {
        mysql_repository = std::make_shared<MySqlAccountRepository>(config.mysql);
    }
    if(require_game_repository)
    {
        context.game_mud_player_repository = std::make_shared<MySqlMudPlayerRepository>(config.mysql);
    }

    if(context.login_discovery == nullptr || !context.login_discovery->ready())
    {
        spdlog::warn("login redis discovery unavailable, fallback to noop discovery");
        context.login_discovery = std::make_shared<NoopServiceDiscovery>();
    }
    if(context.game_discovery == nullptr || !context.game_discovery->ready())
    {
        spdlog::warn("game redis discovery unavailable, fallback to noop discovery");
        context.game_discovery = std::make_shared<NoopServiceDiscovery>();
    }
    if(context.login_account_cache_store == nullptr || !context.login_account_cache_store->ready())
    {
        spdlog::warn("redis account cache unavailable, fallback to noop account cache store");
        context.login_account_cache_store = std::make_shared<NoopAccountCacheStore>();
    }
    if(context.login_session_store == nullptr || !context.login_session_store->ready())
    {
        spdlog::warn("login redis session store unavailable, fallback to noop session store");
        context.login_session_store = std::make_shared<NoopSessionStore>();
    }
    if(context.game_session_store == nullptr || !context.game_session_store->ready())
    {
        spdlog::warn("game redis session store unavailable, fallback to noop session store");
        context.game_session_store = std::make_shared<NoopSessionStore>();
    }

    if(require_login_repository)
    {
        context.login_account_repository = std::move(mysql_repository);
    }
    context.login_token_provider = std::make_shared<JwtTokenProvider>(config.jwt);
    context.game_token_provider = context.login_token_provider;

    if(require_manager_discovery && (context.manager_discovery == nullptr || !context.manager_discovery->ready()))
    {
        context.ready = false;
        context.error = "manager redis discovery not ready";
        return context;
    }
    if(require_login_repository &&
       (context.login_account_repository == nullptr || !context.login_account_repository->ready()))
    {
        context.ready = false;
        context.error = "mysql account repository not ready";
        return context;
    }
    if(require_game_repository &&
       (context.game_mud_player_repository == nullptr || !context.game_mud_player_repository->ready()))
    {
        context.ready = false;
        context.error = "mysql mud player repository not ready";
        return context;
    }
    if(context.login_token_provider == nullptr || context.game_token_provider == nullptr)
    {
        context.ready = false;
        context.error = "token provider not ready";
        return context;
    }

    context.ready = true;
    return context;
}

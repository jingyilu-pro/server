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
#include "redis_service_discovery.h"

ServerContext create_server_context(const RuntimeConfig& config)
{
    ServerContext context;

    context.manager_discovery = std::make_shared<RedisServiceDiscovery>(config.redis);
    context.login_discovery = std::make_shared<RedisServiceDiscovery>(config.redis);
    context.game_discovery = std::make_shared<RedisServiceDiscovery>(config.redis);
    context.login_account_repository = std::make_shared<MySqlAccountRepository>(config.mysql);
    context.login_token_provider = std::make_shared<JwtTokenProvider>(config.jwt);
    context.game_token_provider = context.login_token_provider;

    if(context.manager_discovery == nullptr || !context.manager_discovery->ready())
    {
        context.ready = false;
        context.error = "manager redis discovery not ready";
        return context;
    }
    if(context.login_discovery == nullptr || !context.login_discovery->ready())
    {
        context.ready = false;
        context.error = "login redis discovery not ready";
        return context;
    }
    if(context.game_discovery == nullptr || !context.game_discovery->ready())
    {
        context.ready = false;
        context.error = "game redis discovery not ready";
        return context;
    }
    if(context.login_account_repository == nullptr || !context.login_account_repository->ready())
    {
        context.ready = false;
        context.error = "mysql account repository not ready";
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


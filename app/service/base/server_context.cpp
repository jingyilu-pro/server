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

#include "memory_account_repository.h"
#include "mysql_account_repository.h"
#include "redis_service_discovery.h"
#include "jwt_token_provider.h"

#include "log/glogger.h"

ServerContext create_server_context(const RuntimeConfig& config)
{
    ServerContext context;

    context.discovery = std::make_shared<RedisServiceDiscovery>(config.redis);

    auto mysql_repository = std::make_shared<MySqlAccountRepository>(config.mysql);
    if(mysql_repository->ready())
    {
        context.account_repository = mysql_repository;
    }
    else
    {
        spdlog::warn("mysql repository unavailable, fallback to in-memory account repository");
        context.account_repository = std::make_shared<MemoryAccountRepository>();
    }

    context.token_provider = std::make_shared<JwtTokenProvider>(config.jwt);

    return context;
}


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

#include "game_service.h"

#include "protocol/gateway.pb.h"

GameService::GameService(const RuntimeConfig& config)
    : BasicHttpService("game", config.server.game)
{
    register_handler("/v1/game/enter", [](evhttp_request* request) {
        gateway::GameEnterRequest game_request;
        auto body = read_request_body(request);
        if(!game_request.ParseFromString(body))
        {
            evhttp_send_error(request, 400, "invalid protobuf");
            return;
        }

        auto token = extract_authorization_token(request);
        gateway::GameEnterResponse response;
        if(token.empty())
        {
            response.set_code(401);
            response.set_message("missing jwt");
        }
        else
        {
            response.set_code(0);
            response.set_message("welcome " + game_request.account());
        }

        write_protobuf_response(request, response, 200);
    });
}

GameService::~GameService() = default;


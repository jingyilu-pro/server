//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "client_pressure_types.h"

const char* stage_name(StageType stage)
{
    switch(stage)
    {
    case StageType::manager:
        return "manager";
    case StageType::login:
        return "login";
    case StageType::game:
        return "game";
    }
    return "unknown";
}


//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "define.h"
#include <set>
#include "corocoroutine.h"
#include "coromanager.h"
#include "maskword_result.h"

class MaskWordService;

class MaskWordManager : public CoroManager
{
public:
    explicit MaskWordManager(MaskWordService* service, int thread_count = 1);

    ~MaskWordManager() override;

    CoroAwaitable awaitable(const string& mask_word);

public:
    CoroResult* alloc() override
    {
        expand<MaskWordResult>();
        return inner_alloc();
    }

private:
    PROPERTY(MaskWordService*, service)
};

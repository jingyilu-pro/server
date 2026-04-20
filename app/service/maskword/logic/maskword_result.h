//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#pragma once

#include "cororesult.h"
#include "define.h"

class MaskWordResult final : public CoroResult
{
public:
    MaskWordResult();

    ~MaskWordResult() override;

    void init(const string& str);

    void worker() override;

    void clear() override;

private:
    // data
    PROPERTY(bool, mask);

    PROPERTY_REF(string, str);

    PROPERTY_REF(string, mask_word);
};

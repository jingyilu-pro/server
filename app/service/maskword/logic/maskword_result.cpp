//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "maskword_result.h"

MaskWordResult::MaskWordResult()
    : m_mask(false)
{
}

MaskWordResult::~MaskWordResult() = default;

void MaskWordResult::init(const string& str)
{
    set_str(str);
    set_mask_word(str);
    set_mask(false);
}

void MaskWordResult::worker()
{
    static int64 sid = 0;
    set_mask(++sid % 3 == 0);
    if(mask())
    {
        // std::cout << "thread=" << std::this_thread::get_id() << " str=" << str() << " is maskword" << std::endl;
    }
    // gwarn("------->>>>>>>>>>>>>origin str:{} mask_word:{} mask:{}", str(), mask_word(), mask());
}

void MaskWordResult::clear()
{
    m_str.clear();
    m_mask_word.clear();
}
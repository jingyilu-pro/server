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


#include "maskword_result.h"


MaskWordResult::MaskWordResult()
    : m_mask(false) {
}

MaskWordResult::~MaskWordResult() = default;

void MaskWordResult::init(const string &str) {
    set_str(str);
    set_mask_word(str);
    set_mask(false);
}

void MaskWordResult::worker() {
    static int64 sid = 0;
    set_mask(++sid % 3 == 0);
    if (mask()) {
        // std::cout << "thread=" << std::this_thread::get_id() << " str=" << str() << " is maskword" << std::endl;
    }
    // gwarn("------->>>>>>>>>>>>>origin str:{} mask_word:{} mask:{}", str(), mask_word(), mask());
}

void MaskWordResult::clear() {
    m_str.clear();
    m_mask_word.clear();
}
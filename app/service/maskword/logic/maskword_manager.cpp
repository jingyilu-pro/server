//
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 JingyiLu
//

#include "maskword_manager.h"

MaskWordManager::MaskWordManager(MaskWordService* service, int thread_count /* = 1*/)
    : CoroManager(thread_count), m_service(service)
{
    CoroManager::init();
}

MaskWordManager::~MaskWordManager() = default;

CoroAwaitable MaskWordManager::awaitable(const string& mask_word)
{
    // 会自动回收对象
    auto* result = dynamic_cast<MaskWordResult*>(alloc());
    result->init(mask_word);
    return CoroAwaitable{this, result};
}

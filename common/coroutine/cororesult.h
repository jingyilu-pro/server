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


#pragma once

#include "define.h"
#include "corocoroutine.h"

class CoroResult
{
public:
    CoroResult() : m_uid{0}, m_valid{true}, m_handle(nullptr), m_alloc_time(0) {}
    virtual ~CoroResult() = default;

    virtual void worker() = 0;
    virtual void clear() = 0;
public:
    void resume()
    {
        if(!valid()) return;
        set_valid(false);
        if(m_handle)
            m_handle.resume();
    }

    void destroy()
    {
        if(!valid()) return;
        set_valid(false);
        if(m_handle)
            m_handle.destroy();
    }
private:
    PROPERTY(uint64, uid);
    PROPERTY(bool, valid);
    PROPERTY(coro_handle, handle);
    PROPERTY(time_t, alloc_time)
};


// for example
class TestCoroResult : public CoroResult
{
public:
    virtual void worker();
    virtual void clear() {}

    void init(const string& mask_word) { m_mask_word = mask_word; }
private:
    string m_mask_word;
};

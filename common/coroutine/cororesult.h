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

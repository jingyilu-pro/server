#pragma once

#include "define.h"
#include "corocoroutine.h"
#include "cororesult.h"


class CheckMaskWordResult : public CoroResult
{
public:
    CheckMaskWordResult();
    virtual ~CheckMaskWordResult();

    void init(const string& str);

    virtual void worker();
    virtual void clear();

private:
    // data
    PROPERTY(bool, mask);
    PROPERTY_REF(string, str);
    PROPERTY_REF(string, mask_word);
};


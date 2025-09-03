#include "checkmaskwordresult.h"
#include <iostream>


CheckMaskWordResult::CheckMaskWordResult()
    : m_mask(false)
{

}

CheckMaskWordResult::~CheckMaskWordResult()
{

}

void CheckMaskWordResult::init(const string& str)
{
    set_str(str);
    set_mask_word(str);
    set_mask(false);
}

void CheckMaskWordResult::worker()
{
    static int64 sid = 0;
    set_mask(++sid % 3 == 0);
    if(mask())
    {
        // std::cout << "thread=" << std::this_thread::get_id() << " str=" << str() << " is maskword" << std::endl;
    }
    // gwarn("------->>>>>>>>>>>>>origin str:{} mask_word:{} mask:{}", str(), mask_word(), mask());
}

void CheckMaskWordResult::clear()
{
    m_str.clear();
    m_mask_word.clear();
}


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

#include <vector>
#include <string>

using namespace std;

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

/* x=target variable, y=mask */
#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK_ALL(x,y) (!(~(x) & (y)))
#define BITMASK_CHECK_ANY(x,y) ((x) & (y))


void skip_special_symbol(string& src_str, const string& pattern)
{
    // string s = "&^";
    // string str = "gd^g@|GD&DGH";
    auto pos = src_str.find_first_of(pattern, 0);
    while (pos != string::npos)
    {
        src_str.erase(pos, 1);
        pos = src_str.find_first_of(pattern, pos);
    }
    cout << "src_str=" << src_str << endl;
}


std::vector<std::string> split(const string& src_str, const char* pattern)
{
    std::vector<std::string> result;

    size_t first_pos = 0;
    size_t pos = src_str.find_first_of(pattern, first_pos);
    
    while(pos != string::npos)
    {
        // cout << "pos=" << pos  << " first_pos=" << first_pos << endl;
        result.push_back(src_str.substr(first_pos, pos - first_pos));
        first_pos = pos + 1;
        pos = src_str.find_first_of(pattern, first_pos);
    }

    if(src_str.length() > first_pos)
    {
        result.push_back(src_str.substr(first_pos, src_str.length() - first_pos));
    }

    return result;
}


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


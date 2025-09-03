#pragma once

#ifdef __APPLE__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <unistd.h>
#include <thread>
#include <string>

using namespace std;
// using namespace std::experimental;


/*!
    uint64 is a typedef for an unsigned integer that is exactly 64 bits wide.
    uint32 is a typedef for an unsigned integer that is exactly 32 bits wide.
    uint16 is a typedef for an unsigned integer that is exactly 16 bits wide.
    uint8  is a typedef for an unsigned integer that is exactly 8  bits wide.

    int64 is a typedef for an integer that is exactly 64 bits wide.
    int32 is a typedef for an integer that is exactly 32 bits wide.
    int16 is a typedef for an integer that is exactly 16 bits wide.
    int8  is a typedef for an integer that is exactly 8  bits wide.
!*/

#define COMPILE_TIME_ASSERT(expression) static_assert(expression, "Failed assertion")
#ifdef __GNUC__
typedef unsigned long long uint64;
typedef long long int64;
#elif defined(__BORLANDC__)
typedef unsigned __int64 uint64;
typedef __int64 int64;
#elif defined(_MSC_VER)
typedef unsigned __int64 uint64;
typedef __int64 int64;
#else
typedef unsigned long long uint64;
typedef long long int64;
#endif

typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned char  uint8;

typedef short int16;
typedef int   int32;
typedef char  int8;

typedef int   SOCKET;

// make sure these types have the right sizes on this platform
COMPILE_TIME_ASSERT(sizeof(uint8)  == 1);
COMPILE_TIME_ASSERT(sizeof(uint16) == 2);
COMPILE_TIME_ASSERT(sizeof(uint32) == 4);
COMPILE_TIME_ASSERT(sizeof(uint64) == 8);

COMPILE_TIME_ASSERT(sizeof(int8)  == 1);
COMPILE_TIME_ASSERT(sizeof(int16) == 2);
COMPILE_TIME_ASSERT(sizeof(int32) == 4);
COMPILE_TIME_ASSERT(sizeof(int64) == 8);


#define SAFE_DELETE(x) { if (x) { delete (x); (x) = NULL; } }

#define PROPERTY(type, name) \
public:\
    type name() { return m_##name; };\
    void set_##name(type name) { m_##name = name; };\
private:\
    type m_##name;

#define PROPERTY_REF(type, name) \
public:\
    type& name() { return m_##name; };\
    void set_##name(const type& name) { m_##name = name; };\
private:\
    type m_##name;

#define PROPERTY_PTR(type, name) \
public:\
    type* name() { return m_##name; };\
    void set_##name(type* name) { m_##name = name; };\
private:\
    type* m_##name;


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

/* Numeric representation of the version */
#define DEMO_NUMERIC_VERSION 0x01010001
#define DEMO_PACKAGE_VERSION "1.1.0"

#define DEMO_VERSION_MAJOR 1
#define DEMO_VERSION_MINOR 1
#define DEMO_VERSION_PATCH 0

/* Version number of package */
#define DEMO_VERSION "1.1.0-alpha-dev"

/* Name of package */
#define DEMO_PACKAGE "demo"


const char* get_version(void)
{
	return (DEMO_VERSION);
}

uint32_t get_version_number(void)
{
	return (DEMO_NUMERIC_VERSION);
}

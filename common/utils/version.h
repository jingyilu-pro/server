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

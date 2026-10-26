#pragma once
#include <stdint.h>
#include <stddef.h>
#define MALLOC_CAP_INTERNAL 1U
#define MALLOC_CAP_8BIT 4U
#define MALLOC_CAP_SPIRAM 16U
size_t heap_caps_get_largest_free_block(uint32_t capabilities);

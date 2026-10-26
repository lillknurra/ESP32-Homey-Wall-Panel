#pragma once
#include <stddef.h>
#define MALLOC_CAP_INTERNAL 1U
size_t heap_caps_get_free_size(unsigned caps);
size_t heap_caps_get_largest_free_block(unsigned caps);
size_t heap_caps_get_minimum_free_size(unsigned caps);

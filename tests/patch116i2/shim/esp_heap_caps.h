#pragma once
#include <stddef.h>
#define MALLOC_CAP_INTERNAL 1U
size_t heap_caps_get_free_size(unsigned caps);

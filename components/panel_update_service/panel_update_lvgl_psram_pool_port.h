#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool panel_update_i5_psram_ready(void);
void *panel_update_i5_aligned_alloc(size_t alignment, size_t bytes, uint32_t capabilities);
void panel_update_i5_free(void *ptr);
bool panel_update_i5_is_external_ram(const void *ptr);
_Noreturn void panel_update_i5_fatal(const char *reason);

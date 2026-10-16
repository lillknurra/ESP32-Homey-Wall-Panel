#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "panel_rgb_diag_model.h"

/* ISR hooks use only IRAM code, internal DRAM and a short ISR-safe spinlock. */
bool panel_render_diag_eof(void);
void panel_render_diag_bounce_sample(uint32_t elapsed_us);
void panel_render_diag_vsync(void);
void panel_render_diag_decision(uint32_t actual, uint32_t expected,
                                bool requested, bool policy, bool restart);
void panel_render_diag_restart_requested(void);
void panel_render_diag_restarted(void);
void panel_render_diag_flash_interval(uint64_t start_us, uint32_t elapsed_us, int result);
void panel_render_diag_alloc_failure(size_t bytes, uint32_t caps);
void panel_render_diag_start(void);

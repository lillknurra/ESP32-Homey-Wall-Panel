#!/usr/bin/env python3
"""Hash-pinned, insertion-only instrumentation; never modify installed IDF."""
import argparse
import hashlib
from pathlib import Path
SOURCE_SHA256 = 'da391ef23c849e492db12bba8a19ab81934915396b9ee78d148e26a37a4edaa9'

def instrument(raw):
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA256:
        raise ValueError('expected unmodified ESP-IDF v6.0.1 RGB source')
    text = raw.decode()
    def insert(anchor, addition, before=False):
        nonlocal text
        if text.count(anchor) != 1:
            raise ValueError('instrumentation anchor is not unique')
        block = '\n/* V32_BEGIN */\n' + addition + '\n/* V32_END */\n'
        text = text.replace(anchor, block + anchor if before else anchor + block)
    insert('#include "esp_lcd_panel_rgb.h"',
           '#include "panel_render_diag.h"\n#include "esp_timer.h"')
    insert('    rgb_panel->flags.need_restart = true;',
           '    panel_render_diag_restart_requested();')
    insert('        rgb_panel->bb_eof_count++;',
           '        const bool diag_sample = panel_render_diag_eof();')
    fill = '        need_yield = lcd_rgb_panel_fill_bounce_buffer(rgb_panel, rgb_panel->bounce_buffer[bb]);'
    insert(fill, '        const uint64_t diag_bounce_start = diag_sample ?\n'
                 '            (uint64_t)esp_timer_get_time() : 0U;', before=True)
    insert(fill, '        if (diag_sample) panel_render_diag_bounce_sample(\n'
                 '            (uint32_t)((uint64_t)esp_timer_get_time() - diag_bounce_start));')
    insert('#if CONFIG_LCD_RGB_RESTART_IN_VSYNC\n    do_restart = true;',
           '    panel_render_diag_decision(panel->bb_eof_count, panel->expect_eof_count,\n'
           '                                false, true, do_restart);')
    insert('    if (panel->flags.need_restart) {',
           '    const bool diag_requested = panel->flags.need_restart;', before=True)
    insert('    panel->bb_eof_count = 0;',
           '    panel_render_diag_decision(panel->bb_eof_count, panel->expect_eof_count,\n'
           '                                diag_requested, false, do_restart);', before=True)
    insert('\n}\n\nstatic void lcd_rgb_panel_start_transmission(esp_rgb_panel_t *rgb_panel)',
           '    panel_render_diag_restarted();', before=True)
    insert('    if (intr_status & LCD_LL_EVENT_VSYNC_END) {',
           '        panel_render_diag_vsync();')
    return text

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    if a.source.resolve() == a.output.resolve():
        raise ValueError('refuse overwriting IDF source')
    text = instrument(a.source.read_bytes())
    a.output.parent.mkdir(parents=True, exist_ok=True)
    if not a.output.exists() or a.output.read_text() != text:
        a.output.write_text(text)
    print(f'original_sha256={SOURCE_SHA256} instrumented_sha256={hashlib.sha256(text.encode()).hexdigest()}')
if __name__ == '__main__':
    main()

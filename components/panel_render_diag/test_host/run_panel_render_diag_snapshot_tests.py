#!/usr/bin/env python3
from pathlib import Path
import argparse, subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
args.output_dir.mkdir(parents=True, exist_ok=False)
component = Path(__file__).resolve().parents[1]
binary = args.output_dir / 'snapshot_test'
test = component / 'test_host/test_panel_render_diag_snapshot.c'
subprocess.run([
    'cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pthread',
    '-fsanitize=address,undefined',
    '-I', component / 'test_host/include', '-I', component / 'include',
    test, '-o', binary,
], check=True)
subprocess.run([binary], check=True)
source = (component / 'panel_render_diag.c').read_text(encoding='utf-8')
start = source.index('bool panel_render_diag_snapshot(')
end = source.index('\nstatic void report_task(', start)
body = source[start:end]
assert body.index('portENTER_CRITICAL') < body.index('*out = s_state.metrics') < body.index('portEXIT_CRITICAL')
assert body.index('portEXIT_CRITICAL') < body.index('esp_timer_get_time')
assert not any(token in body for token in ('snprintf(', 'calloc(', 'malloc(', 'panel_diag_pop('))
print('PANEL_RENDER_DIAG_SNAPSHOT_SOURCE_INVARIANTS PASS')

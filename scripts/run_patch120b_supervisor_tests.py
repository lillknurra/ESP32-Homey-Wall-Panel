#!/usr/bin/env python3
"""Compile and execute the production maintenance supervisor with host task shims."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SHIMS = ROOT / 'tests/patch120b/stubs'
INCLUDES = ROOT / 'components/secure_bootstrap/include'

with tempfile.TemporaryDirectory(prefix='patch120b-supervisor-') as temp:
    exe = Path(temp) / 'supervisor'
    subprocess.run([
        'cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
        '-fsanitize=address,undefined', '-I', str(SHIMS), '-I', str(INCLUDES),
        '-I', str(ROOT / 'main'),
        str(ROOT / 'main/panel_ota_supervisor.c'),
        str(ROOT / 'components/secure_bootstrap/panel_ota_handoff.c'),
        str(ROOT / 'tests/patch120b/test_supervisor.c'), '-o', str(exe),
    ], check=True)
    subprocess.run([str(exe)], check=True)

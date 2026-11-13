#!/usr/bin/env python3
"""Compile the production RTC one-shot handoff against minimal IDF host shims."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / 'tests/patch120b/test_handoff.c'
SHIMS = ROOT / 'tests/patch120b/stubs'
INCLUDES = ROOT / 'components/secure_bootstrap/include'

with tempfile.TemporaryDirectory(prefix='patch120b-handoff-') as temp:
    exe = Path(temp) / 'handoff'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
                    '-I', str(SHIMS), '-I', str(INCLUDES), str(TEST), '-o', str(exe)],
                   check=True)
    subprocess.run([str(exe)], check=True)

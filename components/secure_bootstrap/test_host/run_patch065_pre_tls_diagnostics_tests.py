#!/usr/bin/env python3
"""Exercise the production wrappers with exact-call scripted IDF/lwIP doubles."""
from pathlib import Path
import json
import subprocess
import tempfile
root = Path(__file__).resolve().parents[3]
c = root / 'components/secure_bootstrap'
with tempfile.TemporaryDirectory() as tmp:
    exe = Path(tmp) / 'test'
    subprocess.run(['cc', '-std=c11', '-D_DEFAULT_SOURCE', '-DPATCH065_HOST_TEST',
                    '-Wall', '-Wextra', '-Werror', '-pedantic', '-I', str(c/'include'),
                    '-I', str(c/'test_host'), str(c/'athom_pre_tls_diag.c'),
                    str(c/'test_host/test_patch065_pre_tls_diagnostics.c'), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], capture_output=True, text=True, check=True)
    sample = json.loads(next(l[5:] for l in result.stdout.splitlines() if l.startswith('JSON=')))
    assert all(type(v) in (int, bool) for k,v in sample.items() if k != 'result')
    assert sample['result'] == 'tls_connected'
    assert not any(k in sample for k in ('host','hostname','url','address','fd','token','id','digest'))
    assert sample['valid'] and sample['tcp_connected'] and sample['tls_setup_started']
print('PATCH065_FOCUSED_TESTS=PASS (production wrappers, errno, stages, correlation, passivity, privacy)')

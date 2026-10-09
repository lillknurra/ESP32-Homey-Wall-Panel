#!/usr/bin/env python3
"""Production wrappers with scripted existing calls; no network/device I/O."""
from pathlib import Path
import json
import subprocess
import tempfile
c = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temporary:
    exe = Path(temporary) / 'test'
    subprocess.run(['cc', '-std=c11', '-D_DEFAULT_SOURCE', '-DPATCH065_HOST_TEST',
                    '-Wall', '-Wextra', '-Werror', '-pedantic', '-I', str(c/'include'),
                    '-I', str(c/'test_host'), str(c/'test_host/test_patch066_handshake_progress.c'),
                    '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], check=True, text=True, capture_output=True)
    samples = [json.loads(line.split('=', 1)[1]) for line in result.stdout.splitlines() if line.startswith('CASE_')]
    assert len(samples) == 8
    allowed = {'valid','completed','call_count','step_count','state','last_ret','want_read_count',
               'want_write_count','send_calls','send_bytes','send_last_ret','send_last_errno',
               'recv_calls','recv_bytes','recv_last_ret','recv_last_errno','first_tx_ms',
               'first_rx_ms','last_progress_ms','peer_closed'}
    for sample in samples:
        h = sample['handshake']
        assert set(h) == allowed
        assert all(type(v) in (int, bool, type(None)) for k,v in h.items() if k != 'state')
        assert h['state'] in ('client_hello','server_hello','handshake_over')
    assert samples[0]['handshake']['first_tx_ms'] is None
    assert samples[1]['handshake']['first_rx_ms'] is None
    assert samples[2]['handshake']['last_progress_ms'] == 5
print('PATCH066_FOCUSED_TESTS=PASS (A-F, success, zero-length, errno, forwarding, correlation, reset, privacy)')

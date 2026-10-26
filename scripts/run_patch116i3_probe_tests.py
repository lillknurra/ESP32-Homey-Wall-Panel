#!/usr/bin/env python3
"""Patch116I3 mock-only. No ESP-IDF, sockets, hardware or Git mutation."""
from pathlib import Path
import os
import subprocess
import tempfile
import shutil

ROOT = Path(__file__).resolve().parent.parent
COMP = ROOT / 'components' / 'panel_update_service'
TESTS = ROOT / 'tests' / 'patch116i3'

def run(args, env=None):
    p = subprocess.run(args, capture_output=True, text=True, env=env)
    if p.returncode:
        raise AssertionError(f"FAILED {' '.join(map(str,args))}\n{p.stdout}\n{p.stderr}")
    return p.stdout

def test_compiler(cc, folder):
    base = ['-std=c11','-Wall','-Wextra','-Werror','-pedantic','-O1','-g','-fno-omit-frame-pointer']
    inc = ['-I'+str(COMP/'include')]
    common = [str(COMP/'panel_update_i3_probe_policy.c'),str(TESTS/'test_probe_policy.c')]
    out = folder / f'policy_{cc}'
    print(run([cc,*base,*inc,*common,'-o',str(out)]).strip())
    print(run([str(out)]).strip())
    for sanitize in ('undefined', 'address,undefined'):
        san = ['-fsanitize='+sanitize]
        out = folder / f'policy_{cc}_{sanitize.replace(",","_")}'
        run([cc,*base,*inc,*san,*common,'-o',str(out)])
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
        env['UBSAN_OPTIONS'] = 'halt_on_error=1'
        print(run([str(out)],env).strip(), f'({cc} {sanitize})')
    off = folder / f'off_{cc}'
    run([cc,*base,*inc,str(COMP/'panel_update_i3_probe_esp.c'),
         str(TESTS/'test_default_off.c'),'-o',str(off)])
    run([str(off)])
    obj = folder / f'off_{cc}.o'
    run([cc,*base,*inc,'-c',str(COMP/'panel_update_i3_probe_esp.c'),'-o',str(obj)])
    undefined = run(['nm','-u',str(obj)]).strip()
    if undefined:
        raise AssertionError(f'default-off has external dependencies: {undefined}')
    print(cc,'default_off=PASS undefined_symbols=NONE')
    # Audit flags only -- active code needs real native ESP-IDF headers.
    # Missing explicit audit approval must fail before any ESP-IDF include.
    for flags in (['-DPANEL_UPDATE_I3_OBSERVER_ENABLED=1'],
                  ['-DPANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED=1']):
        p = subprocess.run([cc,*base,*inc,*flags,'-c',str(COMP/'panel_update_i3_probe_esp.c'),
                            '-o',str(folder/'should_fail.o')],capture_output=True,text=True)
        if p.returncode == 0:
            raise AssertionError('Unapproved active compile succeeded: '+str(flags))
    print(cc,'fail_closed_compile_gates=PASS')
    fake = TESTS / 'shim'
    probe = str(COMP/'panel_update_i3_probe_esp.c')
    policy = str(COMP/'panel_update_i3_probe_policy.c')
    mock = str(TESTS/'test_observer_adapter.c')
    for enabled in (False,True):
        active_flags = [
            '-DESP_PLATFORM=1', '-DPANEL_UPDATE_I3_OBSERVER_ENABLED=1',
            '-DPANEL_UPDATE_I3_AUDIT_ONLY=1'
        ]
        if enabled:
            active_flags.extend([
                '-DPANEL_UPDATE_I3_NETWORK_TRIAL_ENABLED=1',
                '-DPANEL_UPDATE_READONLY_WORKER_ENABLED=1',
                '-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1',
                '-DPANEL_UPDATE_WORKER_AUDIT_ONLY=1',
                '-DPANEL_UPDATE_HTTPS_AUDIT_ONLY=1'
            ])
        out = folder / f'observer_mock_{cc}_{int(enabled)}'
        run([cc,*base,*inc,'-I'+str(fake),*active_flags,probe,policy,mock,'-o',str(out)])
        print(run([str(out)]).strip(),f'({cc} audit observer trial={int(enabled)})')
    # The actual owner call-site hardcodes no operator authorization. Never
    # enable network at runtime by compile flags alone.
    owner = (ROOT/'components'/'secure_bootstrap'/'secure_bootstrap_esp.c').read_text()
    if '.operator_trial_approved = false' not in owner:
        raise AssertionError('Production owner is missing explicit runtime authorization barrier')
    # No firmware symbols or OTA calls in the new observer implementation.
    source = (COMP/'panel_update_i3_probe_esp.c').read_text()
    if 'monotonic_ms=%" PRIu64' not in source or 'sample.now_ms' not in source:
        raise AssertionError('Passive sample log is missing its monotonic timestamp')
    for forbidden in ('esp_ota_write(', 'esp_ota_begin(', 'esp_restart(', 'nvs_set_', 'athom_oauth_runtime_queue_'):
        if forbidden in source:
            raise AssertionError('Forbidden writer reference: '+forbidden)


with tempfile.TemporaryDirectory(prefix='p116i3_') as temp:
    d = Path(temp)
    for cc in ('gcc','clang'):
        if shutil.which(cc):
            test_compiler(cc, d)
print('PATCH116I3 HOST TESTS PASS')

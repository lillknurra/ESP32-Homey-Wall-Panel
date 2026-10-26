#!/usr/bin/env python3
"""Offline only: deterministic bounded owner/quarantine policy, default-off symbol audit.
No network, no OTA, no physical device access, no Git mutation.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import sys

root=Path(__file__).resolve().parents[1]
base=root/'components/panel_update_service'
tests=root/'tests/patch116i2'

with tempfile.TemporaryDirectory(prefix='patch116i2-worker-') as tmp:
    for cc in ('gcc','clang'):
        exe=Path(tmp)/('policy-'+cc)
        subprocess.run([cc,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer',
            '-I'+str(base/'include'),str(base/'panel_update_worker_policy.c'),
            str(tests/'test_worker_policy.c'),'-o',str(exe)],check=True)
        env=dict(os.environ,ASAN_OPTIONS=('detect_leaks=0:halt_on_error=1' if sys.platform == 'darwin' else 'detect_leaks=1:halt_on_error=1'),UBSAN_OPTIONS='halt_on_error=1')
        subprocess.run([str(exe)],check=True,env=env)
        disabled=Path(tmp)/('default-'+cc)
        subprocess.run([cc,'-std=c11','-O1','-Wall','-Wextra','-Werror',
            '-I'+str(base/'include'),str(base/'panel_update_worker_freertos.c'),
            str(tests/'test_default_off.c'),'-o',str(disabled)],check=True)
        subprocess.run([str(disabled)],check=True)
        nm=subprocess.check_output(['nm','-u',str(disabled)],text=True)
        forbidden=('esp_http_client','esp_ota_','esp_https_ota','xTaskCreate',
                   'esp_wifi','httpd_','esp_restart','nvs_')
        assert not any(x in nm for x in forbidden),nm
        # Fail closed if an audit-only native transport is enabled as production.
        bad=Path(tmp)/('bad-'+cc+'.o')
        r=subprocess.run([cc,'-std=c11','-I'+str(base/'include'),
            '-DPANEL_UPDATE_READONLY_WORKER_ENABLED=1',
            '-c',str(base/'panel_update_worker_freertos.c'),'-o',str(bad)],
            capture_output=True,text=True)
        assert r.returncode!=0 and 'requires ESP-IDF' in r.stderr,(r.returncode,r.stderr)
        # Verify the guarded native adapter against API-shaped test headers;
        # this is NOT a real ESP-IDF build or platform lifetime test.
        shim=tests/'shim'
        active=[cc,'-std=gnu11','-O1','-Wall','-Wextra','-Werror',
            '-fsyntax-only','-DESP_PLATFORM=1',
            '-DPANEL_UPDATE_READONLY_WORKER_ENABLED=1',
            '-DPANEL_UPDATE_WORKER_AUDIT_ONLY=1',
            '-DPANEL_UPDATE_MANIFEST_HTTPS_ENABLED=1',
            '-I'+str(shim),'-I'+str(base/'include'),
            str(base/'panel_update_worker_freertos.c')]
        subprocess.run(active,check=True)
        wrong=active.copy()
        wrong.remove('-DPANEL_UPDATE_WORKER_AUDIT_ONLY=1')
        bad_audit=subprocess.run(wrong,capture_output=True,text=True)
        assert bad_audit.returncode != 0 and 'compile-audit-only' in bad_audit.stderr
        print(f'COMPILER={cc} WORKER_POLICY_AND_DEFAULT_OFF_AND_SYNTAX=PASS')
print('PATCH116I2_OFFLINE_TESTS=PASS')

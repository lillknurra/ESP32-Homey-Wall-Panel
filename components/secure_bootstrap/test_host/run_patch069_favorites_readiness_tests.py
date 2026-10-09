#!/usr/bin/env python3
"""Real snapshot, Favorites, dashboard and UI model regression; no network."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
component = root / 'components/secure_bootstrap'
cjson = root / 'managed_components/espressif__cjson/cJSON'
with tempfile.TemporaryDirectory() as temporary:
    binary = Path(temporary) / 'test'
    subprocess.run([
        'cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
        '-I', str(component/'include'), '-I', str(cjson),
        *[str(component/name) for name in ('panel_homey_read_snapshot.c',
            'panel_homey_favorites.c', 'panel_homey_dashboard_binding.c', 'panel_ui_model.c')],
        str(cjson/'cJSON.c'), str(Path(__file__).with_name('test_patch069_favorites_readiness.c')),
        '-lm', '-o', str(binary)
    ], check=True, cwd=root)
    subprocess.run([str(binary)], check=True, cwd=root)
# GET serializes passive captured state. It cannot start any read chain.
runtime = (component/'athom_oauth_runtime.c').read_text()
start = runtime.index('static esp_err_t status_get(')
end = runtime.index('\nstatic ', start+1)
get = runtime[start:end]
for operation in ('fetch_inventory(', 'http_request', 'queue_inventory',
                  'connect_and_fetch', 'favorites_fetch', 'http_client_perform', 'xQueueSend'):
    assert operation not in get, operation
print('PATCH069_PASSIVE_GET_SOURCE_GATE PASS')

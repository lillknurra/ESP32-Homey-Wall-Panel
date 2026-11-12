#!/usr/bin/env python3
"""Compile the actual passive GET, query parser, classifier and event callback."""
from pathlib import Path
import re
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / 'components/secure_bootstrap'
CLIENT = (COMPONENT / 'athom_cloud_client.c').read_text()
RUNTIME = (COMPONENT / 'athom_oauth_runtime.c').read_text()
MODEL = (COMPONENT / 'athom_cloud_model.c').read_text()
HEADER = (COMPONENT / 'include/athom_cloud_client.h').read_text()


def function(source, signature):
    # Keep indices stable while ignoring braces in literals/comments.
    scrubbed = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
                      lambda match: ' ' * len(match[0]), source, flags=re.S)
    pos = 0
    while True:
        start = source.index(signature, pos)
        brace = scrubbed.index('{', start + len(signature))
        if ';' in scrubbed[start:brace]:
            pos = start + len(signature)
            continue
        depth = 0
        for index in range(brace, len(source)):
            depth += (scrubbed[index] == '{') - (scrubbed[index] == '}')
            if depth == 0:
                return source[start:index + 1]
        raise AssertionError('unterminated function')


status = function(RUNTIME, 'static esp_err_t status_get(httpd_req_t *r)')
guard_snapshot = function(
    RUNTIME,
    'bool athom_oauth_runtime_awning_guard_diagnostic_snapshot(')
guard_state_name = function(
    RUNTIME,
    'static const char *awning_guard_diagnostic_state_name(')
guard_handler = function(
    RUNTIME,
    'static esp_err_t awning_guard_get(httpd_req_t *request)')
for forbidden in ('esp_http_client_perform', 'queue_inventory', 'connect_and_fetch',
                  'network_phase_try_reserve', 'nvs_set_', 'diagnostic_publish(',
                  'runtime_diag_journal_record', 'http_request(', 'xTaskCreate'):
    assert forbidden not in status, forbidden
assert RUNTIME.count('"/homey/live-status"') == 1
route_table = RUNTIME.split('const httpd_uri_t handlers[]={', 1)[1].split('};', 1)[0]
assert '{"/homey/debug/awning-guard",HTTP_GET,awning_guard_get,NULL}' in route_table
assert route_table.count('awning_guard_get') == 1
guard_route = re.search(r'\{"/homey/debug/awning-guard",HTTP_GET,awning_guard_get,NULL\}', route_table)[0]
preselection = function(RUNTIME, 'static void preselection_restore_worker(void *arg)')
assert 'preselection_transport_observed(attempt, false)' in preselection
assert 'preselection_transport_observed(attempt, true)' in preselection
observe = function(RUNTIME, 'static esp_err_t preselection_transport_observed(')
assert observe.index('athom_inventory_attempt_diagnostic_begin(') < observe.index('athom_cloud_refresh(')
assert observe.index('athom_cloud_fetch_user_homeys(') < observe.index('athom_inventory_attempt_diagnostic_complete(')
assert 'return error;' in observe
assert 'athom_homey_diagnostic_status_json(' in status
assert 'athom_homey_status_json(' in status
assert 'athom_oauth_runtime_awning_guard_diagnostic_snapshot' not in status
assert 'awning_guard' not in status
assert 'panel_awning_write_guard_load' not in guard_snapshot
assert 'nvs_' not in guard_snapshot.lower()
for forbidden in ('nvs_', 'athom_cloud_', 'http_request', 'esp_http_client',
                  'queue_inventory', 'commission', 'write_guard_',
                  'queue_awning', 'refresh_inventory', 'xTaskCreate',
                  'httpd_req_recv'):
    assert forbidden not in guard_handler.lower(), forbidden
# Flags belong to the local response buffer and are copied only after this perform.
request = function(CLIENT, 'static esp_err_t http_request_limited(')
for event in ('connected', 'error', 'disconnected'):
    assert f's_transport_metrics.last_{event}_event_seen = buffer.{event}_event_seen;' in request
    assert request.index('esp_http_client_perform') < request.index(f'last_{event}_event_seen =')
assert request.index('last_connected_event_seen =') < request.index('esp_http_client_set_user_data(ctx->handle, NULL)')
for metric in ('cloud_client_init_count', 'homey_client_init_count'):
    assert metric in request

production = '\n\n'.join([
    re.search(r'typedef enum \{[^}]+\} athom_transport_class_t;', HEADER)[0],
    re.search(r'typedef struct \{\s*char \*data;[^}]+\} response_buffer_t;', CLIENT)[0],
    function(CLIENT, 'static athom_transport_class_t transport_classify('),
    function(CLIENT, 'const char *athom_cloud_transport_class_name('),
    function(CLIENT, 'static esp_err_t event_handler_impl(').replace(
        'event_handler_impl', 'event_handler'),
    function(MODEL, 'bool athom_homey_diagnostic_status_json('),
    function(RUNTIME, 'static bool live_status_diagnostic_query('),
    function(RUNTIME, 'static void athom_live_status_diagnostic_workspace_free('),
    guard_snapshot,
    guard_state_name,
    status,
    guard_handler,
])
template = Path(__file__).with_name('test_patch062_connect_diagnostics.c').read_text()
assert template.count('/* PATCH062_PRODUCTION */') == 1
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'test.c'
    binary = Path(directory) / 'test'
    source_text = template.replace('/* PATCH062_PRODUCTION */', production)
    assert source_text.count('/* PATCH119_ROUTE */') == 1
    source.write_text(source_text.replace('/* PATCH119_ROUTE */',
                         'static const httpd_uri_t test_guard_route = ' + guard_route + ';'))
    subprocess.run(['cc', '-pthread', '-I', str(COMPONENT/'include'), '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
                    str(source), '-o', str(binary)], check=True)
    completed = subprocess.run([str(binary)], check=False, text=True,
                               capture_output=True)
    print(completed.stdout, end='')
    if completed.returncode != 0:
        print(completed.stderr, end='')
        raise AssertionError(f'host test exit={completed.returncode}')
    live_line = next(line.split('=', 1)[1] for line in completed.stdout.splitlines()
                     if line.startswith('LIVE_STATUS_JSON='))
    live_payload = json.loads(live_line)
    assert 'awning_snapshot' in live_payload
    assert 'last_inventory_attempt_transport' in live_payload
    assert 'awning_guard' not in live_payload
    guard_lines = [line.split('=', 1)[1] for line in completed.stdout.splitlines()
                   if line.startswith('GUARD_ENDPOINT_JSON=')]
    guards = [json.loads(line) for line in guard_lines]
    assert len(guards) == 7
    assert guards[0] == {
        'schema_version': 1, 'state': 'ready', 'runtime_ready': True,
        'provenance': 'validated_runtime_snapshot'}
    assert [item['state'] for item in guards] == [
        'ready', 'ready', 'uninitialized', 'write_pending', 'quarantined', 'unknown', 'unknown']
    assert all(not item['runtime_ready'] for item in guards[2:])
    assert all(item['schema_version'] == 1 for item in guards)
    assert 'awning_guard' not in completed.stdout
    assert 'GUARD_ENDPOINT_JSON=' in completed.stdout
print('PATCH062_PASSIVE_SOURCE_CHAIN PASS')

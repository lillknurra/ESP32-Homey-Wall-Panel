#!/usr/bin/env python3
from pathlib import Path

p=Path('components/secure_bootstrap/athom_oauth_runtime.c').read_text()
policy=Path('components/secure_bootstrap/athom_restore_policy.c').read_text()

start=p.index('static void preselection_restore_worker(void *arg)')
end=p.index('\nstatic void maybe_start_preselection_restore_worker',start)
body=p[start:end]

wifi_start=p.index('esp_err_t athom_oauth_runtime_on_wifi_online(void)')
wifi_end=p.index('\nstatic void publish_cloud_state',wifi_start)
wifi_body=p[wifi_start:wifi_end]

gate_start=p.index('static void maybe_start_preselection_restore_worker(void)')
gate_end=p.index('\nstatic void auth_restore_worker',gate_start)
gate_body=p[gate_start:gate_end]

restore_start=p.index('static void auth_restore_worker(void *arg)')
restore_end=p.index('\nesp_err_t athom_oauth_runtime_register_handlers',restore_start)
restore_body=p[restore_start:restore_end]

assert 'athom_cloud_select_and_connect' not in body
assert 'connect_and_fetch_inventory' not in body
assert 'ATHOM_HOMEY_DATA_READY' not in body
assert 'homey_selection_required' in body
assert 'ATHOM_HOMEY_DATA_LOADING' in body
assert 'ATHOM_HOMEY_DATA_ERROR' in body
assert 'ATHOM_RESTORE_POLICY_RETRY_TRANSIENT' in body
assert 'ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS' in p
assert 'ATHOM_PRESELECT_RESTORE_MAX_ELAPSED_MS' in p
assert 's_preselection_restore_pending = true;' in restore_body
assert 'restore_preselection_homeys();' not in p

# Wi-Fi/auth two-condition gate.
assert 's_wifi_online = true;' in wifi_body
assert 'maybe_start_preselection_restore_worker();' in wifi_body
assert 'athom_restore_policy_should_start_preselection' in gate_body
assert 's_wifi_online' in gate_body
assert 's_restore_worker_running' in gate_body
assert 's_preselection_restore_pending' in gate_body
assert 's_preselection_restore_worker_running' in gate_body
assert 's_preselection_restore_pending = false;' in gate_body
assert 's_preselection_restore_worker_running = true;' in gate_body
assert 'xTaskCreate(preselection_restore_worker' in gate_body

# Pending is not consumed directly by auth restore; it is offered to the common gate
# only after restore_running is cleared.
clear_pos=restore_body.index('s_restore_worker_running = false;')
gate_pos=restore_body.index('maybe_start_preselection_restore_worker();')
assert clear_pos < gate_pos
assert 's_preselection_restore_pending = false;' not in restore_body
assert 'xTaskCreate(preselection_restore_worker' not in restore_body

# Gate decision is deterministic and requires both Wi-Fi online and restore complete.
assert 'return wifi_online && !restore_running && pending && !worker_running;' in policy

# No full readiness publication in pre-selection worker.
assert 's_homey_data_state = ATHOM_HOMEY_DATA_READY' not in body

# Causal PSRAM stack-placement experiment: placement only.
ps_gate_start=p.index('static void maybe_start_preselection_restore_worker(void)')
ps_gate_end=p.index('\nstatic void auth_restore_worker',ps_gate_start)
ps_gate=p[ps_gate_start:ps_gate_end]
ps_worker_start=p.index('static void preselection_restore_worker(void *arg)')
ps_worker_end=p.index('\nstatic void maybe_start_preselection_restore_worker',ps_worker_start)
ps_worker=p[ps_worker_start:ps_worker_end]
assert 'xTaskCreate(preselection_restore_worker, "athom_preselect",' in ps_gate
assert '12288, NULL, 5, NULL' in ps_gate
assert 'MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT' not in ps_gate
assert 'xTaskCreateWithCaps(preselection_restore_worker, "athom_preselect",' not in ps_gate
assert 'vTaskDelete(NULL);' in ps_worker
assert 'vTaskDeleteWithCaps(NULL);' not in ps_worker


# INTERNAL 12288 high-water causal probe invariants.
assert 'xTaskCreate(preselection_restore_worker, "athom_preselect",' in ps_gate
assert '12288, NULL, 5, NULL' in ps_gate
assert 'xTaskCreateWithCaps(preselection_restore_worker, "athom_preselect",' not in ps_gate
assert 'MALLOC_CAP_SPIRAM' not in ps_gate
assert 'vTaskDelete(NULL);' in ps_worker
assert 'vTaskDeleteWithCaps(NULL);' not in ps_worker
assert 'uxTaskGetStackHighWaterMark(NULL)' in p
assert 'HOMEY_PRESELECT_STACK phase=%s hwm_bytes=%u privacy=sanitized' in p
for phase in ('entry', 'after_discovery', 'before_refresh', 'after_refresh', 'before_delete'):
    assert f'preselection_stack_hwm_log("{phase}")' in ps_worker


# Refresh-buffer root-cause fix invariants.
cloud_src = (Path(__file__).resolve().parents[1] / 'athom_cloud_client.c').read_text(encoding='utf-8')
refresh_start = cloud_src.index('esp_err_t athom_cloud_refresh(athom_cloud_state_t *state)')
refresh_end = cloud_src.index('static esp_err_t parse_homeys', refresh_start)
refresh_body = cloud_src[refresh_start:refresh_end]
assert 'char encoded_refresh[ATHOM_TOKEN_MAX * 3U];' not in refresh_body
assert 'static bool url_encoded_length(const char *input, size_t *length_out)' in cloud_src
assert "c == '-' || c == '_'" in cloud_src
assert "c == '.' || c == '~'" in cloud_src
assert 'if (used > SIZE_MAX - add) return false;' in cloud_src
assert 'char *body = malloc(body_size);' in refresh_body
assert refresh_body.count('malloc(') == 1
assert 'body + prefix_length' in refresh_body
assert 'encoded_length + 1U' in refresh_body
assert 'zero_secure(body, body_size);' in refresh_body
assert 'token_request(body, &state->tokens, true)' in refresh_body
assert 'refresh_token=%s' not in refresh_body

print('PATCH031_SOURCE_INVARIANTS PASS')

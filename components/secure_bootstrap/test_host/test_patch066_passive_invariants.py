"""Tie the production wrappers and published JSON to the existing passive path."""
from pathlib import Path
import re
c = Path(__file__).resolve().parents[1]
s = (c/'athom_pre_tls_diag.c').read_text()
cmake = (c/'CMakeLists.txt').read_text()
for symbol in ('mbedtls_ssl_handshake','mbedtls_net_send','mbedtls_net_recv'):
    assert len(re.findall(r'__real_'+symbol+r'\s*\(',s)) == 2
    assert '__wrap_'+symbol in s and symbol in cmake
assert 'ssl->MBEDTLS_PRIVATE(state)' in s
assert 'mbedtls_ssl_handshake_step(' not in s  # no substituted/extra handshake steps
for symbol in ('mbedtls_net_send','mbedtls_net_recv'):
    body = s.split('int __wrap_'+symbol+'(',1)[1].split('\n}',1)[0]
    assert body.count('__real_'+symbol+'(')==1
    assert 'owned() && s_connection_window && s_handshake_active' in body
    assert 'buf[' not in body and 'memcpy' not in body and 'strlen' not in body
    assert 'errno = entry_errno;' in body and 'errno = saved_errno;' in body
assert not any(x in s for x in ('ESP_LOG','nvs_','esp_http_client_perform','mbedtls_ssl_read(',
                               'mbedtls_ssl_write(','esp_wifi_connect','sendto(','recvfrom('))
header = (c/'include/athom_pre_tls_diag.h').read_text()
handshake = header.split('typedef struct {',1)[1].split('} athom_tls_handshake_diagnostic_t;',1)[0]
assert '*' not in handshake.replace('/* bounded project classification, never a pointer */','')
client = (c/'athom_cloud_client.c').read_text()
assert 'ctx->role == HTTP_ROLE_CLOUD, s_transport_metrics.perform_count + 1U' in client
runtime = (c/'athom_oauth_runtime.c').read_text()
get = runtime.split('static esp_err_t status_get(',1)[1].split('static esp_err_t runtime_diag_journal_get(',1)[0]
assert 'athom_inventory_attempt_diagnostic_copy(' in get
assert not any(x in get for x in ('__wrap_', 'athom_pre_tls_diag_begin', 'esp_http_client_perform', 'maybe_queue', 'start_refresh'))
assert 'diagnostic.raw_pre_tls_diagnostic = after->last_pre_tls_diagnostic;' in runtime
print('PATCH066_PASSIVE_PRIVACY_INVARIANTS=PASS')

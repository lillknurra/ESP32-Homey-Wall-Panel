"""Check exact integration boundaries; behavioral coverage is in the C suite."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[3]
c=root/'components/secure_bootstrap'
s=(c/'athom_pre_tls_diag.c').read_text()
wrappers=['esp_tls_conn_new_sync','lwip_getaddrinfo','lwip_socket','lwip_connect','select','lwip_getsockopt','mbedtls_ssl_setup']
for name in wrappers:
    # One declaration and exactly one forwarding invocation, with no probes.
    assert len(re.findall(r'__real_'+name+r'\s*\(',s)) == 2, name
    assert '__wrap_'+name in s
    assert '--wrap='+name not in s
cmake=(c/'CMakeLists.txt').read_text()
assert all(name in cmake for name in wrappers)
assert 'INTERFACE "-Wl,--wrap=${symbol}"' in cmake
assert not any(token in s for token in ('ESP_LOG','nvs_','esp_http_client_perform','esp_http_client_init','esp_wifi_connect','esp_netif_set_'))
client=(c/'athom_cloud_client.c').read_text()
start=client.index('static esp_err_t http_request_limited(')
end=client.index('static esp_err_t http_request(',start)
request=client[start:end]
assert request.count('esp_http_client_perform(ctx->handle)')==1
assert request.index('athom_pre_tls_diag_begin(')<request.index('esp_http_client_perform(ctx->handle)')<request.index('athom_pre_tls_diag_finish(')
assert 'ctx->role == HTTP_ROLE_CLOUD, s_transport_metrics.perform_count + 1U' in request
runtime=(c/'athom_oauth_runtime.c').read_text()
start=runtime.index('static esp_err_t status_get(')
end=runtime.index('static esp_err_t runtime_diag_journal_get(',start)
get=runtime[start:end]
assert not any(token in get for token in ('athom_pre_tls_diag_begin','__wrap_','esp_http_client_perform','maybe_queue','start_refresh'))
assert 'athom_inventory_attempt_diagnostic_copy(' in get
assert 'diagnostic.raw_pre_tls_diagnostic = after->last_pre_tls_diagnostic;' in runtime
print('PATCH065_PASSIVE_PRIVACY_INVARIANTS=PASS')

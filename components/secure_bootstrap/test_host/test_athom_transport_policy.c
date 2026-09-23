#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_all(const char *path)
{
    FILE *f = fopen(path, "rb");
    assert(f != NULL);
    assert(fseek(f, 0, SEEK_END) == 0);
    long n = ftell(f);
    assert(n >= 0);
    rewind(f);
    char *buf = calloc(1U, (size_t)n + 1U);
    assert(buf != NULL);
    assert(fread(buf, 1U, (size_t)n, f) == (size_t)n);
    fclose(f);
    return buf;
}

static void require(const char *haystack, const char *needle)
{
    if (strstr(haystack, needle) == NULL) {
        fprintf(stderr, "missing policy marker: %s\n", needle);
        exit(2);
    }
}

static void forbid(const char *haystack, const char *needle)
{
    if (strstr(haystack, needle) != NULL) {
        fprintf(stderr, "forbidden policy marker: %s\n", needle);
        exit(3);
    }
}

int main(int argc, char **argv)
{
    assert(argc == 4);
    char *src = read_all(argv[1]);
    char *hdr = read_all(argv[2]);
    char *runtime = read_all(argv[3]);

    require(src, "#define CLOUD_HTTP_TIMEOUT_MS 12000");
    require(src, "#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000");
    require(src, "persistent_http_client_t s_cloud_http");
    require(src, "persistent_http_client_t s_homey_http");
    require(src, "esp_http_client_set_url(ctx->handle, url)");
    require(src, "esp_http_client_set_user_data(ctx->handle, &buffer)");
    require(src, "selected->remote_url");
    require(src, "state->selected_homey.remote_url");
    require(src, "state->selected_homey.local_url_secure[0] = 0");
    require(src, "state->selected_homey.local_url[0] = 0");
    require(src, "PATCH019A1_TRANSPORT");
    require(src, "PATCH019A1_MEMORY");
    require(src, "PATCH021_HTTP_ATTEMPT");
    require(src, "static bool s_patch041_homey_transport_live;");
    require(src, "static esp_err_t patch041_homey_to_cloud_handoff(void)");
    require(src, "PATCH041_HANDOFF action=homey_to_cloud_close");
    require(src, "s_patch041_homey_transport_live = true;");
    require(src, "s_patch041_homey_transport_live = close_err == ESP_OK ? false : true;");
    require(src, "esp_http_client_close(s_homey_http.handle)");
    require(src, "timeout_ms=%d");
    require(src, "DEVICES_RESPONSE_RECEIVED");
    require(src, "privacy=sanitized");
    forbid(src, "getaddrinfo(");
    forbid(src, "dns_begin host=");
    forbid(src, "homey_login_local_secure");
    forbid(src, "homey_login_local\"");
    forbid(src, "athom_homey_preferred_url(&state->selected_homey)");
    forbid(src, "url=%s");

    char *http_limited_begin = strstr(src, "static esp_err_t http_request_limited(");
    char *http_limited_end = strstr(src, "static esp_err_t http_request(");
    assert(http_limited_begin != NULL);
    assert(http_limited_end != NULL);
    assert(http_limited_begin < http_limited_end);
    char http_limited_saved = *http_limited_end;
    *http_limited_end = '\0';
    forbid(http_limited_begin, "patch041_homey_to_cloud_handoff()");
    *http_limited_end = http_limited_saved;

    const char *cloud_call_sites[] = {
        "static esp_err_t token_request(",
        "esp_err_t athom_cloud_fetch_user_homeys(",
        "static esp_err_t delegation_token(",
        "esp_err_t athom_cloud_debug_probe_user_me(",
    };
    for (size_t i = 0U; i < sizeof(cloud_call_sites) / sizeof(cloud_call_sites[0]); ++i) {
        char *begin = strstr(src, cloud_call_sites[i]);
        assert(begin != NULL);
        char *handoff = strstr(begin, "patch041_homey_to_cloud_handoff()");
        char *request = strstr(begin, "http_request(");
        assert(handoff != NULL);
        assert(request != NULL);
        assert(handoff < request);
    }

    require(hdr, "ATHOM_TRANSPORT_DNS_FAIL");
    require(hdr, "ATHOM_TRANSPORT_TCP_CONNECT_FAIL");
    require(hdr, "ATHOM_TRANSPORT_TLS_FAIL");
    require(hdr, "ATHOM_TRANSPORT_HTTP_TIMEOUT");
    require(hdr, "ATHOM_TRANSPORT_HTTP_401");
    require(hdr, "ATHOM_TRANSPORT_HTTP_403");
    require(hdr, "ATHOM_TRANSPORT_HTTP_408");
    require(hdr, "ATHOM_TRANSPORT_HTTP_429");
    require(hdr, "ATHOM_TRANSPORT_HTTP_5XX");
    require(hdr, "ATHOM_TRANSPORT_HOMEY_SESSION_FAIL");
    require(hdr, "ATHOM_TRANSPORT_FAVORITES_FAIL");
    require(hdr, "ATHOM_TRANSPORT_ZONES_FAIL");
    require(hdr, "ATHOM_TRANSPORT_DEVICES_FAIL");
    require(hdr, "ATHOM_TRANSPORT_PARSE_FAIL");
    require(hdr, "ATHOM_TRANSPORT_NO_VALID_ENDPOINT");

    require(runtime, "PATCH021_HOMEY_PHASE");
    require(runtime, "ATHOM_NETWORK_PHASE_NONE");
    require(runtime, "ATHOM_NETWORK_PHASE_AUTH_RESTORE");
    require(runtime, "ATHOM_NETWORK_PHASE_OAUTH");
    require(runtime, "ATHOM_NETWORK_PHASE_HOMEY_SELECT");
    require(runtime, "ATHOM_NETWORK_PHASE_INVENTORY_REFRESH");
    require(runtime, "ATHOM_NETWORK_PHASE_LIGHT_TOGGLE");
    require(runtime, "ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH");
    require(runtime, "ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE");
    require(runtime, "ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC");
    require(runtime, "static bool network_phase_try_reserve(");
    require(runtime, "static void network_phase_release(");
    require(runtime, "PATCH041_NETWORK_PHASE action=reserve");
    require(runtime, "PATCH041_NETWORK_PHASE action=release");
    require(runtime, "released && owner != ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE");
    require(runtime, "maybe_start_preselection_restore_worker();");
    require(runtime, "PATCH021_HOMEY_REMOTE");
    require(runtime, "next_delay_ms=%u");
    require(runtime, "athom_cloud_transport_metrics_copy(&metrics)");
    require(runtime, "homey_data_retry_delay_ms(attempt)");
    require(runtime, "ATHOM_HOMEY_DATA_RETRY_1_MS 5000U");
    require(runtime, "ATHOM_HOMEY_DATA_RETRY_2_MS 10000U");
    require(runtime, "ATHOM_HOMEY_DATA_RETRY_3_MS 20000U");
    require(runtime, "ATHOM_HOMEY_DATA_RETRY_MAX_MS 30000U");
    forbid(runtime, "homey_data_retry_delay_ms(attempt +");
    forbid(runtime, "HOMEY_REMOTE_HTTP_TIMEOUT_MS 12000");
    forbid(runtime, "setCapabilityValue");
    forbid(runtime, "triggerFlow");

    char *oauth_callback = strstr(runtime, "static esp_err_t callback_get(");
    char *select_post = strstr(runtime, "static esp_err_t select_post(");
    char *queue_light = strstr(runtime, "athom_light_toggle_queue_result_t athom_oauth_runtime_queue_light_toggle(");
    char *queue_inventory = strstr(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready(");
    char *live_refresh = strstr(runtime, "static esp_err_t refresh_post(");
    char *preselect = strstr(runtime, "static void maybe_start_preselection_restore_worker(void)");
    char *diag_post = strstr(runtime, "static esp_err_t patch031_diag_cloud_user_me_probe_post(");
    char *register_handlers = strstr(runtime, "esp_err_t athom_oauth_runtime_register_handlers(");
    assert(oauth_callback != NULL && strstr(oauth_callback, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_OAUTH)") != NULL);
    assert(select_post != NULL && strstr(select_post, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_HOMEY_SELECT)") != NULL);
    assert(queue_light != NULL && strstr(queue_light, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_LIGHT_TOGGLE)") != NULL);
    assert(queue_inventory != NULL && strstr(queue_inventory, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_INVENTORY_REFRESH)") != NULL);
    assert(live_refresh != NULL && strstr(live_refresh, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_LIVE_TOKEN_REFRESH)") != NULL);
    assert(preselect != NULL && strstr(preselect, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_PRESELECTION_RESTORE)") != NULL);
    assert(diag_post != NULL && strstr(diag_post, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_PATCH031_DIAGNOSTIC)") != NULL);
    assert(register_handlers != NULL && strstr(register_handlers, "network_phase_try_reserve(ATHOM_NETWORK_PHASE_AUTH_RESTORE)") != NULL);

    char *generic_begin =
        strstr(runtime, "static bool homey_data_failure_is_transient");
    char *generic_end =
        strstr(runtime, "static uint32_t homey_data_retry_delay_ms");
    assert(generic_begin != NULL);
    assert(generic_end != NULL);
    assert(generic_begin < generic_end);
    char generic_saved = *generic_end;
    *generic_end = '\0';
    forbid(generic_begin, "ESP_ERR_HTTP_EAGAIN");
    *generic_end = generic_saved;

    char *worker_begin =
        strstr(runtime, "static void homey_command_worker");
    char *worker_end =
        strstr(runtime, "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready");
    assert(worker_begin != NULL);
    assert(worker_end != NULL);
    assert(worker_begin < worker_end);
    char worker_saved = *worker_end;
    *worker_end = '\0';

    char *policy_begin = strstr(worker_begin, "const bool transient =");
    assert(policy_begin != NULL);
    char *policy_end = strstr(policy_begin, "ESP_LOGW(TAG,");
    assert(policy_end != NULL);
    char policy_saved = *policy_end;
    *policy_end = '\0';

    char *generic_call = strstr(
        policy_begin,
        "homey_data_failure_is_transient(effective_error, http_status)");
    char *boot_gate = strstr(policy_begin, "boot_auto &&");
    char *eagain = strstr(
        policy_begin,
        "effective_error == ESP_ERR_HTTP_EAGAIN");
    char *http_zero = strstr(policy_begin, "http_status == 0");
    assert(generic_call != NULL);
    assert(boot_gate != NULL);
    assert(eagain != NULL);
    assert(http_zero != NULL);
    assert(generic_call < boot_gate);
    assert(boot_gate < eagain);
    assert(eagain < http_zero);
    assert(strstr(eagain + strlen("effective_error == ESP_ERR_HTTP_EAGAIN"),
                  "ESP_ERR_HTTP_EAGAIN") == NULL);

    *policy_end = policy_saved;
    require(worker_begin, "if (!boot_auto || !transient)");
    require(worker_begin, "homey_data_retry_delay_ms(attempt)");
    *worker_end = worker_saved;

    puts("BOOT_AUTO_EAGAIN_HTTP0_POLICY=PASS_ENTERS_EXISTING_RETRY_PATH");
    puts("NON_BOOT_EAGAIN_HTTP0_POLICY=PASS_NO_NEW_RETRY");
    puts("GENERIC_TRANSIENT_POLICY=PASS_EAGAIN_ABSENT");

    free(src);
    free(hdr);
    free(runtime);
    puts("PATCH019A1_TRANSPORT_POLICY_HOST_TEST=PASS");
    return 0;
}

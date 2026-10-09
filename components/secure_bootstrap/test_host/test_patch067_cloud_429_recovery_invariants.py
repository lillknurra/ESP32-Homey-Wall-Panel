from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RUNTIME = (ROOT / "components/secure_bootstrap/athom_oauth_runtime.c").read_text()
CLIENT = (ROOT / "components/secure_bootstrap/athom_cloud_client.c").read_text()
HEADER = (ROOT / "components/secure_bootstrap/include/athom_cloud_client.h").read_text()


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    assert start >= 0, signature
    brace = source.find("{", start)
    assert brace >= 0, signature
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated: {signature}")


origin_path = function_body(
    RUNTIME,
    "static esp_err_t connect_and_fetch_inventory_for_origin(",
)
cached_path = function_body(
    RUNTIME,
    "static esp_err_t fetch_inventory_from_cached_session(",
)
worker = function_body(RUNTIME, "static void homey_command_worker(void *arg)")
bounded_retry = function_body(
    RUNTIME,
    "static bool inventory_refresh_worker_should_retry_after_cloud_429(",
)
preselection_retry_delay = function_body(
    RUNTIME,
    "static uint32_t preselection_restore_retry_delay_ms(",
)
preselection_retry_allowed = function_body(
    RUNTIME,
    "static bool preselection_restore_retry_allowed(",
)
cache_validate = function_body(CLIENT, "static bool cached_homey_session_matches(")
cached_fetch = function_body(
    CLIENT,
    "esp_err_t athom_cloud_fetch_inventory_from_cached_session(",
)
inventory_impl = function_body(
    CLIENT,
    "static esp_err_t athom_cloud_fetch_inventory_impl(",
)

# A periodic snapshot uses only the existing selected session. It does not add
# a cloud request or create/rebind a Homey remote session.
periodic_branch = origin_path.split(
    "if (origin == ATHOM_REFRESH_ORIGIN_PERIODIC ||", 1
)[1].split("}", 1)[0]
assert "fetch_inventory_from_cached_session(homey_id)" in periodic_branch
assert "connect_and_fetch_inventory(homey_id)" not in periodic_branch
assert "connect_and_fetch_inventory_for_origin(" in worker
assert "selected_homey_id, command.origin," in worker
assert "athom_cloud_fetch_user_homeys(" not in cached_path
assert "athom_cloud_select_and_connect(" not in cached_path
assert "cloud_discovery=skipped" in cached_path

# Fallback is specific to the Cloud user-discovery HTTP 429 result. 401, 403,
# other Cloud errors and Homey inventory errors retain their existing paths.
assert "athom_cloud_diagnostic_http_status() != 429" in origin_path
assert 'strcmp(stage, "oauth_user_me_http") != 0' in origin_path
assert "cloud_discovery_429_seen != NULL && *cloud_discovery_429_seen" in origin_path
assert "*cloud_discovery_429_seen = true" in origin_path
assert "fetch_inventory_from_cached_session(homey_id)" in origin_path
assert "failed_attempt >= 2U" in bounded_retry
assert "inventory_refresh_worker_should_retry(origin, transient)" in bounded_retry
assert "http_status == 429 ? 60000U" in preselection_retry_delay
assert "http_status != 429 || attempt < 2U" in preselection_retry_allowed

# Cached access fails closed unless selected ID, cached discovery entry,
# remote endpoint, session and selected alias binding agree.
for marker in (
    "state->selected_homey.id, expected_homey_id",
    "state->homeys.count > ATHOM_HOMEY_MAX",
    "athom_homey_find_exact(&state->homeys, expected_homey_id)",
    "state->homey_session_token",
    "state->selected_homey.remote_url",
    "strcmp(cached->remote_url, state->selected_homey.remote_url) == 0",
):
    assert marker in cache_validate, marker
assert "athom_cloud_alias_activate(expected_homey_id)" in cached_fetch
assert "PANEL_HOMEY_ALIAS_STORE_OK" in cached_fetch
assert "athom_cloud_fetch_inventory_impl(state, false)" in cached_fetch

# The cached inventory implementation remains read-only: only GET collection
# fetches, parsers and snapshot publication are reachable from this helper.
assert '"/api/manager/zones/zone"' in inventory_impl
assert '"/api/manager/devices/device"' in inventory_impl
for forbidden in (
    "HTTP_METHOD_POST",
    "esp_http_client_set_method(.*POST",
    "nvs_set_",
    "athom_cloud_set_favorite_light_onoff",
    "panel_homey_alias_store_save",
):
    assert forbidden not in cached_fetch + cache_validate + inventory_impl, forbidden
assert "Retry-After" not in CLIENT
assert "Retry-After" not in RUNTIME
assert "athom_cloud_fetch_inventory_from_cached_session" in HEADER

print("PATCH067_CLOUD_429_RECOVERY_INVARIANTS=PASS")

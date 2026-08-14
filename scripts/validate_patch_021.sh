#!/bin/sh

fail() {
  echo "PATCH021_VALIDATION=FAIL reason=$1"
  exit 1
}

branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)" || fail "git_branch_unavailable"
test "$branch" = "patch-021-homey-remote-panel-ui-responsiveness-diagnostics" || fail "wrong_branch"

changed="$( (git diff --name-only; git diff --cached --name-only; git ls-files --others --exclude-standard) | sort -u )"
test -n "$changed" || fail "no_patch_changes"

allowed='^(components/secure_bootstrap/athom_cloud_client.c|components/secure_bootstrap/include/athom_cloud_client.h|components/secure_bootstrap/athom_oauth_runtime.c|components/secure_bootstrap/panel_ui.c|components/secure_bootstrap/secure_bootstrap_esp.c|components/secure_bootstrap/test_host/test_athom_transport_policy.c|components/secure_bootstrap/test_host/run_athom_transport_policy_tests.py|docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md|docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md|scripts/validate_patch_021.sh)$'

unexpected="$(printf '%s\n' "$changed" | grep -Ev "$allowed" || true)"
test -z "$unexpected" || fail "unexpected_scope:$unexpected"

component_changes="$(printf '%s\n' "$changed" | grep '^components/' || true)"
forbidden_components="$(printf '%s\n' "$component_changes" | grep -Ev '^(components/secure_bootstrap/athom_cloud_client.c|components/secure_bootstrap/include/athom_cloud_client.h|components/secure_bootstrap/athom_oauth_runtime.c|components/secure_bootstrap/panel_ui.c|components/secure_bootstrap/secure_bootstrap_esp.c|components/secure_bootstrap/test_host/test_athom_transport_policy.c|components/secure_bootstrap/test_host/run_athom_transport_policy_tests.py)$' || true)"
test -z "$forbidden_components" || fail "forbidden_component_scope:$forbidden_components"

if printf '%s\n' "$changed" | grep -Eq '^sdkconfig'; then
  fail "sdkconfig_changed"
fi

grep -F '#define CLOUD_HTTP_TIMEOUT_MS 8000' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "cloud_timeout_changed"
grep -F '#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "homey_timeout_changed"
grep -F 'selected->remote_url' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "remote_login_policy_missing"
grep -F 'state->selected_homey.remote_url' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "remote_inventory_policy_missing"
grep -F 'state->selected_homey.local_url_secure[0] = 0' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "local_secure_runtime_clear_missing"
grep -F 'state->selected_homey.local_url[0] = 0' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "local_runtime_clear_missing"

grep -F '#define ATHOM_HOMEY_DATA_RETRY_1_MS 5000U' components/secure_bootstrap/athom_oauth_runtime.c >/dev/null || fail "retry_1_changed"
grep -F '#define ATHOM_HOMEY_DATA_RETRY_2_MS 10000U' components/secure_bootstrap/athom_oauth_runtime.c >/dev/null || fail "retry_2_changed"
grep -F '#define ATHOM_HOMEY_DATA_RETRY_3_MS 20000U' components/secure_bootstrap/athom_oauth_runtime.c >/dev/null || fail "retry_3_changed"
grep -F '#define ATHOM_HOMEY_DATA_RETRY_MAX_MS 30000U' components/secure_bootstrap/athom_oauth_runtime.c >/dev/null || fail "retry_max_changed"

grep -F 'PATCH021_HTTP_ATTEMPT' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "http_attempt_marker_missing"
grep -F 'PATCH021_HOMEY_PHASE' components/secure_bootstrap/athom_oauth_runtime.c >/dev/null || fail "homey_phase_marker_missing"
grep -F 'PATCH021_HOMEY_REMOTE' components/secure_bootstrap/athom_oauth_runtime.c >/dev/null || fail "homey_remote_marker_missing"
grep -F 'PATCH021_UI_SCROLL' components/secure_bootstrap/panel_ui.c >/dev/null || fail "ui_scroll_marker_missing"
grep -F 'PATCH021_DISPLAY_PERF' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "display_perf_marker_missing"

grep -F 'PATCH018_SWIPE_BEGIN' components/secure_bootstrap/panel_ui.c >/dev/null || fail "patch018_begin_missing"
grep -F 'PATCH018_SWIPE_END' components/secure_bootstrap/panel_ui.c >/dev/null || fail "patch018_end_missing"
grep -F 'lv_indev_set_scroll_limit(input_device, 4)' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_limit_changed"
grep -F 'lv_indev_set_scroll_throw(input_device, 4)' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_throw_changed"

if git diff -- components/secure_bootstrap/athom_cloud_client.c components/secure_bootstrap/athom_oauth_runtime.c components/secure_bootstrap/panel_ui.c components/secure_bootstrap/secure_bootstrap_esp.c | grep -E 'setCapabilityValue|triggerFlow|runFlow|activateMood|HTTP_METHOD_(PUT|DELETE|PATCH)' >/dev/null; then
  fail "mutation_or_command_marker_in_component_diff"
fi

if git diff -- components/secure_bootstrap/athom_cloud_client.c components/secure_bootstrap/athom_oauth_runtime.c components/secure_bootstrap/panel_ui.c components/secure_bootstrap/secure_bootstrap_esp.c | grep -E 'ESP_LOG[IEWD].*(access_token|refresh_token|Authorization|client_secret|homey_id|remote_url|localUrl|https?://)' >/dev/null; then
  fail "sensitive_logging_marker_in_component_diff"
fi

grep -F 'Patch021: Homey remote and panel UI responsiveness diagnostics' docs/history/PATCH_HISTORY.md >/dev/null || fail "history_entry_missing"
grep -F 'PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_entry_missing"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS' docs/handoff/CURRENT_STATE.md >/dev/null || fail "current_state_active_patch_missing"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_state_missing"
grep -F 'Package 3B remains `NOT_STARTED`' docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md >/dev/null || fail "patch_doc_package3b_missing"

python3 -c 'import ast, pathlib; ast.parse(pathlib.Path("components/secure_bootstrap/test_host/run_athom_transport_policy_tests.py").read_text())' || fail "transport_runner_py_parse"

echo "PATCH021_SOURCE_SCOPE=PASS"
echo "PATCH021_POLICY_BOUNDARY=PASS"
echo "PATCH021_DIAGNOSTIC_MARKERS=PASS"
echo "PATCH021_PRIVACY=PASS"
echo "PATCH021_VALIDATION=PASS"

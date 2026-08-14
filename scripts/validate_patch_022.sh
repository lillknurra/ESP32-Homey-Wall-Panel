#!/bin/sh

fail() {
  echo "PATCH022_VALIDATION=FAIL reason=$1"
  exit 1
}

branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)" || fail "git_branch_unavailable"
test "$branch" = "patch-022-bounded-panel-ui-scroll-responsiveness" || fail "wrong_branch"

changed="$( (git diff HEAD --name-only; git ls-files --others --exclude-standard) | sort -u )"
test -n "$changed" || fail "no_patch_changes"

allowed='^(components/secure_bootstrap/secure_bootstrap_esp.c|docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md|scripts/validate_patch_022.sh)$'
unexpected="$(printf '%s\n' "$changed" | grep -Ev "$allowed" || true)"
test -z "$unexpected" || fail "unexpected_scope:$unexpected"

if printf '%s\n' "$changed" | grep -Eq '^(components/secure_bootstrap/panel_ui.c|components/secure_bootstrap/athom_cloud_client.c|components/secure_bootstrap/athom_oauth_runtime.c|sdkconfig)'; then
  fail "forbidden_firmware_or_sdkconfig_scope"
fi

grep -F '#define PATCH022_SCROLL_LIMIT 4U' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_limit_candidate_missing"
grep -F '#define PATCH022_SCROLL_THROW 20U' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_throw_candidate_missing"
grep -F 'lv_indev_set_scroll_limit(input_device, PATCH022_SCROLL_LIMIT)' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_limit_binding_missing"
grep -F 'lv_indev_set_scroll_throw(input_device, PATCH022_SCROLL_THROW)' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_throw_binding_missing"
grep -F 'PATCH021_DISPLAY_PERF' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "display_diagnostics_missing"

if git diff HEAD -- components/secure_bootstrap/secure_bootstrap_esp.c | grep -E 'setCapabilityValue|triggerFlow|runFlow|activateMood|HTTP_METHOD_(PUT|DELETE|PATCH)|athom_cloud|oauth|retry|timeout|backoff|endpoint' >/dev/null; then
  fail "forbidden_transport_or_mutation_change"
fi

if git diff HEAD -- components/secure_bootstrap/secure_bootstrap_esp.c | grep -E 'ESP_LOG[IEWD].*(access_token|refresh_token|Authorization|client_secret|homey_id|remote_url|localUrl|https?://)' >/dev/null; then
  fail "sensitive_logging_marker_in_component_diff"
fi

grep -F 'Patch022 - Bounded Panel UI Scroll Responsiveness' docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md >/dev/null || fail "patch_doc_missing"
grep -F 'PATCH022_BOUNDED_PANEL_UI_SCROLL_RESPONSIVENESS' docs/handoff/CURRENT_STATE.md >/dev/null || fail "current_state_active_patch_missing"
grep -F 'PATCH_021A=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021a_merge_state_missing"
grep -F 'PATCH_021A_PR=30' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021a_pr_missing"
grep -F 'PATCH_021A_MERGE=7049ccbda3a9cce120f0bb73f2ec06e8be06b464' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021a_merge_missing"
grep -F 'patch-022-bounded-panel-ui-scroll-responsiveness' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_branch_missing"
grep -F 'PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_entry_missing"
grep -F 'Package 3B' docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md >/dev/null || fail "package3b_boundary_missing"

echo "PATCH022_SOURCE_SCOPE=PASS"
echo "PATCH022_SCROLL_CANDIDATE=PASS"
echo "PATCH022_POLICY_BOUNDARY=PASS"
echo "PATCH022_PRIVACY=PASS"
echo "PATCH022_VALIDATION=PASS"

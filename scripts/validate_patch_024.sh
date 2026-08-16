#!/usr/bin/env bash
set -u

fail() {
    printf 'PATCH024_STATIC_VALIDATION=FAIL reason=%s\n' "$1" >&2
    exit 1
}

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$repo_dir" || exit 1

is_allowed_scope_path() {
    case "$1" in
        components/secure_bootstrap/panel_ui.c|\
        components/secure_bootstrap/secure_bootstrap_esp.c|\
        docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|\
        docs/handoff/CURRENT_STATE.md|\
        docs/handoff/HANDOFF.md|\
        docs/handoff/MASTER_INDEX.md|\
        docs/history/PATCH_HISTORY.md|\
        docs/history/PATCH_024_PANEL_UI_RENDER_PATH_ATTRIBUTION_DIAGNOSTICS.md|\
        scripts/validate_patch_024.sh)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

actual_scope=$( { git diff --name-only HEAD; git ls-files --others --exclude-standard; } ) || fail "scope_read"
for path in $actual_scope; do
    is_allowed_scope_path "$path" || fail "scope:$path"
done

for required in \
    components/secure_bootstrap/panel_ui.c \
    components/secure_bootstrap/secure_bootstrap_esp.c \
    docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md \
    docs/handoff/CURRENT_STATE.md \
    docs/handoff/HANDOFF.md \
    docs/handoff/MASTER_INDEX.md \
    docs/history/PATCH_HISTORY.md \
    docs/history/PATCH_024_PANEL_UI_RENDER_PATH_ATTRIBUTION_DIAGNOSTICS.md \
    scripts/validate_patch_024.sh; do
    printf '%s\n' "$actual_scope" | grep -Fx "$required" >/dev/null || fail "scope_missing:$required"
done

[ "$(git branch --show-current)" = "patch-024-bounded-panel-ui-render-path-attribution-diagnostics" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "749f6caadefaf69e0ecd6f4df7aaf98880e0739d" ] || fail "base_head"
[ "$(git rev-parse main)" = "749f6caadefaf69e0ecd6f4df7aaf98880e0739d" ] || fail "local_main"
[ "$(git rev-parse origin/main)" = "749f6caadefaf69e0ecd6f4df7aaf98880e0739d" ] || fail "origin_main"

grep -F '#include "esp_timer.h"' components/secure_bootstrap/panel_ui.c >/dev/null || fail "panel_timer_include"
grep -F 'PATCH024_RENDER_PATH phase=panel_refresh' components/secure_bootstrap/panel_ui.c >/dev/null || fail "panel_refresh_marker"
grep -F 'lv_obj_is_scrolling(ui->pager)' components/secure_bootstrap/panel_ui.c >/dev/null || fail "pager_scroll_attribution"
grep -F 'lv_obj_is_scrolling(ui->settings_layer)' components/secure_bootstrap/panel_ui.c >/dev/null || fail "settings_scroll_attribution"
grep -F 'panel_display_lock_traced' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "lock_attribution"
grep -F 'PATCH024_RENDER_PATH window_ms=' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "lock_summary_marker"
grep -F 'homey_model_changed_count' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "model_change_attribution"
grep -F 'homey_favorites_changed_count' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "favorites_change_attribution"
grep -F '#define PATCH022_SCROLL_LIMIT 4U' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_limit_changed"
grep -F '#define PATCH022_SCROLL_THROW 20U' components/secure_bootstrap/secure_bootstrap_esp.c >/dev/null || fail "scroll_throw_changed"

added_panel_lines=$(git diff --unified=0 -- components/secure_bootstrap/panel_ui.c | grep '^+' | grep -v '^+++' || true)
added_bootstrap_lines=$(git diff --unified=0 -- components/secure_bootstrap/secure_bootstrap_esp.c | grep '^+' | grep -v '^+++' || true)
if printf '%s\n' "$added_panel_lines" | grep -Eq 'lv_obj_set_(size|pos|style)|lv_obj_add_flag|lv_obj_remove_flag|lv_obj_set_scroll_(dir|snap)'; then
    fail "ui_behavior_change"
fi
if printf '%s\n' "$added_bootstrap_lines" | grep -Eq 'lv_indev_set_scroll_(limit|throw)|PATCH022_SCROLL_(LIMIT|THROW)'; then
    fail "scroll_policy_change"
fi
if git diff --name-only HEAD | grep '^managed_components/' >/dev/null 2>&1; then
    fail "forbidden_component_scope"
fi
if printf '%s\n%s\n' "$added_panel_lines" "$added_bootstrap_lines" | grep -Eq 'authorization:|Bearer |access_token=|refresh_token='; then
    fail "privacy_pattern"
fi
if grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null 2>&1; then :; else fail "package3b_boundary"; fi
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
grep -F 'Patch024' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch024_state"
grep -F 'PATCH024_PANEL_UI_RENDER_PATH_ATTRIBUTION_DIAGNOSTICS' docs/handoff/MASTER_INDEX.md >/dev/null || fail "patch024_index"
grep -F 'managed_components/**' docs/history/PATCH_024_PANEL_UI_RENDER_PATH_ATTRIBUTION_DIAGNOSTICS.md >/dev/null || fail "managed_boundary"

git diff --check >/dev/null 2>&1 || fail "diff_check"

printf 'PATCH024_STATIC_VALIDATION=PASS\n'

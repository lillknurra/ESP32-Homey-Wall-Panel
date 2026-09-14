#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BASE="24405241476901170a75321aeeb938cc4b3faf5c"
cd "$ROOT"

python3 components/secure_bootstrap/test_host/run_patch038_async_light_toggle_tests.py --validate-source
python3 components/secure_bootstrap/test_host/run_patch038_async_light_toggle_tests.py
python3 components/secure_bootstrap/test_host/run_homey_light_toggle_dispatch_tests.py
python3 components/secure_bootstrap/test_host/run_panel_homey_favorites_tests.py

PANEL_UI_BASELINE_PATHS="
components/secure_bootstrap/panel_ui_model.c
components/secure_bootstrap/panel_homey_dashboard_binding.c
components/secure_bootstrap/panel_ui_store.c
components/secure_bootstrap/include/panel_ui_model.h
components/secure_bootstrap/include/panel_homey_dashboard_binding.h
components/secure_bootstrap/include/panel_ui_store.h
components/secure_bootstrap/test_host/test_panel_ui_model.c
components/secure_bootstrap/test_host/test_panel_homey_dashboard_binding.c
components/secure_bootstrap/test_host/run_panel_ui_tests.py
"

# The stable Patch017 ownership boundary deliberately excludes widgets 4/5
# from panel_ui_apply_homey_dashboard_state(). The legacy model test still
# expects widget 4 to be updated by that snapshot. Prove that every source
# involved in that failing test is unchanged from stable before classifying
# the exact assertion as pre-existing baseline debt.
for path in $PANEL_UI_BASELINE_PATHS; do
    git diff --quiet "$BASE" -- "$path" || {
        printf '%s\n' "PATCH038_PANEL_UI_BASELINE_IDENTITY=FAIL path=$path"
        exit 1
    }
done
printf '%s\n' "PATCH038_PANEL_UI_BASELINE_IDENTITY=PASS"

TMPROOT="$(mktemp -d "${TMPDIR:-/tmp}/patch038_panel_ui.XXXXXX")"
cleanup() {
    rm -rf "$TMPROOT"
}
trap cleanup EXIT HUP INT TERM

PANEL_LOG="$TMPROOT/panel_ui_baseline.log"
if python3 components/secure_bootstrap/test_host/run_panel_ui_tests.py >"$PANEL_LOG" 2>&1; then
    cat "$PANEL_LOG"
    printf '%s\n' "PATCH038_PANEL_UI_BASELINE_STALE_ASSERTION=UNEXPECTED_PASS"
    exit 1
fi
cat "$PANEL_LOG"
grep -Fq 'Assertion failed: (model.widget_status[4] == PANEL_WIDGET_AVAILABLE)' "$PANEL_LOG" || {
    printf '%s\n' "PATCH038_PANEL_UI_BASELINE_STALE_ASSERTION=FAIL_DIFFERENT_FAILURE"
    exit 1
}
printf '%s\n' "PATCH038_PANEL_UI_BASELINE_STALE_ASSERTION=PASS_EXACT_PREEXISTING_FAILURE"

# Run every other panel_ui_model test from the unchanged stable test source.
sed '/test_homey_dashboard_model_integration();/d' \
    components/secure_bootstrap/test_host/test_panel_ui_model.c \
    > "$TMPROOT/test_panel_ui_model_without_stale.c"

cc -std=c11 -Wall -Wextra -Werror -Wno-unused-function -pedantic \
    -I components/secure_bootstrap/include \
    components/secure_bootstrap/panel_ui_model.c \
    components/secure_bootstrap/panel_homey_dashboard_binding.c \
    components/secure_bootstrap/panel_ui_store.c \
    "$TMPROOT/test_panel_ui_model_without_stale.c" \
    -o "$TMPROOT/test_panel_ui_model_without_stale"
"$TMPROOT/test_panel_ui_model_without_stale"
printf '%s\n' "PATCH038_PANEL_UI_REMAINING_MODEL_TESTS=PASS"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
    -I components/secure_bootstrap/include \
    components/secure_bootstrap/panel_homey_dashboard_binding.c \
    components/secure_bootstrap/test_host/test_panel_homey_dashboard_binding.c \
    -o "$TMPROOT/test_panel_homey_dashboard_binding"
"$TMPROOT/test_panel_homey_dashboard_binding"
printf '%s\n' "PATCH038_PANEL_HOMEY_DASHBOARD_BINDING_TEST=PASS"

git diff --check
printf '%s\n' 'PATCH038_VALIDATOR PASS'

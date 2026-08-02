#!/bin/sh

BASE="eb51ff66b698a0667bfd604a12e68420441540fd"
BRANCH="patch-013-read-only-homey-device-snapshot-foundation"
FAIL=0

fail() {
  echo "FAIL: $1"
  FAIL=1
}

changed_paths() {
  {
    git diff --name-only
    git diff --cached --name-only
    git ls-files --others --exclude-standard
  } | sed '/^$/d' | LC_ALL=C sort -u
}

EXPECTED='components/secure_bootstrap/CMakeLists.txt
components/secure_bootstrap/athom_cloud_client.c
components/secure_bootstrap/include/athom_cloud_client.h
components/secure_bootstrap/include/panel_homey_alias_provider.h
components/secure_bootstrap/include/panel_homey_read_snapshot.h
components/secure_bootstrap/panel_homey_read_snapshot.c
components/secure_bootstrap/test_host/run_panel_homey_read_snapshot_tests.py
components/secure_bootstrap/test_host/test_panel_homey_read_snapshot.c
docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md
docs/architecture/HOMEY_INVENTORY_CONTRACT.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/handoff/MASTER_INDEX.md
docs/history/PATCH_013_READ_ONLY_HOMEY_DEVICE_SNAPSHOT_FOUNDATION.md
docs/history/PATCH_HISTORY.md
scripts/validate_patch_013.sh'

EXPECTED_SORTED="$(printf '%s\n' "$EXPECTED" | LC_ALL=C sort -u)"

[ "$(git branch --show-current)" = "$BRANCH" ] || fail "wrong branch"
[ "$(git rev-parse HEAD)" = "$BASE" ] || fail "unexpected branch HEAD"
[ "$(git rev-parse main)" = "$BASE" ] || fail "unexpected local main"
[ "$(git rev-parse origin/main)" = "$BASE" ] || fail "unexpected origin/main"
[ -z "$(git diff --cached --name-only)" ] || fail "expected zero staged files"

ACTUAL="$(changed_paths)"
[ "$ACTUAL" = "$EXPECTED_SORTED" ] || {
  echo "EXPECTED PATHS:"
  printf '%s\n' "$EXPECTED_SORTED"
  echo "ACTUAL PATHS:"
  printf '%s\n' "$ACTUAL"
  fail "exact 16-file scope mismatch"
}

for forbidden in \
  components/secure_bootstrap/secure_bootstrap_esp.c \
  components/secure_bootstrap/panel_ui.c \
  components/secure_bootstrap/panel_ui_model.c \
  components/secure_bootstrap/panel_ui_store.c \
  components/secure_bootstrap/include/secure_bootstrap.h \
  components/secure_bootstrap/include/panel_ui.h \
  components/secure_bootstrap/include/panel_ui_model.h \
  components/secure_bootstrap/include/panel_ui_store.h \
  components/secure_bootstrap/include/phone_provisioning.h \
  components/secure_bootstrap/phone_provisioning_logic.c \
  components/secure_bootstrap/phone_provisioning_store.c \
  components/secure_bootstrap/athom_oauth_flow.c \
  components/secure_bootstrap/athom_oauth_config.c \
  components/secure_bootstrap/athom_auth_store.c \
  sdkconfig.defaults
do
  printf '%s\n' "$ACTUAL" | grep -Fxq "$forbidden" && fail "forbidden changed file: $forbidden"
done

[ "$(grep -o '"panel_homey_read_snapshot.c"' components/secure_bootstrap/CMakeLists.txt | wc -l | tr -d ' ')" = "1" ] || fail "snapshot source must appear once in CMake"

grep -F '#define PANEL_HOMEY_SNAPSHOT_MAX_ITEMS 16U' components/secure_bootstrap/include/panel_homey_read_snapshot.h >/dev/null || fail "max items"
grep -F '#define PANEL_HOMEY_ALIAS_MAX 48U' components/secure_bootstrap/include/panel_homey_alias_provider.h >/dev/null || fail "alias max"
grep -F '#define PANEL_HOMEY_CAPABILITY_ALIAS_MAX 32U' components/secure_bootstrap/include/panel_homey_alias_provider.h >/dev/null || fail "capability alias max"
grep -F '#define PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS 120000ULL' components/secure_bootstrap/include/panel_homey_read_snapshot.h >/dev/null || fail "stale threshold"
grep -F 'panel_homey_read_snapshot_t buffers[2]' components/secure_bootstrap/include/panel_homey_read_snapshot.h >/dev/null || fail "double buffer"
grep -F 'panel_homey_alias_provider_not_configured' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "production provider not configured"
grep -F '"/api/manager/devices/device"' components/secure_bootstrap/athom_cloud_client.c >/dev/null || fail "existing device endpoint missing"
[ "$(grep -F 'esp_err_t athom_cloud_fetch_inventory(' components/secure_bootstrap/include/athom_cloud_client.h | wc -l | tr -d ' ')" = "1" ] || fail "public fetch API changed unexpectedly"

python3 - <<'PY_CONCURRENCY'
from pathlib import Path
import re
import sys

source = Path("components/secure_bootstrap/athom_cloud_client.c").read_text(encoding="utf-8")
required = (
    "static volatile bool s_device_snapshot_store_initialized;",
    "static portMUX_TYPE s_device_snapshot_init_mux = portMUX_INITIALIZER_UNLOCKED;",
    "portENTER_CRITICAL(&s_device_snapshot_init_mux);",
    "portEXIT_CRITICAL(&s_device_snapshot_init_mux);",
)
missing = [token for token in required if token not in source]
match = re.search(
    r"static void ensure_device_snapshot_store\(void\)\s*\{(?P<body>.*?)\n\}",
    source,
    re.DOTALL,
)
if missing or match is None:
    print("missing thread-safe snapshot initialization markers:", missing)
    sys.exit(1)
body = match.group("body")
enter = body.find("portENTER_CRITICAL(&s_device_snapshot_init_mux);")
inner_check = body.find("if (!s_device_snapshot_store_initialized)", enter)
initialize = body.find("panel_homey_snapshot_store_init(", inner_check)
mark_ready = body.find("s_device_snapshot_store_initialized = true;", initialize)
exit_lock = body.find("portEXIT_CRITICAL(&s_device_snapshot_init_mux);", mark_ready)
if min(enter, inner_check, initialize, mark_ready, exit_lock) < 0 or not (
    enter < inner_check < initialize < mark_ready < exit_lock
):
    print("snapshot initialization ordering is not protected")
    sys.exit(1)
critical = body[enter:exit_lock]
for forbidden in ("cJSON", "http", "HTTP", "ESP_LOG", "malloc", "calloc", "realloc", "free("):
    if forbidden in critical:
        print("forbidden operation in snapshot init critical section:", forbidden)
        sys.exit(1)
PY_CONCURRENCY
[ "$?" -eq 0 ] || fail "thread-safe snapshot initialization"

for test_marker in \
  test_null_arguments \
  test_copy_before_publication \
  test_empty_inventory \
  test_invalid_result \
  test_invalid_device_entry \
  test_missing_id \
  test_capabilities_missing_or_invalid \
  test_missing_capability \
  test_wrong_capability_object_type \
  test_invalid_alias_characters \
  test_alias_boundaries \
  test_item_limit_16 \
  test_item_overflow_17 \
  test_provider_errors \
  test_publication_preserves_previous \
  test_lookup_not_found \
  test_generation_progression \
  test_lock_balance
do
  grep -F "static void $test_marker(void)" \
    components/secure_bootstrap/test_host/test_panel_homey_read_snapshot.c >/dev/null || \
    fail "missing host test marker: $test_marker"
done

if grep -E 'cJSON|esp_http|athom_token|homey_session|raw_device_id|raw_capability_id' components/secure_bootstrap/include/panel_homey_read_snapshot.h; then
  fail "public snapshot header exposes forbidden runtime/parser type"
fi

FUNCTIONAL_FILES='components/secure_bootstrap/athom_cloud_client.c
components/secure_bootstrap/include/athom_cloud_client.h
components/secure_bootstrap/panel_homey_read_snapshot.c
components/secure_bootstrap/include/panel_homey_read_snapshot.h
components/secure_bootstrap/include/panel_homey_alias_provider.h'

functional_added_lines() {
  for file in $FUNCTIONAL_FILES; do
    if git ls-files --error-unmatch "$file" >/dev/null 2>&1; then
      git diff --no-ext-diff -U0 -- "$file"
    else
      git diff --no-index --no-ext-diff -U0 -- /dev/null "$file" 2>/dev/null || true
    fi
  done | sed -n 's/^+//p' | grep -v '^+++' || true
}

FUNCTIONAL_ADDED="$(functional_added_lines)"

if printf '%s\n' "$FUNCTIONAL_ADDED" | grep -E 'HTTP_METHOD_(PUT|PATCH|DELETE)|setCapabilityValue|runFlow|triggerFlow|activateMood|execute.*Flow'; then
  fail "mutation marker added in functional implementation"
fi

if printf '%s\n' "$FUNCTIONAL_ADDED" | grep -E '(/api/manager/[^"[:space:]]+)' | grep -v -F '/api/manager/devices/device'; then
  fail "new Homey manager endpoint added in functional implementation"
fi

if grep -R -n -E '(access_token|refresh_token|client_secret|authorization|homey_session_token|localUrl|remoteUrl)' \
  components/secure_bootstrap/include/panel_homey_alias_provider.h \
  components/secure_bootstrap/include/panel_homey_read_snapshot.h \
  components/secure_bootstrap/panel_homey_read_snapshot.c \
  components/secure_bootstrap/test_host/test_panel_homey_read_snapshot.c; then
  fail "secret or transport marker in snapshot module"
fi

python3 components/secure_bootstrap/test_host/run_panel_homey_read_snapshot_tests.py || fail "snapshot host tests"
python3 -c 'import ast,pathlib; ast.parse(pathlib.Path("components/secure_bootstrap/test_host/run_panel_homey_read_snapshot_tests.py").read_text(encoding="utf-8"))' || fail "runner syntax"
sh -n scripts/validate_patch_013.sh || fail "validator syntax"
git diff --check || fail "git diff --check"

for file in $(git ls-files --others --exclude-standard); do
  git diff --no-index --check /dev/null "$file" >/dev/null 2>&1
  rc=$?
  [ "$rc" -le 1 ] || fail "untracked whitespace check: $file"
done

grep -F 'ACTIVE_DEVELOPMENT_PATCH=PATCH_013' docs/handoff/CURRENT_STATE.md >/dev/null || fail "CURRENT_STATE active patch"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "Package 3B status"

if [ "$FAIL" -ne 0 ]; then
  echo "PATCH_013_VALIDATION FAIL"
  exit 1
fi

echo "PATCH_013_VALIDATION PASS"

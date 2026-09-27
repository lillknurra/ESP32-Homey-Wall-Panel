#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

python3 components/secure_bootstrap/test_host/run_panel_homey_awning_provisioning_tests.py
python3 components/secure_bootstrap/test_host/run_panel_homey_alias_store_tests.py
python3 components/secure_bootstrap/test_host/run_panel_homey_light_provisioning_tests.py
python3 -c 'import ast,pathlib; ast.parse(pathlib.Path("scripts/provision_patch_051_awning_bindings.py").read_text())'
python3 scripts/provision_patch_051_awning_bindings.py --dry-run

grep -Fq '"panel_homey_awning_provisioning.c"' components/secure_bootstrap/CMakeLists.txt
grep -Fq '#include "panel_homey_awning_provisioning.h"' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'panel_homey_awning_provisioning_merge(' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'panel_homey_alias_store_publish(' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'panel_homey_alias_sha256(' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'char *selected_homey_id_sha256;' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'PATCH051_ASSIGN(selected_homey_id_sha256, "selected_homey_id_sha256")' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'form->selected_homey_id_sha256 != NULL' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'form.selected_homey_id_sha256,' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq '{"/homey/awnings",HTTP_POST,awning_bindings_post,NULL}' components/secure_bootstrap/phone_provisioning_store.c

test "$(grep -Fo '{"/homey/awnings",HTTP_POST,awning_bindings_post,NULL}' components/secure_bootstrap/phone_provisioning_store.c | wc -l | tr -d ' ')" = "1"
! grep -Fq '{"/homey/awnings",HTTP_GET' components/secure_bootstrap/phone_provisioning_store.c

grep -Fq 'entry->dashboard_binding_index > AWNING_LAST_WIDGET_INDEX' components/secure_bootstrap/panel_homey_awning_provisioning.c
grep -Fq '(uint8_t)(AWNING_FIRST_WIDGET_INDEX + i)' components/secure_bootstrap/panel_homey_awning_provisioning.c

if grep -R -n -E 'setCapabilityValue|triggerFlow|runFlow|triggerAdvancedFlow|HTTP_METHOD_(PUT|PATCH|DELETE)' \
    components/secure_bootstrap/panel_homey_awning_provisioning.c \
    components/secure_bootstrap/phone_provisioning_store.c \
    scripts/provision_patch_051_awning_bindings.py; then
    echo "PATCH051_HOMEY_MUTATION_SURFACE=FAIL" >&2
    exit 1
fi

if grep -E 'ESP_LOG[A-Z]*\(.*(raw_device|raw_capability|device_id|capability_id)' \
    components/secure_bootstrap/phone_provisioning_store.c >/dev/null; then
    echo "PATCH051_RAW_IDENTIFIER_LOGGING=FAIL" >&2
    exit 1
fi

git diff --check

echo "PATCH051_AWNING_PROVISIONING_HOST_TESTS=PASS"
echo "PATCH051_ALIAS_STORE_REGRESSION=PASS"
echo "PATCH051_LIGHT_PROVISIONING_REGRESSION=PASS"
echo "PATCH051_PRIVATE_BINDING_DRY_RUN=PASS"
echo "PATCH051_STATIC_VALIDATION=PASS"
echo "PATCH051_HOMEY_MUTATION=NOT_IMPLEMENTED"
echo "PATCH051_FLASH=NOT_RUN"

# Patch051 V5 runtime-capacity and gate-hygiene invariants.
grep -Fq 'cfg.max_uri_handlers=23;' components/secure_bootstrap/secure_bootstrap_esp.c
grep -Fq 'patch051_awning_request_heap_t *heap' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'calloc(1U, sizeof(*heap))' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'patch051_awning_request_heap_free(heap);' components/secure_bootstrap/phone_provisioning_store.c
if sed -n '/static esp_err_t awning_bindings_post/,/static esp_err_t light_bindings_get/p' \
  components/secure_bootstrap/phone_provisioning_store.c | \
  grep -Eq 'char body\[PATCH051_AWNING_FORM_BODY_MAX\]|panel_homey_alias_record_t (existing|merged)'; then
  echo 'PATCH051_HTTP_HANDLER_STACK_STATE=FAIL' >&2
  exit 1
fi
grep -Fq 'bindings[j].device_id) == 0' components/secure_bootstrap/panel_homey_awning_provisioning.c
grep -Fq 'private binding awning device IDs must be distinct' scripts/provision_patch_051_awning_bindings.py
grep -Fq 'encoded private binding request exceeds panel body capacity' scripts/provision_patch_051_awning_bindings.py
test ! -e scripts/__pycache__
echo 'PATCH051_HTTP_HANDLER_CAPACITY=23'
echo 'PATCH051_HTTP_HANDLER_LARGE_REQUEST_STATE=HEAP'
echo 'PATCH051_AWNING_DEVICE_DISTINCTNESS=REQUIRED'
echo 'PATCH051_VALIDATOR_PYCACHE_SIDE_EFFECT=ABSENT'

# PATCH051_V8 compile regression guard.
grep -Fq 'char selected_homey_id[' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'active_homey_id(selected_homey_id, sizeof(selected_homey_id));' components/secure_bootstrap/phone_provisioning_store.c
grep -Fq 'zero_secure(selected_homey_id, sizeof(selected_homey_id));' components/secure_bootstrap/phone_provisioning_store.c
if sed -n '/static esp_err_t light_bindings_get/,/static esp_err_t wipe_post/p' components/secure_bootstrap/phone_provisioning_store.c | grep -Fq 'char active_homey_id['; then
  echo 'PATCH051_LIGHT_BINDINGS_HELPER_LOCAL_COLLISION=FAIL' >&2
  exit 1
fi
echo 'PATCH051_LIGHT_BINDINGS_HELPER_LOCAL_COLLISION=ABSENT'

# Patch051 V10 pre-merge durable-state lock assertions.
grep -Fq 'PATCH051_STATUS=ACTIVE_PR83__BUILD_VALIDATED__PRE_MERGE_LOCKED' docs/handoff/CURRENT_STATE.md
grep -Fq 'ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=Patch051' docs/handoff/CURRENT_STATE.md
grep -Fq 'ACTIVE_FUNCTIONAL_DEVELOPMENT_BRANCH=patch-051-private-awning-binding-provisioning' docs/handoff/CURRENT_STATE.md
grep -Fq 'PATCH051_SOURCE_COMMIT=b3b8e6f5b3cbb060ae3735819edadf9aa753c3a6' docs/handoff/CURRENT_STATE.md
grep -Fq 'PATCH051_PR=83' docs/handoff/CURRENT_STATE.md
grep -Fq 'PATCH051_SOURCE_BUILD_FIRMWARE_SHA256=39092b5039b5180148f8a26cd7a6ae7308827384d0304a1723c7ea3f725a864e' docs/handoff/CURRENT_STATE.md
grep -Fq 'Patch051 Pre-Merge Lock - Private Awning Binding Provisioning Foundation' docs/handoff/HANDOFF.md
grep -Fq '## Patch051 Active Boundary' docs/handoff/MASTER_INDEX.md
grep -Fq 'Status: `ACTIVE / PR #83 / BUILD_VALIDATED / PRE_MERGE_LOCKED`' docs/history/PATCH_HISTORY.md
grep -Fq 'PATCH051_PRE_MERGE_LOCK=ACTIVE' docs/history/PATCH_051_PRIVATE_AWNING_BINDING_PROVISIONING_FOUNDATION.md
echo 'PATCH051_PRE_MERGE_DURABLE_STATE_LOCK=PASS'

# Patch051 V10A firmware evidence model guard.
grep -Fq 'operation and firmware flash remain outside the lock gate. Lock-head build' docs/handoff/MASTER_INDEX.md
grep -Fq 'identity is recorded separately from the accepted source-build image because' docs/handoff/MASTER_INDEX.md
grep -Fq 'PATCH051_SOURCE_BUILD_FIRMWARE_SHA256=' docs/handoff/CURRENT_STATE.md
if grep -Fq 'lock validation build must reproduce the accepted firmware SHA-256' docs/handoff/HANDOFF.md; then
  echo 'PATCH051_LOCK_BINARY_EQUIVALENCE_MODEL=FAIL' >&2
  exit 1
fi
echo 'PATCH051_LOCK_BINARY_EQUIVALENCE_MODEL=CORRECTED'

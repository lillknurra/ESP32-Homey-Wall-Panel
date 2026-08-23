#!/bin/sh
set -eu
python3 components/secure_bootstrap/test_host/run_patch031_restore_policy_tests.py
python3 components/secure_bootstrap/test_host/run_athom_oauth_tests.py
python3 components/secure_bootstrap/test_host/run_tests.py
python3 components/secure_bootstrap/test_host/test_patch031_source_invariants.py
python3 components/secure_bootstrap/test_host/test_patch031_diag_cloud_probe_invariants.py
python3 components/secure_bootstrap/test_host/test_patch031_freshness_timeout_invariants.py
SRC=components/secure_bootstrap/athom_oauth_runtime.c
grep -q 'HOMEY_PRESELECT_RESTORE phase=worker result=started' "$SRC"
grep -q 'HOMEY_PRESELECT_RESTORE phase=attempt result=retry' "$SRC"
grep -q 'HOMEY_PRESELECT_RESTORE result=selection_required' "$SRC"
grep -q 'ATHOM_PRESELECT_RESTORE_MAX_ATTEMPTS' "$SRC"
grep -q 'ATHOM_PRESELECT_RESTORE_MAX_ELAPSED_MS' "$SRC"
grep -q 'ATHOM_RESTORE_POLICY_RETRY_TRANSIENT' components/secure_bootstrap/athom_restore_policy.c
grep -q 'athom_restore_policy_should_start_preselection' components/secure_bootstrap/athom_restore_policy.c
grep -q 's_wifi_online = true;' "$SRC"
grep -q 'maybe_start_preselection_restore_worker();' "$SRC"
git diff --check
echo 'PATCH_031_VALIDATION PASS'

#!/bin/sh
validate_patch_011() {
  python3 components/secure_bootstrap/test_host/run_tests.py || return 1
  python3 components/secure_bootstrap/test_host/run_athom_oauth_tests.py || return 1

  grep -q 'homey-panel' components/secure_bootstrap/athom_oauth_runtime.c || return 1
  grep -q 'http://homey-panel.local/oauth/callback' components/secure_bootstrap/include/athom_oauth_config.h || return 1
  grep -q 'ATHOM_OAUTH_STATE_BYTES 32U' components/secure_bootstrap/include/athom_oauth_flow.h || return 1
  grep -q 'ATHOM_OAUTH_ERR_MISSING_STATE' components/secure_bootstrap/include/athom_oauth_flow.h || return 1
  grep -q 'athom_oauth_status_json' components/secure_bootstrap/test_host/test_athom_oauth_flow.c || return 1
  grep -q '/local/' .gitignore || return 1

  if git ls-files --error-unmatch local/athom-oauth.json >/dev/null 2>&1; then
    echo 'FAIL: local/athom-oauth.json is tracked'
    return 1
  fi

  if grep -R -E \
    'synthetic-access-token-not-for-live-use|synthetic-refresh-token-not-for-live-use' \
    components/secure_bootstrap/athom_oauth_* \
    components/secure_bootstrap/include/athom_oauth_* \
    tools/provision_athom_oauth.py \
    docs/history/PATCH_011_LIVE_ATHOM_OAUTH_HOMEY_CONNECTION.md; then
    echo 'FAIL: synthetic token literal entered Patch 011 live scope'
    return 1
  fi

  git diff --check || return 1
  echo 'PATCH_011_VALIDATION PASS'
  return 0
}

validate_patch_011
RC=$?

if [ "$RC" -ne 0 ]; then
  false
else
  true
fi

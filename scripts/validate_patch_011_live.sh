#!/bin/sh
validate_patch_011_live() {
  python3 components/secure_bootstrap/test_host/run_tests.py || return 1
  python3 components/secure_bootstrap/test_host/run_athom_oauth_tests.py || return 1
  python3 components/secure_bootstrap/test_host/run_athom_cloud_model_tests.py || return 1

  grep -q 'ATHOM_TOKEN_MAX 4096U' components/secure_bootstrap/include/athom_cloud_model.h || return 1
  grep -q 'local_url_secure' components/secure_bootstrap/include/athom_cloud_model.h || return 1
  grep -q 'local_url' components/secure_bootstrap/include/athom_cloud_model.h || return 1
  grep -q 'remote_url' components/secure_bootstrap/include/athom_cloud_model.h || return 1
  grep -q 'delegation/token?audience=homey' components/secure_bootstrap/athom_cloud_client.c || return 1
  grep -q '/api/manager/users/login' components/secure_bootstrap/athom_cloud_client.c || return 1
  grep -q '/api/manager/zones/zone' components/secure_bootstrap/athom_cloud_client.c || return 1
  grep -q '/api/manager/devices/device' components/secure_bootstrap/athom_cloud_client.c || return 1
  grep -q 'athom_auth_v2' components/secure_bootstrap/athom_auth_store.c || return 1
  grep -q 'homey.device.control' docs/history/PATCH_011_LIVE_ATHOM_OAUTH_HOMEY_CONNECTION.md || return 1
  grep -q 'phone_provisioning_show_live_ready' components/secure_bootstrap/athom_oauth_runtime.c || return 1
  grep -q 's_live_homey_name' components/secure_bootstrap/phone_provisioning_store.c || return 1
  grep -q 'live_display_restore rendered=true' components/secure_bootstrap/phone_provisioning_store.c || return 1
  grep -q 'zero_secure(s_live_homey_name,sizeof(s_live_homey_name))' components/secure_bootstrap/phone_provisioning_store.c || return 1
  grep -q '^/build_host/$' .gitignore || return 1

  if git ls-files --error-unmatch local/athom-oauth.json >/dev/null 2>&1; then
    echo "FAIL: local/athom-oauth.json is tracked"
    return 1
  fi

  git diff --check || return 1
  echo "PATCH_011_LIVE_VALIDATION PASS"
  return 0
}

validate_patch_011_live
RC=$?
if [ "$RC" -ne 0 ]; then false; else true; fi

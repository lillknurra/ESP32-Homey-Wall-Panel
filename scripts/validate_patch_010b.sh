#!/bin/sh
set -eu
expected='components/secure_bootstrap/CMakeLists.txt
components/secure_bootstrap/include/phone_provisioning.h
components/secure_bootstrap/include/secure_bootstrap.h
components/secure_bootstrap/phone_provisioning_logic.c
components/secure_bootstrap/phone_provisioning_mock_provider.c
components/secure_bootstrap/phone_provisioning_store.c
components/secure_bootstrap/secure_bootstrap_esp.c
components/secure_bootstrap/test_host/run_tests.py
components/secure_bootstrap/test_host/test_phone_provisioning.c
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_010B_PHONE_PROVISIONING_RUNTIME_FOUNDATION.md
main/main.c
scripts/validate_patch_010b.sh'
actual=$(git status --porcelain | sed 's/^...//' | sort)
[ "$actual" = "$(printf '%s\n' "$expected" | sort)" ] || { echo "scope mismatch"; printf '%s\n' "$actual"; exit 1; }
python3 components/secure_bootstrap/test_host/run_tests.py
grep -q 'PHONE_PROV_UNPROVISIONED' components/secure_bootstrap/include/phone_provisioning.h
grep -q 'staging_v1' components/secure_bootstrap/phone_provisioning_store.c
grep -q 'active_v1' components/secure_bootstrap/phone_provisioning_store.c
! grep -R -E 'api\.athom\.com/oauth|client_secret[[:space:]]*=' components/secure_bootstrap main
git diff --check
echo 'PATCH_010B_VALIDATION PASS'

# PATCH010B_C_DECLARATION_GUARD
python3 - <<'PY_GUARD'
from pathlib import Path
p=Path('components/secure_bootstrap/phone_provisioning_store.c')
s=p.read_text(encoding='utf-8')
required=[
 'static bool s_wifi_online;',
 'static bool s_persistence_restored;',
 'static bool s_display_ready_rendered;',
 'static bool s_display_homey_name_present;',
]
for decl in required:
    if decl not in s:
        raise SystemExit('missing declaration: '+decl)
    symbol=decl.split()[-1].rstrip(';')
    if s.index(decl) > s.find(symbol, s.index(decl)+len(decl)) and s.find(symbol, s.index(decl)+len(decl)) >= 0:
        pass
    first_use=s.find(symbol)
    if first_use != s.index(decl)+len('static bool '):
        raise SystemExit('symbol used before declaration: '+symbol)
if "'\\\\0'" in s:
    raise SystemExit("invalid multi-character null literal: '\\\\0'")
if "homey_name[0]!='\\0'" not in s:
    raise SystemExit('expected single-character null comparison missing')
print('PATCH010B_C_DECLARATION_GUARD PASS')
PY_GUARD

# PATCH010B_CHANGE_REFRESH_GUARD
python3 - <<'PY_CHANGE_GUARD'
from pathlib import Path
logic=Path('components/secure_bootstrap/phone_provisioning_logic.c').read_text(encoding='utf-8')
store=Path('components/secure_bootstrap/phone_provisioning_store.c').read_text(encoding='utf-8')
test=Path('components/secure_bootstrap/test_host/test_phone_provisioning.c').read_text(encoding='utf-8')
required=[
 'phone_prov_change_refresh(&s_ctx,phone_prov_mock_provider())',
 'PHONE_PROV change_candidates count=%u',
 'static bool s_change_flow;',
 'Homey-alternativen har lästs in på nytt',
 'test_change_refresh_repopulates_candidates',
]
for token in required:
    if token not in store and token not in test:
        raise SystemExit('missing change-refresh token: '+token)
if 'phone_prov_change_refresh' not in logic:
    raise SystemExit('change-refresh implementation missing')
if 'phone_prov_change(&s_ctx)' in store:
    raise SystemExit('legacy change handler remains')
print('PATCH010B_CHANGE_REFRESH_GUARD PASS')
PY_CHANGE_GUARD

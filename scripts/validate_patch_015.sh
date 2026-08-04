#!/bin/sh
BASE=5479957fa79de9c6f85c083c1123ad14870b13b5
BRANCH=patch-015-private-homey-alias-provisioning
PKG=/Users/petter/Downloads/patch_015_final_correction_package
[ "$(git branch --show-current)" = "$BRANCH" ] || exit 1
[ "$(git rev-parse HEAD)" = "$BASE" ] || exit 1
[ "$(git rev-parse origin/main)" = "$BASE" ] || exit 1
[ -z "$(git diff --cached --name-only)" ] || exit 1
python3 components/secure_bootstrap/test_host/run_panel_homey_alias_store_tests.py || exit 1
python3 components/secure_bootstrap/test_host/run_panel_homey_read_snapshot_tests.py || exit 1
python3 components/secure_bootstrap/test_host/run_panel_ui_tests.py || exit 1
git diff --check || exit 1
python3 -c 'import pathlib,subprocess,sys; b="5479957fa79de9c6f85c083c1123ad14870b13b5"; p=pathlib.Path("/Users/petter/Downloads/patch_015_final_correction_package"); a=set((p/"allowed_paths.txt").read_text().splitlines()); l=set((p/"locked_paths.txt").read_text().splitlines()); t=set(subprocess.check_output(["git","diff","--name-only",b],text=True).splitlines()); u=set(subprocess.check_output(["git","ls-files","--others","--exclude-standard"],text=True).splitlines()); c={x for x in t|u if x}; bad=c-a; touched=c&l; print("PATCH_015_CHANGED_FILES="+str(len(c))); print("PATCH_015_SCOPE="+("PASS" if not bad else "FAIL:"+",".join(sorted(bad)))); print("PATCH_015_LOCKED="+("PASS" if not touched else "FAIL:"+",".join(sorted(touched)))); sys.exit(0 if not bad and not touched else 1)' || exit 1
python3 -c 'from pathlib import Path; p=Path("components/secure_bootstrap/secure_bootstrap_esp.c").read_text(); h=p.index("static esp_err_t wipe_homey_account_state"); e=p.index("static void wipe_task",h); b=p[h:e]; calls=[b.find("athom_cloud_alias_invalidate();"),b.find("panel_homey_alias_store_wipe();"),b.find("athom_auth_store_wipe();")]; assert min(calls)>=0 and calls==sorted(calls); w=p[e:]; assert w.count("wipe_homey_account_state()")>=1; print("PATCH_015_ACCOUNT_WIPE=PASS")' || exit 1
[ "$(grep -R -n -E 'ESP_LOG.*(raw_device|raw_capability|access_token|refresh_token|homey_session_token)' components/secure_bootstrap --include='*.c' --include='*.h' | wc -l | tr -d ' ')" = "0" ] || exit 1
[ "$(git diff "$BASE" -- components/secure_bootstrap | grep -E '^\+[^+].*(HTTP_METHOD_(PUT|PATCH|DELETE)|/api/manager/flow|/api/manager/moods|oauth.*scope)' | wc -l | tr -d ' ')" = "0" ] || exit 1
grep -q 'Patch 013 runtime remains NOT RUN' docs/handoff/HANDOFF.md || exit 1
grep -q 'Package 3B remains NOT_STARTED' docs/handoff/HANDOFF.md || exit 1
! git diff "$BASE" -- docs | grep -q 'Patch 014B' || exit 1
echo PATCH_015_STATIC_VALIDATION=PASS

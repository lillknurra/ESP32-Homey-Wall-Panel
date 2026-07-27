# Current State

- Stable branch: `main`
- Stable merge base: `a8c460873432f930d7632de298d2e87e9d0848f1`
- Active patch: `Patch 010B - Phone Provisioning Runtime Foundation`
- Active branch: `patch-010b-phone-provisioning-runtime-foundation`
- Pull request: `#14`
- Published implementation commit: `1f315120d3c2b88e9880f19ca0a0dc0a3517f23c`
- Status: `PUBLISHED / CHANGES REQUIRED / REVIEW FIX IN PROGRESS / NOT MERGED`

Patch 010B implements synthetic phone provisioning, explicit Homey selection, fail-closed session handling, atomic NVS publication, reboot restore, Homey switching and Homey-only wipe. Real Athom OAuth, real credentials, live Homey traffic and Homey mutation remain `NOT RUN`.

Accepted evidence before review fix: host tests, validator, ESP-IDF v6.0.1 build and hardware/runtime `PASS`; firmware SHA-256 `6db741383507326f5d7e83ae52802594bf454afc0ee174d78312b3cb3360618d`; source-diff SHA-256 `08cc45b88a358feae1a52147df0fc7caf2924f42845c9f8547f97cc73d6da748`.

Review-fix local validation: `PASS` (host tests, Patch 010B validator, declaration guard, Change Homey refresh guard, review-hardening guard, `git diff --check`, secrets review, ESP-IDF v6.0.1 reconfigure, targeted secure_bootstrap C preflight and full build). Review-fix source-evidence SHA-256 over the five code/test/validator diff files: `ac8efbb1e8d59fe6f59821d3b7f7f0cca0c427ee5b38639a35d4c361487edaa5`. Review-fix firmware SHA-256: `a24fb73d38ac254f5288d0f2c1ec207ecd6a2ec625246eda546c4b3b6b4c8416`. Hardware/runtime regression for the review fix: `NOT RUN`. Status remains `CHANGES REQUIRED / NOT MERGED` pending renewed diff review and bounded hardware/runtime regression. No merge, cleanup or later patch is authorized.

# Patch 010B - Phone Provisioning Runtime Foundation

Purpose: synthetic phone provisioning, explicit exact-ID Homey selection, fail-closed sessions, atomic NVS publication, reboot restore, Homey switching and Homey-only wipe.

- Base: `a8c460873432f930d7632de298d2e87e9d0848f1`
- Branch: `patch-010b-phone-provisioning-runtime-foundation`
- Initial published commit: `1f315120d3c2b88e9880f19ca0a0dc0a3517f23c`
- PR: `#14`
- Status: `PUBLISHED / CHANGES REQUIRED / REVIEW FIX IN PROGRESS / NOT MERGED`

No real Athom credentials, login, endpoints, OAuth exchange, live Homey traffic or mutation are implemented. Encrypted NVS, Secure Boot, flash encryption, eFuse and anti-rollback remain `NOT RUN`.

Accepted pre-review evidence: host/static/build/hardware runtime `PASS`; firmware SHA-256 `6db741383507326f5d7e83ae52802594bf454afc0ee174d78312b3cb3360618d`; source-diff SHA-256 `08cc45b88a358feae1a52147df0fc7caf2924f42845c9f8547f97cc73d6da748`.

Review fix: explicit failure for missing/mismatched ACTIVE readback, initial candidate-count bounds, deterministic valid-record mismatch coverage, correct Wi-Fi preservation status and durable-state alignment. Local review-fix validation is `PASS`: host tests, validator and guards, secrets review, `git diff --check`, ESP-IDF v6.0.1 reconfigure, targeted C preflight and full build. Review-fix source-evidence SHA-256 over the five code/test/validator diff files: `ac8efbb1e8d59fe6f59821d3b7f7f0cca0c427ee5b38639a35d4c361487edaa5`. Review-fix firmware SHA-256: `a24fb73d38ac254f5288d0f2c1ec207ecd6a2ec625246eda546c4b3b6b4c8416`. Hardware/runtime regression for the review fix remains `NOT RUN`; status remains `CHANGES REQUIRED / NOT MERGED` until renewed diff review and bounded hardware/runtime evidence are accepted.

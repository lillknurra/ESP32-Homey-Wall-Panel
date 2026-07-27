# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

Patch 010B is active on `patch-010b-phone-provisioning-runtime-foundation`, based on `a8c460873432f930d7632de298d2e87e9d0848f1`, published as commit `1f315120d3c2b88e9880f19ca0a0dc0a3517f23c` in draft PR #14. Status: `PUBLISHED / CHANGES REQUIRED / REVIEW FIX IN PROGRESS / NOT MERGED`.

The bounded review fix hardens ACTIVE readback, candidate-count bounds, deterministic host coverage, Wi-Fi preservation status and durable state documentation. Real Athom OAuth, live Homey traffic and real credentials remain `NOT RUN`.

Local review-fix gates are `PASS`. Source-evidence SHA-256: `ac8efbb1e8d59fe6f59821d3b7f7f0cca0c427ee5b38639a35d4c361487edaa5`; firmware SHA-256: `a24fb73d38ac254f5288d0f2c1ec207ecd6a2ec625246eda546c4b3b6b4c8416`. Hardware/runtime regression for the review fix remains `NOT RUN`. Immediate next work is renewed full diff review followed by the bounded hardware/runtime regression. Do not commit, push, mark ready, merge or clean up until that evidence is accepted.

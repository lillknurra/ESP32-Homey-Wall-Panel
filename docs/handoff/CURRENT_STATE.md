# Current State

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Stable branch: `main`
- Stable baseline at Patch 010A start: `95de8fe0bf903f2c30cc8cf134661c8e7fd33bad`
- Latest completed implementation: `Patch 009 - Offline Athom Session Parsing and Homey Selection`
- Latest repository finalization: `Patch 009A - Post-Merge Finalization`
- Patch 009A status: `COMPLETE / MERGED / SELF-FINALIZING`
- Patch 009A merge commit: `95de8fe0bf903f2c30cc8cf134661c8e7fd33bad`
- Active patch: `Patch 010A - Athom OAuth and Homey Selection Architecture`
- Active branch: `patch-010a-athom-oauth-homey-selection-architecture`
- Active patch status: `DOCUMENTATION-ONLY`

## Patch 010A scope

Patch 010A records the high-level account, phone-provisioning, panel-owned token state, first Homey selection, later Homey switching, account-change and open OAuth boundaries. It changes exactly eight documentation files and no source, test, CMake, firmware or configuration files.

## Evidence boundaries

- Documentation and static validation: REQUIRED
- Real Athom OAuth: NOT RUN
- Live Homey discovery and inventory traffic: NOT RUN
- Homey mutation: NOT RUN
- Real Athom/Homey protocol compatibility: NOT VERIFIED
- ESP-IDF build, flash, serial, runtime and hardware validation: NOT IN SCOPE
- Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS and anti-rollback: NOT RUN

## Finalization rule

Patch 009A is self-finalizing. No Patch 009B or other patch may be created solely to record Patch 009A's own merge SHA.

## Next work

After Patch 010A, the proposed next step is `Patch 010B - Verify Athom OAuth support`.


## Patch 010B active implementation

Patch 010B adds synthetic phone provisioning runtime, host tests, ESP-IDF build, flash, phone, persistence and wipe evidence before publication. Real Athom OAuth and live Homey traffic remain NOT RUN.

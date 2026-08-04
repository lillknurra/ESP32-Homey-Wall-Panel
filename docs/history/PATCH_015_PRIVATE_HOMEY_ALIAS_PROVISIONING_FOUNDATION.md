# Patch 015 – Private Homey Alias Provisioning and Read-Only Dashboard Activation Foundation

Status: IMPLEMENTED_BUILD_SIZE_FLASH_RUNTIME_VISUAL_PASS_AWAITING_COMMIT.
Baseline: `5479957fa79de9c6f85c083c1123ad14870b13b5`.

Introduces a private `homey_alias_v1` dual-slot NVS store with schema version, generation, CRC32 and readback-before-activation. At most six explicit raw device/capability pairs are stored locally and resolved to the existing sanitized Patch 013/014 aliases. Records are bound to a SHA-256 digest of the exact selected Homey identity and fail to `NOT_CONFIGURED` when absent, corrupt or mismatched. No raw identifiers belong in Git, UI, logs or evidence. Homey mutation, Package 3B and Patch 013 runtime remain out of scope.

<!-- PATCH_015_RUNTIME_VISUAL_GATE_BEGIN -->
## Patch 015 verified gate status

- Implementation: **COMPLETE**
- Host tests and static validation: **PASS**
- ESP-IDF build: **PASS**
- ESP-IDF size gate: **PASS**
- Flash: **PASS**
- Patch 015 runtime visual gate: **PASS**
- Display immediately after automated serial capture: visually corrupted
- Panel restarted without serial logging: display stable and correct
- Root cause assessment: automated serial capture affected DTR/RTS/reset timing during startup; the observed corruption was not caused by Patch 015 firmware, display code, or UI code
- Display/UI implementation changes required: **none**
- Future runtime gates: do not automatically open the serial port after flash and do not use automatic serial capture that may toggle DTR/RTS during display startup
- Patch 013 runtime remains **NOT RUN**
- Package 3B remains **NOT_STARTED**
- Commit: **NOT RUN**
- Push: **NOT RUN**
- Pull request: **NOT RUN**
<!-- PATCH_015_RUNTIME_VISUAL_GATE_END -->

# Patch History

## Patch 001 - Bootstrap

- Status: VALIDATED / PUBLISHED ON ACTIVE BRANCH
- Branch: `agent/bootstrap-project`
- Commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Purpose: establish a Project-Template-derived repository structure and minimal buildable ESP-IDF application.
- Non-goals: GPIO mapping, display initialization, touch initialization, peripheral drivers, UI, networking behavior, provisioning, Homey integration, enclosure, and production configuration.

## Patch 002 - Primary Hardware Evidence Capture

- Status: STATIC VALIDATION PASS / COMMITTED / PUBLISHED ON ACTIVE BRANCH / NOT MERGED TO MAIN
- Branch: `agent/bootstrap-project`
- Starting commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Commit: `e42283df9e256cca32cc1790ff8f14824edc7365`
- Remote verification: `origin/agent/bootstrap-project` matched the commit at the Patch 003 baseline audit.
- Purpose: source-pin and classify exact-board hardware facts before implementation.
- Non-goals: firmware, drivers, LVGL, `main/`, `sdkconfig.defaults`, flashing, and hardware claims.
- ESP-IDF build: NOT IN SCOPE
- Runtime: NOT RUN
- Hardware: NOT RUN
- Homey integration: NOT RUN
- Validator: `scripts/validate_patch_002.sh`
- Detailed record: `docs/history/PATCH_002_PRIMARY_HARDWARE_EVIDENCE.md`

## Patch 003 - Display UX and Control Architecture

- Status: IMPLEMENTED LOCALLY / STATIC VALIDATION PASS / UNCOMMITTED
- Branch: `agent/bootstrap-project`
- Starting commit: `e42283df9e256cca32cc1790ff8f14824edc7365`
- Purpose: define panel UX, control, identity, profile, binding, fallback, diagnostics, and Homey inventory contracts without runtime implementation.
- Non-goals: Node.js implementation, Homey access, resolved bindings, LVGL, Wi-Fi, display, touch, firmware changes, and runtime or hardware claims.
- ESP-IDF build: NOT IN SCOPE
- Runtime: NOT RUN
- Hardware: NOT RUN
- Homey integration: NOT RUN
- Protocol: NOT RUN
- Firmware: NOT MODIFIED
- Intended commit: `docs: define display UX and control architecture`
- Validator: `scripts/validate_patch_003.sh`
- Detailed record: `docs/history/PATCH_003_DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`

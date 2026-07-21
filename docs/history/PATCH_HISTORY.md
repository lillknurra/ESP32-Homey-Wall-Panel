# Patch History

## Patch 001 - Bootstrap

- Status: VALIDATED / PUBLISHED ON ACTIVE BRANCH
- Branch: `agent/bootstrap-project`
- Commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Purpose: establish a Project-Template-derived repository structure and minimal buildable ESP-IDF application.
- Non-goals: GPIO mapping, display initialization, touch initialization, peripheral drivers, UI, networking behavior, provisioning, Homey integration, enclosure, and production configuration.

## Patch 002 - Primary Hardware Evidence Capture

- Status: IMPLEMENTED LOCALLY / STATIC VALIDATION PASS / UNCOMMITTED
- Branch: `agent/bootstrap-project`
- Starting commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- Purpose: source-pin and classify exact-board hardware facts before implementation.
- Non-goals: firmware, drivers, LVGL, `main/`, `sdkconfig.defaults`, flashing, and hardware claims.
- Intended commit: `docs: capture primary hardware evidence`
- Validator: `scripts/validate_patch_002.sh`
- Detailed record: `docs/history/PATCH_002_PRIMARY_HARDWARE_EVIDENCE.md`

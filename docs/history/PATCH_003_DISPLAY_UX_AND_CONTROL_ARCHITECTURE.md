# Patch 003 - Display UX and Control Architecture

## Baseline

- Branch: `agent/bootstrap-project`
- Base commit: `e42283df9e256cca32cc1790ff8f14824edc7365`
- Remote baseline: `origin/agent/bootstrap-project` matched the base commit
- Worktree at baseline audit: clean
- Intended commit message: `docs: define display UX and control architecture`

## Purpose

Define the durable UX and control architecture for two ESP32 Homey wall panels
without implementing firmware or Homey integration.

The patch locks:

- responsibility boundaries and direct runtime communication between ESP32 and Homey;
- stable panel identity and profile resolution;
- the first two-column by three-row favorites dashboard;
- three awning, one security, and two lighting widgets;
- interaction, confirmation, state, fallback, and diagnostic contracts;
- allowed Homey binding types;
- the read-only Homey inventory contract;
- a Git-safe unresolved panel binding baseline.

## Files

Created:

- `docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`
- `docs/architecture/HOMEY_INVENTORY_CONTRACT.md`
- `config/panel_binding.yaml`
- `docs/history/PATCH_003_DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md`
- `scripts/validate_patch_003.sh`

Updated:

- `docs/architecture/DESIGN_DECISIONS.md`
- `docs/handoff/CURRENT_STATE.md`
- `docs/handoff/HANDOFF.md`
- `docs/handoff/MASTER_INDEX.md`
- `docs/history/PATCH_HISTORY.md`

## Non-goals

Patch 003 does not include:

- Node.js source or npm packages;
- Homey Personal Access Token or OAuth implementation;
- actual Homey connection, inventory, or command execution;
- raw Homey IDs, guessed capabilities, or resolved bindings;
- LVGL, Wi-Fi, display, touch, or firmware implementation;
- changes to `main/`, `components/`, `CMakeLists.txt`, `sdkconfig.defaults`,
  `partitions.csv`, `package.json`, or `package-lock.json`;
- runtime, hardware, protocol, firmware, or Homey integration PASS claims.

## Design decisions

Patch 003 records DD-002 through DD-011 in
`docs/architecture/DESIGN_DECISIONS.md` and defines the detailed contracts in
the two new architecture documents.

## Validation

Required commands:

```bash
bash -n scripts/validate_patch_003.sh
bash scripts/validate_patch_003.sh
git diff --check
git status --short
git diff --name-only
git diff --stat
git diff
```

The validator checks all ten Patch 003 files, YAML structure, canonical panel
and profile invariants, six exact slot numbers, widget-type counts, unresolved
bindings, documented binding types, modified/staged/untracked paths, forbidden
implementation and package paths, secret-like values across all ten artifacts,
Patch 002 status correction, and Patch 003 evidence boundaries. It also reports
a controlled installation hint when PyYAML is unavailable.

## Evidence status after local validation

- Documentation: IMPLEMENTED LOCALLY
- Static validation: PASS
- Secrets review: PASS
- ESP-IDF build: NOT IN SCOPE
- Runtime: NOT RUN
- Hardware: NOT RUN
- Homey integration: NOT RUN
- Protocol: NOT RUN
- Firmware: NOT MODIFIED

## Completion criteria

Patch 003 is ready for local commit review only when:

- all required files exist;
- shell syntax and the Patch 003 validator pass;
- `git diff --check` passes;
- no out-of-scope file is changed;
- no secret or raw Homey identifier is present;
- the complete diff has been reviewed;
- evidence claims remain limited to documentation and static validation.

Publication, remote verification, merge, and merged-baseline verification are
separate later steps.

## Rollback

Before commit, restore updated files and remove the five new files. After a
Patch 003 commit, use a normal `git revert` of that commit. No Homey, firmware,
or hardware rollback is required because Patch 003 makes no external or
runtime changes.

## Next work

Patch 004 remains Minimal Platform Bring-up and I2C Discovery. The Homey
inventory exporter should be implemented in a later separate patch series so
host tooling and firmware/hardware bring-up do not share one patch purpose.

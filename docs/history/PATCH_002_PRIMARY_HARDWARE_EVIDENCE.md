# Patch 002 - Primary Hardware Evidence Capture

## Purpose

Create a durable, source-pinned hardware evidence baseline before firmware or driver implementation.

## Baseline

- Branch: `agent/bootstrap-project`
- Required starting commit: `fe153af0996bfafb3fc4a5f529e25b0eb4a88412`
- ESP-IDF project baseline: v6.0.1

## Scope

- catalog official Waveshare wiki, schematic, demo archive, and BSP sources;
- classify every recorded claim;
- document pins, display, touch, PMIC/expander, memory, and audio architecture;
- preserve official-source conflicts and unknowns;
- add deterministic static validation.

## Non-goals

- firmware or driver implementation;
- LVGL integration;
- editing `main/`;
- editing `sdkconfig.defaults`;
- choosing a display timing through assumption;
- flashing or hardware execution;
- build, runtime, integration, or hardware PASS claims.

## Evidence

Evidence Capture phase 2 used the official demo archive with SHA-256:

`a9025815a1e51b81dcdc1936271c5bab528fbfc7ea37cd0a790dcb9a81e7e84f`

The preserved user package has SHA-256:

`83528644cac61518a38ca67f315fc19a97851b5dea616d224fa4281d7d3959ff`

## Important conflict

The exact-board Arduino examples and current Waveshare BSP use different RGB porch and pulse timing tuples. Both are retained as `EXAMPLE_DEPENDENT`; Patch 002 does not silently select one.

## Validation

`scripts/validate_patch_002.sh` checks:

- expected files;
- evidence-class vocabulary;
- required source hashes and commit identifiers;
- absence of `HARDWARE_VALIDATED` factual claims;
- no patch changes under `main/` or to `sdkconfig.defaults`;
- exact changed-path scope;
- shell syntax.

## Completion criteria

Patch 002 is ready for commit only after the local validator passes and the full diff is reviewed. Publication and remote verification are separate user-authorized steps.

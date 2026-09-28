# Patch052 - Read-Only Inventory Refresh Recovery Gate Decoupling

## Purpose

Allow the existing read-only Homey inventory refresh path to recover strict
live readiness after that readiness has been cleared, without weakening any
Homey write, light-toggle, awning-provisioning or other mutation gate.

## Verified Base and Merge

- base / source parent: `a7135c07aaad47ab22a3c57e654c3b3521611854`;
- source commit: `34ce797bf66e9ca999a48090ecc582e7da1dcc19`;
- source tree: `47ca3ccdf7a9c227df821cde6103df73b50c0376`;
- PR: `#85`;
- actual merge: `93f7cb206b67609c5c8a5539686c53afab6d0c4d`;
- merged tree: `47ca3ccdf7a9c227df821cde6103df73b50c0376`;
- stable branch after merge: `main`.

## Exact Implementation Scope

- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/include/phone_provisioning.h`;
- `components/secure_bootstrap/phone_provisioning_store.c`;
- `components/secure_bootstrap/test_host/test_patch052_read_only_refresh_recovery_invariants.py`.

## Design Boundary

Patch052 changes only the read-only inventory-refresh recovery prerequisite.
The queue requires Wi-Fi online state, an exact selected Homey identity, an
existing Homey session token and the command queue, while preserving the
existing network-phase and worker arbitration.

A successful inventory fetch restores strict live readiness only after
`homey_inventory_result_verified()` accepts the authoritative inventory result.
Failure keeps the Homey data state in error and does not promote readiness.

The existing light dispatch, light queue, `/homey/lights` provisioning access
and `/homey/awnings` provisioning access continue to require strict
`phone_provisioning_homey_runtime_ready()` readiness. Patch052 introduces no
awning write path and performs no Homey mutation.

## Accepted Validation

- source review: `PASS`;
- security/privacy review: `PASS`;
- write-gate regression review: `PASS`;
- live-operation arbitration review: `PASS`;
- host/source regressions: `PASS`;
- tracked Patch052 diff SHA-256: `2d91ade02de6d4f91704a4a3b18623bcf743dd45e4bf43c6c03f5a5ad7a67b91`;
- Patch052 regression-test SHA-256: `f1d36f27d55489ecbd58c211232b04efcf32d5eb43744079c44fc8241f30259a`;
- canonical ESP-IDF `v6.0.1` build: `PASS`;
- canonical-build evidence ZIP SHA-256: `a50a6464b2071227ac2b99bd23b829d24abbf0928ed00fd53f04b6fb4856c4f6`;
- canonical firmware SHA-256: `ca5bc2debad7cc7e1d2f5d638f151206a43441fd7a9dd1537549ac5e23a5f8b1`;
- canonical firmware size: `1597008` bytes;
- matched-baseline build: `PASS`;
- matched-build evidence ZIP SHA-256: `69c95e854d0cb22bdc3ecac40ca6f42819c353016edc28fa657bbfc1bab183f2`;
- matched current-main baseline firmware SHA-256: `c469e81de28c8b2b4275c5d4c4287b82bebc1e4596872e4bee59d6b279d7cc28`;
- matched Patch052 firmware SHA-256: `005447be8846c41a9affd2a81eb470e7502e813101b95da710217f5090129b6d`;
- matched baseline size: `1597008` bytes;
- matched Patch052 size: `1597008` bytes;
- patched-minus-baseline size: `0` bytes.

The historical Patch051 firmware-size difference is not attributable to
Patch052: under the same ESP-IDF, target, canonical SDKCONFIG and dependency
snapshot, current-main baseline and Patch052 have equal firmware size.

## Runtime and Safety Boundary

- Patch052 runtime validation: `NOT_RUN`;
- firmware flash for Patch052 validation: `NOT_RUN`;
- serial operation: `NOT_RUN`;
- Homey HTTP/runtime operation: `NOT_RUN`;
- Homey mutation/write: `NOT_RUN`;
- alias provisioning: `NOT_RUN`;
- awning write: `NOT_RUN`;
- light toggle: `NOT_RUN`;
- Flow/Advanced Flow execution: `NOT_RUN`;
- CI/check classification at publication and post-merge verification:
  `NO_CHECKS_REPORTED`.

Build and static evidence do not constitute runtime or hardware evidence.

## Completion

Patch052 is `COMPLETE / MERGED / BUILD_VALIDATED` at merge `93f7cb206b67609c5c8a5539686c53afab6d0c4d` and tree
`47ca3ccdf7a9c227df821cde6103df73b50c0376`. Patch052A records this durable state without changing firmware,
source, tests or runtime behavior.

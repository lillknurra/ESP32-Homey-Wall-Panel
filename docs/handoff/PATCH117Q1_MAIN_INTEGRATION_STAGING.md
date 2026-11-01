# Patch117Q1 main integration staging – 2026-11-01

## Scope and source

This is a local integration candidate prepared above public `main`
`9c6f6ccccae125707a22d9a5ca755a2be6693298`. It snapshots the complete public
Patch117Q1 source without importing unpublished local history. Canonical local
`main`, the original Patch117Q/Q1 worktrees, and installed firmware are not
modified. Publication, PR, merge, and physical operations are outside scope.

Public Patch117Q1 is `bc32bdf9003087bc43aa2a2b6b4b50beee37bad1`; its parent is
public Patch117Q `b1cbf1c0de0f422c9cad46c2e4ec82df670e75eb`. Patch117Q1's
functional source fixes secure zero-and-free cleanup of the retained Favorites
response buffer when the experimental transport close before Devices fails.
That path remains fail-closed and does not issue the Devices request.

## Runtime and build evidence boundaries

The operator reports Patch117Q installed and a physical observation of Homey
READY, a complete Devices HTTP 200 response, a fresh generation-valid snapshot,
19 zones, and 79 devices. The retained status response reflected periodic
inventory sequence 3. First BOOT_AUTO attempt success and READY provenance were
not verified. Patch117Q1 is offline-validated only and has not been physically
installed.

`CONFIG_PATCH117Q_HOMEY_DEVICES_FRESH_TRANSPORT` defaults OFF. The physically
tested Patch117Q build used ON. A normal build with OFF does not reproduce the
tested transport behavior; integration validation must explicitly select and
verify ON. This is an experiment, not an approved production default.

The staging instructions remove machine-specific paths redacted in the public
snapshot and replace them with checkout discovery via `git rev-parse
--show-toplevel`. Historical notes remain historical and do not override the
current handoff or grant authorization.

## Required validation before any later publication decision

- Confirm all firmware/build blobs match public Patch117Q1.
- Run relevant Q1, transport, auth/recovery, Wi-Fi provisioning, and awning
  regression tests on this candidate.
- Clean-build ESP32-S3 with ESP-IDF v6.0.1 and the Q experiment explicitly ON.
- Verify generated Kconfig, Patch102 12 MHz PCLK source, app partition fit,
  security gates, and privacy of the full candidate diff.

Build evidence does not establish Patch117Q1 runtime behavior. Any future
physical validation requires a separate explicit operator authorization.

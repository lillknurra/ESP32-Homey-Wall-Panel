# Project Instructions

- Project: `ESP32 Homey Wall Panel`
- Repository: `lillknurra/ESP32-Homey-Wall-Panel`
- Default branch: `main`
- Primary target: `ESP32-S3 using ESP-IDF`
- ESP-IDF baseline: `v6.0.1`
- ESP-IDF path: `~/GitHub/esp-idf-v6.0.1`

The canonical local development worktree is the verified repository checkout
on branch `main`. Determine its location with
`git rev-parse --show-toplevel`; do not rely on machine-specific paths. Any
archived reserve checkout is not a default source for new tasks. On the audited
operator machine, the canonical checkout is `/Users/petter/GitHub/ESP32-Homey-Wall-Panel` and the archived
reserve is `/Users/petter/Archives/ESP32-Homey-Wall-Panel/GitHub_Archive_20261028/Reserve-Worktree/ESP32-Homey-Wall-Panel-Current`. Verify the actual branch, HEAD, remote tracking
state and worktree against `docs/handoff/HANDOFF.md` before work.

Every patch defines purpose, non-goals, base branch and commit, expected files,
design decisions, validation, completion criteria, rollback, and next work.

Keep evidence classes separate: documentation, build, runtime, integration,
protocol, firmware, hardware, synchronization, packaging, and measurement.

## Project-specific principles

- The hardware baseline is Waveshare ESP32-S3-Touch-LCD-4B with ESP32-S3-WROOM-1-N16R8.
- Do not guess GPIO mappings, bus timing, peripheral addresses, power-rail sequencing, or vendor initialization details.
- Do not select enclosure or Homey integration protocol without documented requirements and evidence.
- Keep secrets, Wi-Fi credentials, Homey tokens, and device-specific provisioning data out of Git.
- Prefer ESP-IDF-native components and explicit versioning.
- Treat a successful build as build evidence only, not runtime or hardware evidence.

## Local operator environment

- Verify ESP-IDF `v6.0.1` from `~/GitHub/esp-idf-v6.0.1` before build or flash.
  A different toolchain requires a documented reason and explicit approval.
- `MANUAL_BOOT_BUTTON_USE=ABSOLUTELY_FORBIDDEN`. Never press, request, suggest
  or use the physical BOOT button for flashing, connection establishment,
  recovery, diagnostics, provisioning or any future operation. Only an
  independently verified automatic ROM-entry/reset method may be considered,
  and only with explicit authorization for the current task. Do not instruct the
  operator to press a reset button. USB reconnect or power cycling also requires
  explicit authorization for the current task.
- Serial collection must not depend on `Ctrl+]`. Use a bounded logger that waits
  for the USB serial port, closes it correctly and exits automatically.
- Copy/paste commands for the operator must not use shell heredocs.
- Generated ZIP files, logs, reports and validation results belong under
  `~/Downloads/`. A headless or CI environment may use an explicitly configured
  output directory outside the repository.

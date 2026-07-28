# Handoff

`docs/handoff/CURRENT_STATE.md` is authoritative.

Patch 011 - Live Athom OAuth and Homey Connection is active on:

`patch-011-live-athom-oauth-homey-connection`

Base and stable Patch 010B merge commit:

`ae6b16b93777237bf1cb1637f55b8c34bdd86b41`

Current status:

`IMPLEMENTATION COMPLETE / LIVE RUNTIME VERIFIED / DOCUMENTATION FINALIZATION IN PROGRESS / NOT COMMITTED`

## Verified live result

The panel completed the real Athom and Homey connection flow:

- Athom OAuth: `PASS`;
- `/user/me`: `PASS`;
- discovered Homey:
  `Strandängsgatan`;
- selected Homey ID:
  `60bdcc6cfa595c0c05f97f9d`;
- delegation: `PASS`;
- Homey login/session: `PASS`;
- zones: `19`;
- devices: `79`;
- state: `ready`;
- detail after live completion:
  `inventory_complete`;
- last error: `0`.

The working session and selected Homey were persisted. After an ordinary firmware restart, without new OAuth or live-select:

- state restored to `ready`;
- selected Homey restored to `Strandängsgatan`;
- zone count restored to `19`;
- device count restored to `79`;
- display showed:
  `Strandängsgatan`
  and
  `Status: Ansluten`;
- runtime ID changed;
- select attempt reset to `0`.

An empty `homeys` list after restore is expected because discovery candidates are transient. `detail: idle` after restore is expected before a new live operation.

## Important implementation results

- device inventory uses a controlled dynamically growing response buffer;
- initial size:
  `65536` bytes;
- growth:
  `65536 -> 131072 -> 262144 -> 524288`;
- explicit maximum:
  `524288` bytes;
- auth restore is serialized against OAuth and live-select;
- restore is started only once per runtime;
- live-ready state is bridged to the display;
- the display preserves the live Homey name if a later display render occurs.

## Required boundaries

- Never include OAuth client secrets, tokens, authorization headers or response bodies in Git, logs, screenshots or evidence.
- Never flash the full NVS partition or
  `athom-client-config.nvs.bin`
  during normal verification.
- Do not erase saved Wi-Fi.
- Use only ordinary firmware flash when explicitly required.
- Do not commit, push, create a pull request or merge without separate authorization.

## Immediate next action

Run the existing Patch 011 documentation and source validators, relevant host tests and `git diff --check`. Review the complete diff and secret boundary. No additional firmwareflash is required for documentation-only finalization.

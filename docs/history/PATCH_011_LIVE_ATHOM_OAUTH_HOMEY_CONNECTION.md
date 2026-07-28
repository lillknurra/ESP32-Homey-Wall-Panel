# Patch 011 – Live Athom OAuth and Homey Connection

## Purpose
Establish the local callback foundation for real Athom OAuth without external callback infrastructure.

## Base
`main` at `ae6b16b93777237bf1cb1637f55b8c34bdd86b41`.

## Implemented scope

- mDNS hostname `homey-panel.local` after Wi-Fi online;
- local `/homey/login`, `/oauth/callback`, `/homey/live-status`,
  `/homey/live-select` and `/homey/live-refresh` endpoints;
- 256-bit URL-safe OAuth state, TTL, constant-time comparison and one-time consume;
- CRC-protected OAuth client configuration in NVS namespace `athom_cli_cfg`;
- live token exchange and refresh;
- `/user/me` Homey discovery and exact-ID selection;
- delegation token and Homey login/session establishment;
- live zone and device inventory;
- persistent auth/session restore;
- live-ready display integration;
- local NVS image generator that never logs credential values;
- host tests, validators, ESP-IDF build and hardware/runtime verification.

## Explicit boundary

Patch 011 verifies live read-only Homey connection and inventory. Homey mutation execution remains outside scope. Authorization codes, OAuth client secrets, tokens, authorization headers and response bodies must never enter Git, serial logs or evidence packages.

## Validation

Host tests, Patch 011 validators, ESP-IDF v6.0.1 build, ordinary firmware flash, live Athom OAuth, live Homey connection, inventory, restart restore and display integration are verified `PASS`.

## Product deployment model

The first real installation target is two panels connected to one Homey Pro
(Early 2019). This is a physical Homey Pro and is treated as platform `local`.

The application uses one developer-owned Web API Client named
`ESP32 Homey Wall Panel`. End users sign in with their ordinary Homey account.
They do not create API keys, Web API clients, Client IDs, or Client Secrets.

Both panels contain the same pre-provisioned application Client ID and Client
Secret. Each panel performs its own OAuth login and stores its own access token,
refresh token, selected Homey ID, and Homey session token. Resetting one panel
must not invalidate or erase the other panel.

Requested scopes:

- `homey.zone.readonly`
- `homey.device.readonly`
- `homey.device.control`

The connection URL priority for Homey Pro (Early 2019) is:

1. `localUrlSecure`
2. `localUrl`
3. `remoteUrl`

Only URLs returned by `GET https://api.athom.com/user/me` may be used. URLs are
not constructed locally. If all URLs are unavailable, or the cached endpoint
fails persistently, `/user/me` must be refreshed before retrying.

Application credentials are installer-provisioned before normal Wi-Fi and Homey
setup. A complete NVS image may only be used on a new or intentionally reset
panel where initializing NVS is acceptable. Normal firmware updates must not
overwrite NVS. Homey account wipe removes user tokens, selected Homey, and Homey
session while preserving Wi-Fi and `athom_cli_cfg`.


## Final live verification

<!-- PATCH_011_FINAL_RUNTIME_VERIFIED -->

Patch 011 completed the complete live Athom OAuth and Homey connection flow.

### Verified account and Homey flow

- OAuth callback:
  `http://homey-panel.local/oauth/callback`
- Athom OAuth:
  `PASS`
- `/user/me`:
  `PASS`
- discovered and selected Homey:
  `Strandängsgatan`
- exact Homey ID:
  `60bdcc6cfa595c0c05f97f9d`
- delegation:
  `PASS`
- Homey login/session:
  `PASS`
- zone inventory:
  `19`
- device inventory:
  `79`
- final state:
  `ready`
- final detail:
  `inventory_complete`
- final error:
  `0`

### Inventory response handling

The normal HTTP response limit remains 65536 bytes. Device inventory may grow through controlled doubling:

`65536 -> 131072 -> 262144 -> 524288`

The explicit maximum is 524288 bytes. Reaching the maximum or failing allocation returns `ESP_ERR_NO_MEM`; a truncated response is not passed to JSON parsing.

### Auth restore serialization

Auth restore is guarded by explicit running and one-shot state. OAuth callbacks and live-select are rejected while restore is active. Restore cannot be started repeatedly through repeated handler registration and cannot overwrite an active OAuth/select operation.

### Persistence and restart verification

After ordinary firmware restart:

- no new OAuth was required;
- no new live-select was required;
- selected Homey restored to `Strandängsgatan`;
- Homey session restored;
- zone count restored to `19`;
- device count restored to `79`;
- state restored to `ready`;
- display restored to:
  `Strandängsgatan` / `Status: Ansluten`;
- runtime ID changed;
- select attempt reset to `0`.

The discovery candidate list `homeys` is transient and may be empty after restore. `detail: idle` is expected before a new live operation.

### Display integration

The live runtime now notifies the display layer after successful live selection and after auth restore. The display layer retains the live Homey name and prioritizes it over the legacy synthetic provisioning state, preventing the installation view from overwriting the verified ready view.

The Wi-Fi change button behavior is unchanged.

### Operational and security boundaries

- Never flash the complete NVS partition during normal verification.
- Never flash `athom-client-config.nvs.bin` during normal verification.
- Preserve saved Wi-Fi.
- Do not expose OAuth client secrets, tokens, authorization headers or response bodies.
- Homey mutation is not part of Patch 011.
- Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS and anti-rollback are not part of Patch 011.

### Known non-blocking warning

The build may report:

`parse_token_field defined but not used`

This warning does not affect the verified live runtime.

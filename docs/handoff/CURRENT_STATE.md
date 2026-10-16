# Current State

## Authoritative repository state

- `REPOSITORY=lillknurra/ESP32-Homey-Wall-Panel`
- `BRANCH=main`
- PR #94 (verified Patch084 source integration) is `MERGED`.
- Verified Patch084 source merge on `main`: `e9e53fdbcf6d0e29c41965992290a312ab0449b7`.
- Verified Patch084 source tree: `141bfa9f262355a478eeef49934eb8b740b5f51d`.
- Later documentation-only merges may advance `main`; always verify its live HEAD.
  The source merge SHA above is an evidence baseline, not a frozen current HEAD.

## Installed and verified runtime baseline

- Installed application: Patch084, BIN SHA-256
  `440a646b227ce96ecca8699cc230e37e092dc8cad404be4bb5a88de40e5d1e27`.
- The app-only write and image verification passed; no NVS erase or explicit
  NVS write was performed.
- A single read-only runtime diagnostics response reported Homey READY, a
  present/fresh/generation-valid snapshot, completed inventory on boot-auto
  attempt 1, and active aliases for `awning_1`, `awning_2`, and `awning_3`.
  Patch084's special incomplete-data retry was not exercised because the first
  inventory attempt completed successfully.
- The operator visually confirmed widget values, menus, page navigation,
  Solskydd and Balkongmarkis double-blink, settings press feedback, and the
  absence of Saving/Saved dialogs.
- These runtime observations do not establish long-term stability or prove the
  retry path under a real incomplete response. The RGB/full-screen flicker
  remains unresolved.

## Integrated source and validation evidence

- PR #94 integrates the physically verified Patch084 source baseline into
  `main`; it preserves Patch083's bounded boot-auto recovery for
  `ESP_ERR_HTTP_INCOMPLETE_DATA`.
- Patch084's isolated source integration had an ESP-IDF v6.0.1 clean build and
  focused regression validation. The physical runtime and offline build/test
  evidence remain distinct.
- The Patch084 UI changes add visual-only double-blink feedback to the two
  interactive awning widgets (Solskydd / `awning_2`, Balkongmarkis /
  `awning_3`) and immediate settings button/value feedback. They do not add a
  Homey awning command path. The larger Markis (`awning_1`) direction controls
  remain a separate future scope.
- Patch078 Wi-Fi STA/APSTA refresh behavior and Patch079E Wi-Fi provisioning
  protections are part of the integrated source. Physical re-provisioning,
  Wi-Fi credential change, and permanent Wi-Fi commit were not tested in the
  Patch084 runtime check.

## Current priorities

1. Patch085: design and validate safe control for Solskydd (`awning_2`) and
   Balkongmarkis (`awning_3`) using the existing five-step position model.
   Do not enable Homey writes until each capability contract and command
   semantics are verified, safety gates fail closed, and a separate bounded
   authorization explicitly permits physical control.
2. A separate later patch for Markis (`awning_1`) with direction-based
   UPP/STOPP/NED semantics.
3. Investigate the unresolved RGB/full-screen display flicker.
4. Verify long-duration stability and runtime memory margin.
5. Future Verisure integration.

## Open evidence limits and safety boundaries

- `NVS_BACKUP=NOT_VERIFIED`; preserve existing Wi-Fi, Homey, alias and local
  settings. Do not infer that app-only flashing prevents firmware-initiated
  NVS writes during normal runtime.
- Target LVGL pool headroom and long-term runtime memory are not verified.
- The special Patch083/084 incomplete-response retry has offline regression
  evidence but was not activated in the Patch084 physical boot observation.
- Stale GOT_IP/event ordering and physical Wi-Fi reconfiguration behavior retain
  the limits documented by the Patch079 review chain.
- No Homey write, awning movement, Wi-Fi reprovisioning, settings persistence
  test, or other physical mutation is authorized by this state record.
- Historical host-suite failures must retain their recorded status; do not
  promote them to PASS without new accepted evidence.

Historical patch and validation details remain in `docs/history/`, the
Patch084 integration evidence, and the archived task reports. Older PENDING
labels in historical Scratchpad entries describe their original date and are
superseded only by the verified completion statements above.

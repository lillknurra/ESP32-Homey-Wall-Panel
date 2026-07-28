# Current State

- Stable branch: `main`
- Stable Patch 010B merge/base commit:
  `ae6b16b93777237bf1cb1637f55b8c34bdd86b41`
- Active patch:
  `Patch 011 - Live Athom OAuth and Homey Connection`
- Active branch:
  `patch-011-live-athom-oauth-homey-connection`
- Status:
  `IMPLEMENTATION COMPLETE / LIVE RUNTIME VERIFIED / DOCUMENTATION FINALIZATION IN PROGRESS / NOT COMMITTED`

## Patch 011 verified result

Patch 011 now provides a complete live account and Homey connection path:

- local callback:
  `http://homey-panel.local/oauth/callback`;
- live Athom OAuth authorization and token exchange: `PASS`;
- `/user/me` and Homey discovery: `PASS`;
- exact Homey selection:
  `Strandängsgatan`
  (`60bdcc6cfa595c0c05f97f9d`): `PASS`;
- Athom delegation token: `PASS`;
- Homey login and Homey session creation: `PASS`;
- live zone inventory: `19`;
- live device inventory: `79`;
- final live state:
  `ready`;
- final live detail:
  `inventory_complete`;
- final live error:
  `0`;
- selected Homey persistence: `PASS`;
- OAuth/auth and Homey-session restore after ordinary restart: `PASS`;
- restore without new OAuth or new live-select: `PASS`;
- restored zone and device counts: `19` and `79`;
- display transition:
  `Strandängsgatan` / `Status: Ansluten`: `PASS`.

The live inventory response buffer starts at 65536 bytes and can grow in controlled doubling steps to an explicit maximum of 524288 bytes for device inventory. Other OAuth, delegation and login responses retain the normal 65536-byte limit.

Auth restore is serialized against OAuth and live-select. Restore is one-shot per runtime and cannot overwrite an active OAuth or select operation.

After restore:

- `homeys` may be empty because the discovery list is transient;
- `selected_homey`, auth/session and inventory counts remain persistent;
- `detail: idle` is expected before a new live operation;
- `select_attempt: 0` is expected in a new runtime.

## Security and operational boundaries

- OAuth client ID and client secret remain private local configuration.
- Tokens, authorization headers and response bodies must not be logged or committed.
- Ordinary firmware flash is allowed for approved verification.
- The complete NVS partition and
  `athom-client-config.nvs.bin`
  must never be flashed during normal verification.
- Wi-Fi configuration must remain preserved.
- Secure Boot, flash encryption, eFuse writes, production-key provisioning,
  encrypted NVS and anti-rollback remain outside Patch 011.
- Homey mutation execution remains outside Patch 011.

## Known non-blocking issue

The ESP-IDF build may still report:

`parse_token_field defined but not used`

This warning is non-blocking and does not affect the verified runtime result.

## Immediate next work

1. finalize durable Patch 011 documentation;
2. run existing Patch 011 validators and relevant host tests;
3. run `git diff --check`;
4. review the complete diff for secrets and unintended files;
5. do not commit, push, open a pull request or merge without separate authorization.

# Patch 010A - Athom OAuth and Homey Selection Architecture

## Purpose

Define a lean, durable architecture for Athom account use, phone-assisted first setup, panel-owned OAuth state and Homey selection without implementing or claiming live OAuth compatibility.

## Base and branch

- Base: `main` at `95de8fe0bf903f2c30cc8cf134661c8e7fd33bad`
- Branch: `patch-010a-athom-oauth-homey-selection-architecture`

## Exact scope

1. `docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md`
2. `docs/architecture/DESIGN_DECISIONS.md`
3. `docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md`
4. `docs/handoff/CURRENT_STATE.md`
5. `docs/handoff/HANDOFF.md`
6. `docs/handoff/MASTER_INDEX.md`
7. `docs/history/PATCH_HISTORY.md`
8. `docs/history/PATCH_010A_ATHOM_OAUTH_AND_HOMEY_SELECTION_ARCHITECTURE.md`

## Accepted principles

- Use the ordinary Athom/Homey account; create no separate panel account.
- Enter the password only on an Athom-controlled surface.
- Use a phone portal for first setup and first Homey selection.
- Store access token, refresh token, expiry, selected Homey ID and discovery strategy on the panel.
- Present readable names while using exact Homey ID as authority.
- Allow later same-account Homey switching on the panel.
- Require phone provisioning again for account changes.
- Keep callback infrastructure outside normal runtime.
- Do not treat a distributed firmware client secret as confidential.

## Open OAuth questions

Redirect URI formats, local redirect, PKCE, device grant, public/native client support, client-secret requirements, scopes, exact authorization and token endpoints, refresh behavior, revoke/logout and callback/consent behavior remain open.

## Preferred but unverified callback direction

A fixed HTTPS callback, random state, phone-to-panel binding, short-lived one-time code, replay protection and no permanent credential relay are preferred. This is not a verified implementation.

## Risks

- wrong phone or wrong panel;
- replay;
- local-network attacker;
- client-secret extraction;
- token exposure;
- wrong Homey;
- interrupted provisioning.

The architecture fails closed, keeps secrets out of logs and evidence, and publishes no partial configuration as ready.

## Non-goals

No OAuth client registration, endpoint verification, callback server, phone portal, token-storage code, real credentials, live Homey list, Homey mutation, build, flash, serial, runtime, hardware validation, Secure Boot, flash encryption or eFuse work.

## Validation

- exact eight-file scope;
- exactly five new design decisions, DD-015 through DD-019;
- open questions remain explicitly open;
- callback direction is marked unverified;
- no endpoint guesses or credential material;
- `git diff --check` passes;
- full staged scope matches the reviewed diff.

## Rollback

Revert the single Patch 010A documentation commit normally. No runtime, hardware, credential or Homey state exists to roll back.

## Later patch order

1. Patch 010B - Verify Athom OAuth support
2. Patch 010C - Phone portal and callback flow
3. Patch 010D - Token storage and lifecycle
4. Patch 010E - Live Homey list and first selection
5. Patch 010F - Change Homey on panel
6. Patch 010G - Logout, account change and wipe

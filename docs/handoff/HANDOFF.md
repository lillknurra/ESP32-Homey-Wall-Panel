# Handoff

## Authoritative current state

`docs/handoff/CURRENT_STATE.md` owns the stable baseline, active branch, patch status, evidence status and immediate next work.

## Completed baseline

Patch 009 and Patch 009A are complete. Patch 009A was merged through PR #12 at `95de8fe0bf903f2c30cc8cf134661c8e7fd33bad` and is self-finalizing. No Patch 009B is required or permitted solely to record that merge.

## Active Patch 010A context

Patch 010A is documentation-only. It records these principles:

- the ordinary Athom/Homey account is used;
- Homey passwords are entered only on an Athom-controlled surface;
- first setup and first Homey selection use a phone portal;
- the panel owns tokens, expiry, selected Homey ID and discovery strategy;
- readable names are shown, while exact Homey ID is authoritative;
- Homey switching within the same account may later occur on the panel;
- account switching requires phone provisioning again;
- a callback service is excluded from normal runtime;
- an embedded client secret is not treated as confidential.

Redirect formats, local redirect, PKCE, device grant, public/native clients, client-secret rules, scopes, exact endpoints, refresh behavior, revoke/logout and callback/consent behavior remain open. The HTTPS callback and one-time-code model is only a preferred direction.

## Next step

The recommended next patch is `Patch 010B - Verify Athom OAuth support`. Real OAuth and live Homey traffic remain outside Patch 010A.

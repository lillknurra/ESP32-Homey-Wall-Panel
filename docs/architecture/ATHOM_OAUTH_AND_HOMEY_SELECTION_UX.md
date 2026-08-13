# Athom OAuth and Homey Selection UX

## Purpose

This document defines the user-facing setup and Homey-selection flow. Patch 011 verified the live Athom OAuth, callback, Homey discovery, exact selection, session creation, persistence and restart-restore path described below.

## First setup

1. The panel shows a QR code or local portal address.
2. The user opens the portal on a phone.
3. The user chooses **Log in with Homey**.
4. The browser is sent to an Athom-controlled login and consent surface.
5. The panel receives the OAuth result through the provisioning flow without receiving the user's password.
6. The account's available Homey devices are listed by readable name.
7. The user explicitly selects a Homey.
8. The panel verifies and stores the exact Homey ID and discovery strategy.
9. The panel enters normal operation only after the complete token and Homey selection state has been stored successfully.

Even when only one Homey is returned, the permanent selection is explicitly confirmed.

## Change Homey on the panel

The later panel path is:

```text
Settings -> Homey connection -> Change Homey
```

The panel refreshes the available Homey list, presents readable names, verifies the selected exact ID, invalidates the old Homey session and creates a new session for the selected Homey.

## Change account

Changing to another Athom account requires the phone portal again. The existing account state must not be silently reused for the new account.

## Verified callback and state handling

Patch 011 verified the local callback:

`http://homey-panel.local/oauth/callback`

The implemented flow uses a 256-bit URL-safe random OAuth state, bounded TTL, constant-time comparison and one-time consumption. The panel exchanges the code directly with Athom and never receives the user's Homey password.

## Risks and required behavior

- **Wrong phone or wrong panel:** provisioning must be bound to the intended panel and show which panel is being configured.
- **Replay:** state and transfer codes must be short-lived and single-use.
- **Local-network attacker:** the local portal must be available only during a bounded provisioning session and must not expose tokens or raw identifiers.
- **Client-secret extraction:** a secret distributed in firmware is not treated as confidential.
- **Token exposure:** tokens must not enter Git, logs, evidence packages or user-visible diagnostics.
- **Wrong Homey:** readable names are presentation only; the exact ID is verified and stored.
- **Interrupted provisioning:** partial credentials or selections must not be published as a complete configuration.

## Patch 010A historical open questions

At completion of Patch 010A, redirect formats, local redirect, PKCE, device grant, public/native clients, client-secret rules, scopes, exact endpoints, refresh behavior, revoke/logout and callback/consent behavior were intentionally open.

Patch 011 later verified the local redirect, scopes, exact endpoints, authorization-code exchange, refresh and callback/consent behavior used by the panel. PKCE, device grant, public/native client operation, production client-secret distribution and remote revoke semantics remain outside verified scope.

## Patch 011 verified implementation status

Patch 011 verified Athom authorization and token endpoints, required scopes, local callback behavior, authorization-code exchange, token refresh, `/user/me` discovery, exact Homey selection, delegation, Homey session establishment, persistence, restart restore and display transition.

Changing to another Athom account still requires the phone portal. Changing Homey within the same account must invalidate the previous Homey session and establish a new session for the exact selected ID.

PKCE, device authorization grant, public/native client operation without installer-provisioned client configuration, remote revoke semantics, Homey mutation execution and production credential distribution remain outside verified scope.

## Patch019A1.7 lifecycle clarification

Patch019A1.7 does not change the user-facing OAuth or Homey selection flow. It
only changes the internal HTTPS lifecycle between the Cloud delegation phase and
the Homey login phase.

After the selected Homey has been resolved and the Cloud delegation-token request
has succeeded, the firmware closes the live Cloud transport before starting
Homey login. The Athom OAuth state, selected Homey identity, delegation behavior,
phone portal, account-change rules and credential privacy boundaries are
unchanged.

The accepted Patch019A1.7 runtime observed the direct Cloud HTTP 200 path and did
not exercise Cloud refresh, later Cloud reconnect or error-recovery stimuli.
Those paths remain `NOT_OBSERVED` for that evidence window, not failed.

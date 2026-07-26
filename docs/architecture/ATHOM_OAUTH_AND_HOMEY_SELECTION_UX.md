# Athom OAuth and Homey Selection UX

## Purpose

This document defines the user-facing setup and Homey-selection flow. It does not define verified Athom endpoints or implement live OAuth.

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

## Preferred callback direction

A fixed HTTPS callback, random OAuth state, phone-to-panel session binding, short-lived one-time transfer code and replay protection are the preferred direction. This is not yet a verified implementation.

## Risks and required behavior

- **Wrong phone or wrong panel:** provisioning must be bound to the intended panel and show which panel is being configured.
- **Replay:** state and transfer codes must be short-lived and single-use.
- **Local-network attacker:** the local portal must be available only during a bounded provisioning session and must not expose tokens or raw identifiers.
- **Client-secret extraction:** a secret distributed in firmware is not treated as confidential.
- **Token exposure:** tokens must not enter Git, logs, evidence packages or user-visible diagnostics.
- **Wrong Homey:** readable names are presentation only; the exact ID is verified and stored.
- **Interrupted provisioning:** partial credentials or selections must not be published as a complete configuration.

## Open questions

Athom support for redirect formats, local redirect, PKCE, device grant, public/native clients, client-secret rules, scopes, exact endpoints, refresh behavior, revoke/logout and callback/consent behavior remains open.

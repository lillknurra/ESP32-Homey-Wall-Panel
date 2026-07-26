# ESP32-native Athom Cloud architecture

## Decision

The wall panel owns the long-lived Athom OAuth state and uses a limited ESP-IDF-native HTTPS client. A phone is used for first-time provisioning, but no permanent companion app, proxy or credential relay is part of normal runtime.

The user signs in with the same ordinary Athom/Homey account used in the Homey app. No separate panel account is created. The panel and its local portal never request, receive or store the user's Homey password; credential entry occurs only on an Athom-controlled authorization surface.

## Panel-owned state

After successful provisioning the panel owns and stores locally:

- access token;
- refresh token;
- token expiry;
- selected Homey ID;
- discovery strategy.

A Homey is shown to the user by a readable name, but the exact Homey ID is the sole selection authority. Patch 009 already requires explicit selection, rejects duplicate or stale IDs and invalidates the old session before another Homey is used.

## First setup and later changes

First account connection and first Homey selection are completed through a phone-based local portal. Later switching between Homey devices within the same account may be initiated directly on the panel. Switching to another Athom account requires the phone portal again.

## Preferred callback direction - not verified

The preferred direction is a fixed HTTPS callback with:

- cryptographically random OAuth state;
- binding between the phone session and the target panel;
- a short-lived one-time transfer code;
- replay protection;
- secure local transfer to the panel;
- no permanent credential relay.

This is a preferred direction only. It is not a verified Athom-compatible implementation.

## Client-secret boundary

A client secret embedded in distributed ESP32 firmware is not a meaningful product secret and must not be treated as confidential. Private-use provisioning may still place client configuration locally, but distribution and revocation implications remain explicit design concerns.

## Open OAuth questions

The following remain OPEN and must not be assumed:

- accepted redirect URI formats;
- local redirect to the panel;
- PKCE support;
- device authorization grant support;
- public/native client support;
- client-secret requirements;
- required scopes;
- exact authorization endpoint;
- exact token endpoint;
- token refresh and rotation behavior;
- revoke and logout support;
- callback and consent behavior.

No endpoint guesses or live OAuth claims are part of Patch 010A.

## Existing implementation boundary

The current offline implementation includes bounded parsing, token/session publication boundaries, Homey listing and exact-ID selection contracts, discovery-strategy modeling, read-only inventory boundaries and credential wipe interfaces. Real Athom OAuth, live Homey traffic and real protocol compatibility remain NOT RUN or NOT VERIFIED.

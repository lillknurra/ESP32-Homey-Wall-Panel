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

## Verified callback direction after Patch 011

Patch 011 verified the local callback:

`http://homey-panel.local/oauth/callback`

The implemented flow uses cryptographically random 256-bit URL-safe OAuth state, a bounded TTL, constant-time comparison and one-time consumption. The panel exchanges the authorization code directly with Athom and does not receive the user's Homey password.

## Client-secret boundary

A client secret embedded in distributed ESP32 firmware is not a meaningful product secret and must not be treated as confidential. Private-use provisioning may still place client configuration locally, but distribution and revocation implications remain explicit design concerns.

## Patch 010A historical open questions

At completion of Patch 010A, the following were intentionally open and were not assumed:

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

Patch 011 later verified the local callback, required scopes, exact authorization and token endpoints, authorization-code exchange, token refresh and consent flow used by the panel. PKCE, device authorization grant, public/native client operation, production client-secret distribution and remote revoke semantics remain outside verified scope.

## Patch 010A historical implementation boundary

At completion of Patch 010A, the implementation was offline-only and included bounded parsing, token/session publication boundaries, Homey listing and exact-ID selection contracts, discovery-strategy modeling, read-only inventory boundaries and credential wipe interfaces. Real Athom OAuth, live Homey traffic and protocol compatibility were `NOT RUN` or `NOT VERIFIED` within Patch 010A's own evidence boundary.

Patch 011 subsequently verified the live OAuth and Homey connection behavior described below.

## Patch 011 verified OAuth and Homey connection status

Patch 011 verified the Athom authorization and token endpoints, local callback, authorization-code exchange, refresh, `/user/me` discovery, exact Homey selection, delegation, Homey login/session, read-only zone and device inventory, persistent restore and display transition.

Requested scopes are `homey.zone.readonly`, `homey.device.readonly` and `homey.device.control`. Homey URL priority is `localUrlSecure`, then `localUrl`, then `remoteUrl`, using only URLs returned by `/user/me`.

Homey mutation execution, revoke semantics beyond local wipe, PKCE, device authorization grant, public/native client operation without installer-provisioned client configuration, production credential distribution, Secure Boot, flash encryption, eFuse writes, production keys, encrypted NVS and anti-rollback remain outside verified scope.

## Patch017 verified favorites readiness

Patch017 verified the Homey favorites binding path used by the read-only
dashboard. It uses authoritative Homey `favoriteDevices` ordering, gates
dashboard publication on verified live data, retries transient Homey transport
failures and preserves favorite widget state across wake. It does not add Homey
mutation or generic device-control behavior.

## Patch019A1.7 Cloud-to-Homey TLS handoff

Patch019A1.6G proved a persistent Cloud TLS resource conflict with later Homey
TLS setup: `PERSISTENT_CLOUD_TLS_RESOURCE_CONFLICT=PROVEN` and
`FRAGMENTATION_SPECIFIC=NOT_PROVEN`.

Patch019A1.7 keeps persistent Cloud HTTP/TLS within the Cloud phase, then closes
the live Cloud transport at the natural Cloud-to-Homey phase boundary after
delegation and before Homey login. The handoff uses
`esp_http_client_close(s_cloud_http.handle)`, does not call cleanup in the
handoff, preserves the handle and configuration, and remains idempotent.

The accepted fresh live v2 runtime observed the Cloud direct-200 path,
delegation reuse, exactly one successful handoff close and successful Homey
login/session/zones/devices. Optional Cloud refresh, later Cloud reconnect and
error-recovery stimuli were `NOT_OBSERVED` in that runtime window and must not be
classified as failures.

This does not change OAuth, endpoint priority policy, retry semantics, Favorites
behavior, Homey mutation boundaries, sdkconfig, allocator policy, PSRAM policy
or MbedTLS buffer sizes.

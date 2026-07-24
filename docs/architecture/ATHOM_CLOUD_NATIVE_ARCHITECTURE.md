# ESP32-native Athom Cloud architecture

## Decision

The wall panel and Homey may be on different networks. The panel therefore owns
the long-lived Athom OAuth state and implements a limited ESP-IDF-native HTTPS
client. No permanent credential relay is required.

## Verified capabilities

Official Homey Web API documentation verifies:

- OAuth authorization-code authentication;
- client ID, client secret and redirect URL;
- access token, refresh token and expiry;
- user lookup and listing/selecting a Homey by ID;
- Homey authentication with selectable discovery strategy;
- `CLOUD` and `REMOTE_FORWARDED` strategies.

## Unverified capabilities

No official support was found for:

- PKCE;
- OAuth device authorization grant;
- public/native OAuth clients without a meaningful client secret.

These capabilities are not implemented and must not be assumed.

## Components

- credential-store interface;
- NVS-backed store compatible with encrypted NVS deployments;
- replaceable HTTPS transport interface;
- OAuth state and authorization-code importer;
- refresh-token lifecycle;
- Homey listing and explicit selection;
- CLOUD/REMOTE_FORWARDED strategy model;
- read-only inventory interface;
- empty mutation allowlist;
- credential wipe and reprovisioning state machine.

The offline patch does not contain endpoint guesses. The HTTPS adapter remains a
separate implementation boundary pending endpoint capture from official
documentation or a controlled protocol-validation phase.

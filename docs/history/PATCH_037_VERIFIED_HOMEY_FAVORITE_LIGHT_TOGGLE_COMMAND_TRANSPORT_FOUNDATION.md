# Patch037 - Verified Homey Favorite Light Toggle Command Transport Foundation

## Status and Identity

- implementation status: `COMPLETE / MERGED / REMOTE_VERIFIED`;
- source branch:
  `patch-037-verified-homey-favorite-light-toggle-command-transport-foundation`;
- source commit:
  `98906766601c7f89dabe83d9c6999ab036e79dac`;
- source tree:
  `3ecefddc7778f02bcdf36ce5abf87c7ae327a234`;
- source parent:
  `a59db1915294eec2a2402d91f087333b812c7b22`;
- pull request: `#53`;
- merge method: `squash`;
- actual implementation merge SHA:
  `bee25218fc9afc252f836663b36efe021e94e381`;
- merged tree:
  `3ecefddc7778f02bcdf36ce5abf87c7ae327a234`;
- merged parent:
  `a59db1915294eec2a2402d91f087333b812c7b22`;
- corrected implementation diff SHA256:
  `9a342e4fa7be0e8fa1976f9308c517abb17ec5dfa71f7fbf06454dd710820fb4`.

## Purpose

Add the bounded production command-transport foundation for the two verified
favorite light widgets without exposing a generic mutation surface. Patch037
keeps command eligibility, private light-toggle authorization, execution
readiness, dispatch and actual Homey mutation as distinct boundaries.

The architecture remains:

`COMMAND_ELIGIBLE != LIGHT_TOGGLE_AUTHORIZED != LIGHT_TOGGLE_EXECUTION_READY != COMMAND_DISPATCH != HOMEY_MUTATION`.

## Fixed Command Contract

The only supported targets are:

- widget4 -> `light_1/on` -> Ytterbelysning;
- widget5 -> `light_2/on` -> Dekorbelysning.

The fixed Homey write contract is:

- method: `PUT`;
- endpoint family: `/api/manager/devices/device/:deviceId/capability/onoff`;
- true body: `{"value":true}`;
- false body: `{"value":false}`;
- content type: `application/json`;
- authorization: existing Homey Bearer session token;
- raw device identity is resolved privately and is not caller supplied;
- callers cannot supply URL, method, capability, raw device ID, arbitrary JSON,
  options or transaction identifiers.

The write-only transport disables automatic redirects and HTTP authorization
retries and performs exactly one production `esp_http_client_perform()` call.
Automatic write retry is forbidden.

## Write Result Semantics

- no write attempted -> local/internal error may be `INTERNAL_ERROR`;
- write attempted with no fresh response -> `TRANSPORT_AMBIGUOUS`;
- fresh 2xx -> `WRITE_REQUEST_ACCEPTED` only;
- fresh 401 -> `UNAUTHORIZED`;
- other fresh non-2xx -> `REJECTED`.

A request-accepted result is not an authoritative device-state result. No
optimistic permanent state is published. Read-only Homey refresh remains the
state authority.

## Execution Readiness and Private Resolution

Patch036 execution readiness is sampled immediately before the only production
write primitive. Dispatch fails closed if readiness is insufficient. Raw Homey
device IDs remain private to alias-runtime resolution and are never exposed as
a generic command argument or logged as evidence.

## Exact Merged Implementation Scope

Patch037 implementation merge changed exactly:

- `components/secure_bootstrap/athom_cloud_client.c`;
- `components/secure_bootstrap/athom_oauth_runtime.c`;
- `components/secure_bootstrap/include/athom_cloud_client.h`;
- `components/secure_bootstrap/include/athom_oauth_runtime.h`;
- `components/secure_bootstrap/test_host/run_homey_light_toggle_dispatch_tests.py`;
- `components/secure_bootstrap/test_host/test_homey_light_toggle_dispatch.c`;
- `scripts/validate_patch_037.sh`.

No documentation, `sdkconfig*`, managed component or unrelated path entered PR
#53.

## Accepted Implementation and Validation Evidence

- `PATCH037_IMPLEMENTATION_ARCHITECTURE=PASS`;
- `PATCH037_FIXED_HOMEY_ENDPOINT_METHOD_BODY=PASS`;
- `PATCH037_TWO_LIGHT_ONLY_ENFORCEMENT=PASS`;
- `PATCH037_READINESS_AT_DISPATCH=PASS`;
- `PATCH037_PRIVATE_TARGET_RESOLUTION=PASS`;
- `PATCH037_NO_GENERIC_COMMAND_SURFACE=PASS`;
- `PATCH037_WRITE_RESULT_SEMANTICS=PASS`;
- `PATCH037_NO_WRITE_RETRY=PASS`;
- `PATCH037_NO_OPTIMISTIC_STATE=PASS`;
- `PATCH037_PRIVACY=PASS`;
- `PATCH037_HOST_TEST=PASS`;
- `PATCH037_STATIC_VALIDATOR=PASS`;
- `GIT_DIFF_CHECK=PASS`;
- `ESP_IDF_IDENTITY=PASS`;
- `PATCH037_ESP_IDF_BUILD=PASS`;
- `PATCH037_LOCAL_VALIDATION_GATE=PASS`;
- `PATCH037_PUBLICATION_GATE=PASS`;
- `PATCH037_PUBLICATION_REMOTE_VERIFY=PASS`;
- `PATCH037_READY_FOR_REVIEW_GATE=PASS`;
- `PATCH037_MERGE_PREFLIGHT_GATE=PASS`;
- `PATCH037_MERGE_GATE=PASS`.

The accepted ESP-IDF identity is v6.0.1 at `~/GitHub/esp-idf-v6.0.1`, target
`esp32s3`. The accepted full build returned zero, produced
`build/esp32_homey_wall_panel.bin`, and reported binary size `0x182ef0`.

Build evidence proves only that the exact validated source built in the verified
toolchain. It is not runtime, protocol, integration, hardware or Homey-mutation
evidence.

## Merge Verification

PR #53 was squash-merged at `2026-09-10T18:49:02Z`. The actual merge SHA is
`bee25218fc9afc252f836663b36efe021e94e381`. Independent post-merge comparison
from parent `a59db1915294eec2a2402d91f087333b812c7b22` to the merge showed exactly
one commit and exactly the seven approved implementation paths above.

## Runtime and Safety Boundary

Patch037 performed:

- flash: `NOT_RUN`;
- live Patch037 write-path runtime: `NOT_RUN`;
- Homey mutation: `NOT_RUN`.

A later actual write-path runtime gate requires separate explicit Homey-mutation
authorization. It remains restricted to the two light mappings above.

The following are excluded or forbidden:

- `AWNING_CONTROL=EXCLUDED`;
- `SECURITY_CONTROL=EXCLUDED`;
- `OTHER_DEVICE_CONTROL=EXCLUDED`;
- `AUTOMATIC_WRITE_RETRY=FORBIDDEN`;
- `OPTIMISTIC_STATE=FORBIDDEN`.

## Durable Lock and Self-Finalization

This detailed history file is introduced by the bounded Patch037 documentation-
only durable lock/finalization based on implementation merge
`bee25218fc9afc252f836663b36efe021e94e381`.

The durable lock scope is exactly:

- `docs/handoff/MASTER_INDEX.md`;
- `docs/handoff/CURRENT_STATE.md`;
- `docs/handoff/HANDOFF.md`;
- `docs/history/PATCH_HISTORY.md`;
- `docs/history/PATCH_037_VERIFIED_HOMEY_FAVORITE_LIGHT_TOGGLE_COMMAND_TRANSPORT_FOUNDATION.md`.

The lock is self-finalizing. It must not preclaim its own merge SHA. After the
lock is merged and the actual merged `main` ref is remotely verified, that ref
is the final stable repository merge and Patch037 is complete. No Patch037B,
Patch038 or other state-lock patch may be created solely to record this lock's
own merge identity. A future functional patch remains `UNDECIDED` until a
separate explicit scope decision.

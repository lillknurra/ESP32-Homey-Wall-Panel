# Patch051 - Private Awning Binding Provisioning Foundation

## Purpose

Provision the three operator-authorized awning read bindings into the existing
private `homey_alias_v1` dual-slot NVS store so the already-merged read-only
dashboard snapshot path can resolve widgets `awning_1`, `awning_2` and
`awning_3` on the physical panel.

Patch051 does not authorize or implement awning commands.

## Verified Base

- stable remote `main`:
  `81ba059c9c81e4229e2b841123a3359ebaa06d89`;
- stable tree:
  `b5b648ca525d1543e79a586b34c1ca8156c9bb77`;
- Patch050 / Patch050A: complete and not reopened.

## Accepted Live Read-Only Evidence

Operator mapping:

- `awning_1` -> `Markis` -> sanitized device alias `device_2a62fd38e633`;
- `awning_2` -> `Solskydd` -> sanitized device alias `device_01b770f9e1a5`;
- `awning_3` -> `Balkongmarkis` -> sanitized device alias `device_21af186ac62e`.

Unique chosen capability aliases:

- `awning_1`: `capability_a2d1bc3acd8d`;
- `awning_2`: `capability_09c823a7dbb0`;
- `awning_3`: `capability_5f25cf8276cf`.

Capability binding classification:
`PASS__UNIQUE_SEMANTIC_CAPABILITY_PER_ROLE`.

The private raw runtime binding remains outside Git. Its accepted SHA-256 is
`4a72e437f63dd0a2077cd7a4a049c9cabb3a1b4345d2a1569f41d5ae6b3077f2`.

## Design

The panel exposes one bounded local provisioning endpoint:

`POST /homey/awnings`

It accepts exactly three private device/capability pairs plus the selected-Homey
SHA-256 digest. The request is allowed only while Wi-Fi is online, the
authoritative Homey runtime is ready, and an active selected Homey identity is
available.

The supplied digest must equal SHA-256 of the exact active selected Homey ID.

The merge operation:

- replaces only dashboard binding indices `0`, `1`, `2`;
- preserves valid existing indices `3`, `4`, `5`;
- rejects duplicate binding indices;
- rejects duplicate raw device/capability pairs;
- rejects malformed private identifiers;
- writes only through the existing `panel_homey_alias_store_publish()` dual-slot
  write/readback mechanism.

Raw Homey identifiers are transient private request data. They must not enter
Git, UI text, logs, evidence or diagnostics.

Pre-build source review established three additional invariants within the same
Patch051 purpose:

- the existing HTTP server already occupied all 22 configured handler slots;
  the awning POST route is handler 23, so the server capacity is raised to 23;
- ESP-IDF v6.0.1 HTTPD uses a 4096-byte default server-task stack. The 2048-byte
  request body plus two alias records are therefore held in a zeroed heap
  allocation rather than as handler-local large objects;
- the three operator-authorized awning roles must map to three distinct raw
  Homey device IDs.

## Exact Implementation Scope

- `components/secure_bootstrap/CMakeLists.txt`;
- `components/secure_bootstrap/include/panel_homey_awning_provisioning.h`;
- `components/secure_bootstrap/panel_homey_awning_provisioning.c`;
- `components/secure_bootstrap/phone_provisioning_store.c`;
- `components/secure_bootstrap/secure_bootstrap_esp.c`;
- `components/secure_bootstrap/test_host/run_panel_homey_awning_provisioning_tests.py`;
- `components/secure_bootstrap/test_host/test_panel_homey_awning_provisioning.c`;
- `docs/history/PATCH_051_PRIVATE_AWNING_BINDING_PROVISIONING_FOUNDATION.md`;
- `scripts/provision_patch_051_awning_bindings.py`;
- `scripts/validate_patch_051.sh`.

## Non-Goals

- awning write/control path;
- Homey mutation;
- Flow or Advanced Flow reads/execution;
- browser login;
- OAuth persistence changes;
- Homey LAN/mDNS/PAT fallback;
- generic raw-ID editor;
- embedding raw Homey identifiers in firmware or Git;
- firmware flash before build/validation evidence is accepted.

## Validation

Required before flash:

- focused awning provisioning host tests;
- existing alias-store regression tests;
- existing light-provisioning regression tests;
- operator script dry-run against the exact private binding;
- static source invariants;
- privacy scan against the real private raw identifiers;
- `git diff --check`;
- complete ten-file diff review;
- ESP-IDF `v6.0.1` full build and size;
- exact firmware binary SHA-256.

A successful build is build evidence only.

## Runtime Gate

After accepted build evidence, flash is a separate explicit operator phase.

Runtime success requires:

1. provision the exact accepted private binding through `/homey/awnings`;
2. no raw identifiers in panel logs or UI;
3. next authoritative Homey inventory activates the new alias record;
4. widgets 0-2 resolve as configured and reflect authoritative availability;
5. existing light behavior remains intact;
6. no awning or other Homey mutation occurs.

Automatic serial capture immediately after flash remains prohibited because
prior evidence showed DTR/RTS startup interference.

## Rollback

Revert Patch051 source normally. Private awning bindings can be replaced by a
later valid bounded record or cleared only through an explicitly authorized
alias-store maintenance action. Do not erase unrelated account, Wi-Fi or Homey
state as rollback.

## Pre-Build Gate Corrections

Early local gates produced false failures before ESP-IDF compilation. The
final gate no longer parses porcelain status whitespace, no longer creates
`__pycache__` during syntax checking, and does not use a brittle substring
count for the selected-Homey digest field. The assignment line contains
`selected_homey_id_sha256` twice, so the four semantic source lines contain
five textual occurrences.

## ESP-IDF Compile Correction

The first complete ESP-IDF v6.0.1 build reached `phone_provisioning_store.c` and exposed one compile-time name collision.

Patch051 generalized the existing helper name from `light_active_homey_id()` to `active_homey_id()` so both light and awning provisioning could use the same selected-Homey check. The existing `light_bindings_get()` local buffer was also named `active_homey_id`, causing the local object to shadow the helper function.

The local buffer is now named `selected_homey_id`. The helper remains `active_homey_id()` and both call sites use that single generic helper. A validator guard rejects reintroduction of the helper/local-name collision.

## Evidence State

- `PATCH051_SOURCE_COMMIT=b3b8e6f5b3cbb060ae3735819edadf9aa753c3a6`;
- `PATCH051_SOURCE_TREE=5c15e14f3e79328821478f3e0a1ac262c61b2c88`;
- `PATCH051_PR=83`;
- `PATCH051_BUILD_VALIDATION=PASS__ESP_IDF_V6_0_1_BUILD_AND_SIZE`;
- `PATCH051_VALIDATOR_EXIT=0`;
- `PATCH051_V8_RESULT_ZIP_SHA256=2c6b577d72403dbb1e4e0dc5268ee797760296e5fb3c8ced90dec4895e57effc`;
- `PATCH051_V8_FULL_SCOPE_DIFF_SHA256=23a07c790fc1e76124de1e1700237e5470b7430e88e00f74f05fe39b3e2db4ed`;
- `PATCH051_V9_PUBLICATION_ZIP_SHA256=10cebf6b47a1bc289ad35a8750fdf6c978cbb4c497a1225eef9a1705e8120f68`;
- `PATCH051_COMMITTED_DIFF_SHA256=64cdb5add06a5ca06e068e96940239e1b91ad320beafa8510ce0c5eab2a9a5d1`;
- `PATCH051_SOURCE_BUILD_FIRMWARE_SHA256=39092b5039b5180148f8a26cd7a6ae7308827384d0304a1723c7ea3f725a864e`;
- `PATCH051_SOURCE_BUILD_FIRMWARE_SIZE=1624992`;
- `PATCH051_REMOTE_SOURCE_VERIFY=PASS`;
- `PATCH051_PR_MERGEABILITY_BEFORE_STATE_LOCK=clean`;
- `PATCH051_GITHUB_CHECKS=NONE_CONFIGURED_OR_REPORTED`;
- `PATCH051_PRE_MERGE_LOCK=COMPLETE`;
- `PATCH051_HOMEY_READ_DURING_LOCK=NOT_RUN`;
- `PATCH051_HOMEY_MUTATION=NOT_RUN_AND_NOT_IMPLEMENTED`;
- `PATCH051_FLASH=NOT_RUN`.

The pre-merge lock must not alter firmware implementation files. A fresh
ESP-IDF build is still required. Its binary SHA-256 is recorded as a new
lock-head build identity because ESP-IDF embeds application metadata derived
from Git/build state and optionally compile date/time; byte identity with the
earlier source-build image is therefore not a valid lock criterion.

## Post-Merge Reconciliation

- `PATCH051_FINAL_PR_HEAD=3b590d96a7f8bdd5986cc3a2ab4aabb7efa879c0`;
- `PATCH051_FINAL_PR_TREE=0e2836e1e2266e0d45789fd2e7df198dfff52287`;
- `PATCH051_MERGE_SHA=38e2cef7849085670fbf21c1bae3ca489cd84b5a`;
- `PATCH051_MERGE_TREE=0e2836e1e2266e0d45789fd2e7df198dfff52287`;
- `PATCH051_VALIDATED_TREE=0e2836e1e2266e0d45789fd2e7df198dfff52287`;
- `PATCH051_MERGED_TREE_MATCHES_VALIDATED_TREE=PASS`;
- `PATCH051_V10B_RESULT_ZIP_SHA256=5d6a950982069aa76c2ca2035af80bfd522eefc824cc1dfa803085bf4f4751bd`;
- `PATCH051_FINAL_PR_DIFF_SHA256=404e6a85463fb1b870e739b0253fa3b7b4212e23b31e790035e3520ebb87963f`;
- `PATCH051_LOCK_DIFF_SHA256=13c91931a94b486690a379dcff533fd2354f1ec1d471614ea0715590edf7551f`;
- `PATCH051_LOCK_HEAD_VALIDATION_LOG_SHA256=e90ce73c9fb66b7c017b199310de43713c7d6951f15b65c74c56a226b801aa49`;
- `PATCH051_LOCK_HEAD_BUILD_FIRMWARE_SHA256=9589d99f0bdebb77dadb7a22ef8b9b38b0d9ca1f1b9890a020613bcafe26a24a`;
- `PATCH051_LOCK_HEAD_BUILD_FIRMWARE_SIZE=1624992`;
- `PATCH051_REMOTE_MERGE_VERIFY=PASS`;
- `PATCH051_FIRMWARE_RUNTIME=NOT_RUN`;
- `PATCH051_FLASH=NOT_RUN`.

Patch051 is complete and merged. Patch051A records this merge as a
documentation-only, self-finalizing reconciliation. The next operational step
is the separate physical read-only runtime gate; Patch051 is not reopened for
new source work.

# Homey Inventory Contract

## Purpose

The Homey inventory exporter provides structurally read-only, normalized
evidence that can be used to review and maintain panel bindings. Patch 004
implemented the host-tool foundation, schema, synthetic validation, privacy
boundary, and atomic candidate publication. Patch 011 later verified live Athom
OAuth, Homey discovery, exact Homey selection, delegation, Homey login/session,
and live read-only zone and device inventory on the ESP32 panel runtime.

## Authority model

The sources of truth are separated:

```text
Homey runtime
    -> homey_inventory.json
    -> homey_inventory.md

homey_inventory.json
    + manual engineering decisions
    -> panel_binding.yaml
```

- Homey is authoritative for current runtime objects and capabilities.
- `homey_inventory.json` is the authoritative sanitized snapshot.
- `homey_inventory.md` is generated documentation and is never manually edited.
- `panel_binding.yaml` is the manually reviewed authority for panel mappings.

The exporter must never treat the three files as independent, competing
sources of truth.

## Connection modes

The future host-side inventory CLI supports these user-facing choices:

- `--connection auto`;
- `--connection local`;
- `--connection cloud`.

The CLI also reserves:

- `--mode` for a later explicitly defined inventory or validation mode;
- `--output` for the private candidate-output location;
- `--timeout` for bounded connection and collection operations.

These flags are part of the host inventory tool contract, not the ESP32 runtime
interface. Their exact accepted values and defaults are implementation work for
a later patch.

`--connection auto` uses this internal ordered strategy:

1. `LOCAL_SECURE`;
2. `LOCAL`;
3. `MDNS`;
4. `REMOTE_FORWARDED`;
5. `CLOUD`.

The strategy names describe candidate discovery or transport paths. They do not
permit credentials, IP addresses, MAC addresses, or forwarding URLs to enter
Git-tracked artifacts or logs. `--connection local` restricts selection to the
local candidates, while `--connection cloud` selects the cloud candidate only.

Fallback to the next candidate is allowed only for a classified reachability or
discovery failure. Authentication failure, authorization failure, TLS
validation failure, API incompatibility, malformed response, schema mismatch,
or configuration error must stop instead of being hidden by fallback.

Local access may use a provisioned Homey address and Personal Access Token.
Cloud access may use `AthomCloudAPI` and OAuth. Credentials and sessions are
stored outside Git, preferably in the operating system credential store.
`REMOTE_FORWARDED` configuration is private and must never be serialized into a
sanitized inventory.

All successful connection candidates return the same restricted internal
read-only client and feed the same collection and normalization pipeline.

## Read-only boundary

The exporter may read:

- zones;
- devices;
- device classes, drivers, and capability metadata;
- Flows;
- Advanced Flows;
- Moods;
- minimal platform metadata required for compatibility reporting.

The exporter must not:

- set capability values;
- start Flows or Advanced Flows;
- activate Moods;
- create, update, or delete Homey objects;
- pair or unpair devices;
- invoke a generic unrestricted Homey endpoint.

The raw Homey API object must be encapsulated behind an explicit read-only
allowlist.

## Export artifacts

### `homey_inventory.json`

A deterministic, machine-readable, sanitized inventory containing:

- schema and generator metadata;
- non-secret connection metadata;
- summary counts;
- normalized zones;
- normalized devices and capabilities;
- normalized Flows and Advanced Flows;
- normalized Moods;
- warnings and unresolved references.

Raw Homey IDs, tokens, account identifiers, addresses, and session data are
forbidden.

### `homey_inventory.md`

Generated only from sanitized JSON. It presents:

- metadata and summary;
- zone hierarchy;
- devices grouped by zone;
- candidate controls;
- Flows, Advanced Flows, and Moods;
- binding readiness;
- warnings and privacy report.

### `panel_binding.yaml`

A manually curated configuration containing stable panel, profile, widget, and
binding aliases. The exporter may generate an initial unresolved template but
must not overwrite reviewed bindings automatically.

## Normalization invariants

Local and cloud export of the same Homey state must produce semantically
identical inventory content after excluding:

- generation timestamp;
- requested and selected connection method;
- fallback metadata.

Normalization must provide:

- deterministic ordering;
- explicit null or policy values for unavailable data;
- no pass-through of unknown raw API fields;
- stable aliases independent of display-name changes;
- consistent treatment of readable and writable capabilities;
- warnings rather than silent deletion of unresolved objects.

## Pseudonymization

Stable aliases replace raw Homey IDs. Examples are:

- `zone_upstairs`;
- `device_awning_01`;
- `flow_arm_away`;
- `advanced_flow_good_night`;
- `mood_evening`.

The private alias registry maps raw IDs to stable aliases and is Git-ignored.
Aliases must not expose person names, addresses, account details, or security
state. Alias changes are explicit migrations, not incidental export effects.

## Sensitive data policy

The following must never enter Git-tracked exports or logs:

- Personal Access Tokens;
- OAuth access or refresh tokens;
- authorization codes or client secrets;
- Wi-Fi credentials;
- cookies and session identifiers;
- raw Homey, user, device, Flow, or Mood IDs;
- account email addresses;
- IP or MAC addresses;
- cloud-forwarding URLs;
- security, presence, alarm, or location state;
- unredacted request headers or error payloads.

Redaction occurs before terminal or file logging. Verbose mode may add context
but may not weaken redaction.

## Binding validation

A later validator must ensure that:

- each panel refers to an existing profile;
- every resolved alias exists in the current inventory;
- capability bindings refer to readable or writable capabilities as required;
- security-sensitive actions have confirmation policy;
- removed objects create visible broken-binding errors;
- no raw Homey IDs or secrets appear in the binding file.

Unresolved bindings are valid during design and inventory capture, but they are
not actionable.

## Publication model

A future exporter writes candidate outputs to a temporary private location,
then runs schema validation, binding validation, semantic diff, and secrets
scanning. Existing accepted outputs are replaced atomically only after all
required checks pass.

A failed or partial collection must leave the previous accepted snapshot
unchanged.

## Panel runtime principle

The inventory exporter is a host-based development and maintenance tool. It is
not part of the panel runtime path and is not required for normal panel
operation.

Normal ESP32 runtime communicates directly with Homey. The initial architecture
has no Homey companion app, proxy server, or mandatory cloud service between the
panel and Homey. Local direct communication is preferred to reduce latency and
internet dependency. Exact direct protocol, endpoint discovery, authentication,
and whether any explicitly configured cloud path is supported later remain
unimplemented and require separate validation.

## Remaining out of scope after Patch 004

Patch 004 does not provide:

- Node.js source code or npm dependencies;
- exact OAuth registration or callback implementation;
- a Personal Access Token or any session data;
- full JSON Schema definitions;
- an actual Homey export;
- verified Homey devices, capabilities, Flows, Advanced Flows, or Moods;
- any Homey command execution.


## Patch 005 offline live-access gate

The offline foundation adds an explicit `--live` gate, validates a private
configuration path outside the repository, and defines a read-operation
allowlist plus sanitized call ledger. It intentionally does not construct a
working live Homey client and does not authenticate, discover, collect, or
publish real Homey data. Those evidence classes remain NOT RUN pending separate
review and authorization.

## Current live integration status after Patch 011

Patch 011 verified the panel-side live connection and read-only inventory path: Athom OAuth and refresh, `/user/me` discovery, exact Homey selection, delegation, Homey login/session, 19 zones, 79 devices and persistent auth/session restore.

This does not mean that every host-exporter contract item is implemented. Full sanitized publication of devices, capabilities, Flows, Advanced Flows and Moods, stable alias-registry operation, semantic-diff publication and binding validation remain separate host-tool work unless explicitly verified by another patch.

Homey mutation remains outside scope. Historical `NOT RUN` statements in the Patch 004 and Patch 005 sections remain correct for those patch-local evidence boundaries.


## Patch 013 panel-side sanitized snapshot foundation

Patch 013 reuses the existing read-only `/api/manager/devices/device` response to produce a bounded panel-side snapshot. The snapshot is not the host exporter and is not a replacement for `homey_inventory.json`. It contains only generic private-provider aliases, device availability, and an optional allowlisted boolean capability value. Raw Homey device IDs and capability IDs are transient parser inputs and must not survive publication or enter logs.

Snapshot publication uses two fixed buffers, monotonically increasing generations and a short caller-supplied publication lock. Invalid, duplicate, overflowing or partially parsed candidates are never published. A previous valid snapshot may remain readable until its fixed stale threshold expires.

The production alias provider is intentionally not configured in Patch 013. No dashboard binding or Homey mutation is implemented.

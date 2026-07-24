# Design Decisions

## DD-001 - Firmware framework

**Status:** Accepted for bootstrap

The firmware uses ESP-IDF v6.0.1 and targets the Waveshare ESP32-S3-Touch-LCD-4B.

Rationale:

- direct access to ESP32 platform capabilities;
- explicit component and build management;
- suitable foundation for display, networking, provisioning, and device integration.

The board-level hardware baseline is defined in `docs/hardware/HARDWARE_BASELINE.md`. GPIO mappings and peripheral initialization remain intentionally unverified until extracted from Waveshare primary artifacts and validated on hardware.

## DD-002 - ESP32 and Homey responsibility boundary

**Status:** Accepted for Patch 003

The ESP32 owns UI, dashboards, layout, animations, favorites, navigation, gestures, local
interaction state, confirmation behavior, configuration validation, and safe
fallback presentation. Homey owns devices, capabilities, authoritative runtime
state, automation, Flows, Advanced Flows, and Moods. Normal runtime is direct
between ESP32 and Homey; no companion app, proxy server, or mandatory cloud
service is part of the initial command path. Exact protocol and authentication
remain unresolved.

## DD-003 - Stable panel identity

**Status:** Accepted for Patch 003

Configuration separates `panel_id`, `display_name`, `profile_id`, and
`hardware_id`. The panel-map key is the canonical `panel_id`; it is read from
provisioned local configuration and is never guessed or derived from a MAC
address. The stable panel IDs are `wall-panel-stairs` and
`wall-panel-upstairs`.

## DD-004 - Layered panel configuration and profiles

**Status:** Accepted for Patch 003

Resolved configuration applies base configuration, then a shared profile, then
panel-specific overrides. Both initial panels use `shared-favorites-v1`.

## DD-005 - Initial 2 x 3 favorites dashboard

**Status:** Accepted for Patch 003

The initial page contains three awning widgets, one security widget, and two
lighting widgets in a two-column by three-row grid with a page indicator.

## DD-006 - Awning interaction model

**Status:** Accepted for Patch 003

The UI models open/up, stop, and close/down and may display position and motion
when supported. Exact Homey capabilities remain unresolved until inventory
evidence exists.

## DD-007 - Security-sensitive Verisure interaction

**Status:** Accepted for Patch 003

Security actions require an explicit confirmation gesture, defaulting to long
press. The UI must wait for authoritative Homey confirmation and must not infer
success from command transmission.

## DD-008 - Lighting interaction model

**Status:** Accepted for Patch 003

A short tap is intended for a validated toggle binding and a long press opens
details. Unknown or unavailable state must not be presented as a guessed value.

## DD-009 - Homey binding types

**Status:** Accepted for Patch 003

Allowed binding types are `unresolved`, `device_capability`, `flow`,
`advanced_flow`, and `mood`. The initial binding file uses only `unresolved`.

## DD-010 - Homey inventory contract

**Status:** Accepted for Patch 003

A future host-side Node.js exporter uses the official `homey-api`, is structurally
read-only, supports `auto`, `local`, and `cloud` connection modes, and produces
a sanitized JSON snapshot, generated Markdown, and a manually reviewed YAML
binding configuration. Local and cloud collection share one normalization
pipeline. `auto` evaluates `LOCAL_SECURE`, `LOCAL`, `MDNS`,
`REMOTE_FORWARDED`, and `CLOUD` in that order, with fallback only for classified
reachability or discovery failures. The exporter is not part of the panel
runtime path.

## DD-011 - Safe fallback and degraded mode

**Status:** Accepted for Patch 003

Invalid identity, profile, schema, or binding configuration disables Homey
control and exposes sanitized diagnostics. The panel must never create guessed
IDs, capabilities, or commands as fallback behavior.


## DD-012 - Patch 004 inventory implementation boundary

**Status:** Accepted for Patch 004

The Homey inventory exporter is an isolated TypeScript host tool using exactly
pinned `homey-api`. Its production adapter is configuration-gated, its internal
client is structurally read-only, and automated validation uses synthetic data.
Live authentication, discovery, integration, and protocol behavior require later
separate evidence.


## DD-013 - Explicit live Homey gate

**Status:** Accepted for Patch 005 offline foundation

Non-synthetic Homey access requires explicit `--live` and a validated private
configuration outside the repository. Normal tests and validators must not
authenticate or issue network requests.

## DD-014 - Structurally read-only live boundary

**Status:** Accepted for Patch 005 offline foundation

The live path exposes only explicit read methods for platform metadata, zones,
devices, Flows, Advanced Flows, and Moods. A sanitized call ledger records only
allowlisted operation names, never arguments, identifiers, values, or responses.

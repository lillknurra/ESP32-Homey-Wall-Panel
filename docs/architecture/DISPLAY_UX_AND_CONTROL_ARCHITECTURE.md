# Display UX and Control Architecture

## Status and scope

This document defines the durable UX and control architecture for the two
ESP32 Homey wall panels. It is a design baseline only. Patch 003 does not
implement display, touch, networking, Homey communication, or device control.

## Responsibility boundary

The ESP32 panel owns:

- dashboards and page navigation;
- layout, visual state, animations, and page indicators;
- favorites and their order;
- gestures, confirmations, and local interaction state;
- local configuration loading and validation;
- degraded-mode presentation and local diagnostics.

Homey owns:

- devices and zones;
- capabilities and current device state;
- automations, Flows, Advanced Flows, and Moods;
- execution of real home-control actions;
- authoritative runtime state for bound controls.

The panel must not duplicate Homey automation logic. It presents curated
controls and communicates directly with Homey during normal runtime. The
initial architecture has no Homey companion app, proxy server, or mandatory
cloud service in the command path. Exact direct protocol and authentication
mechanisms remain unresolved until a later, separately validated integration
patch.

## Panel identity

Panel identity is separated into four fields:

- `panel_id`: stable logical identity used by configuration and bindings;
- `display_name`: user-facing name that may change without rebinding;
- `profile_id`: selected shared configuration profile;
- `hardware_id`: physical-unit identity used for diagnostics and provisioning.

The first two logical panels are:

| panel_id | display_name |
|---|---|
| `wall-panel-stairs` | Väggpanel Trappa |
| `wall-panel-upstairs` | Väggpanel Ovanvåning |

Replacing hardware must not require changing `panel_id`. A replacement may get
a new `hardware_id` while retaining the logical panel identity and profile.

## Configuration layering

Configuration is resolved in this order:

1. base configuration;
2. shared profile selected by `profile_id`;
3. panel-specific overrides.

The startup sequence is:

1. read `panel_id` from provisioned local configuration;
2. select the panel entry;
3. resolve `profile_id`;
4. load base configuration;
5. apply profile values;
6. apply panel-specific overrides;
7. validate the resolved configuration;
8. start the normal UI or enter safe fallback mode.

The canonical `panel_id` is the key below `panels` in
`config/panel_binding.yaml`. It is provisioned locally and must never be guessed
or derived from a MAC address, display name, Homey object, or other incidental
hardware or network property.

The `defaults` mapping in `config/panel_binding.yaml` is the machine-readable
base configuration layer. The initial panels share one favorites profile. The
architecture still permits later differences without changing panel
identities.

## Initial dashboard

The initial page is a favorites dashboard with a two-column by three-row grid:

```text
+----------------------+----------------------+
| Markis 1             | Markis 2             |
+----------------------+----------------------+
| Markis 3             | Verisure             |
+----------------------+----------------------+
| Belysning 1          | Belysning 2          |
+----------------------+----------------------+
|                   page indicator            |
+---------------------------------------------+
```

These are six logical widgets. They are not assumed to represent exactly six
Homey devices. A widget may later bind to a device capability, Flow, Advanced
Flow, or Mood.

Future page families may include:

- Favorites;
- Lighting;
- Blinds;
- Security.

Patch 003 does not lock final pixel geometry, typography, iconography, colors,
or animation timing.

## Common control states

Every actionable widget must be able to represent:

- `unknown`: no authoritative state has been received;
- `available`: control can be used;
- `pending`: the panel has dispatched or queued the configured command and awaits authoritative Homey confirmation;
- `unavailable`: Homey or the bound object reports that control is unavailable;
- `blocked`: configuration or safety policy prevents the action;
- `error`: the most recent command or synchronization attempt failed.

A widget must not display success only because a touch was detected. Success is
shown only after later integration logic receives authoritative confirmation.

## Awning interaction model

An awning widget must support the semantic actions:

- open or move up;
- stop;
- close or move down.

It may additionally display position and movement direction when the real
Homey object exposes suitable readable capabilities. Patch 003 does not guess
capability names or command values.

The UI contract must remain stable even if one awning is bound directly to
capabilities and another is bound to Flows.

## Verisure and security-sensitive interaction

Security-sensitive actions must not execute from an ordinary single tap.

The global `defaults.interaction.sensitive_action` policy applies to widgets
whose `widget_type` is listed in `applies_to_widget_types`. The initial list
contains `security`, so the Verisure widget inherits the long-press confirmation
requirement. A later reviewed widget-specific override may strengthen, but must
not silently weaken, that policy.

The initial interaction contract is:

1. a tap opens a security detail or confirmation surface;
2. an arm or disarm action requires an explicit confirmation gesture;
3. the default confirmation method is long press;
4. the panel shows `pending` while waiting for Homey confirmation;
5. timeout, rejection, or unavailable state is shown as failure;
6. the panel never infers successful alarm state from command transmission.

Whether Verisure is bound through a device capability, Flow, or Advanced Flow
remains unresolved until a read-only Homey inventory and safety review exist.

## Lighting interaction model

The initial lighting contract is:

- short tap: toggle when a safe writable toggle capability is configured;
- long press: open details for dimming or additional controls;
- unavailable object: disable command input while retaining visible status;
- unknown state: do not present a guessed on/off value.

Exact capabilities remain unresolved until Homey inventory evidence exists.

## Binding model

The only allowed binding types are:

- `unresolved`;
- `device_capability`;
- `flow`;
- `advanced_flow`;
- `mood`.

The initial binding file uses only `unresolved`. A binding becomes actionable
only after its referenced alias, capabilities, and safety policy have been
reviewed and validated.

Bindings use stable aliases, never raw Homey IDs. Manually reviewed bindings
are configuration authority for the panels and must not be rewritten merely
because a later inventory snapshot changes.

## Fallback and degraded mode

If panel identity, profile resolution, binding configuration, or schema
validation fails, the panel enters a safe fallback mode.

Fallback mode must:

- disable all Homey control actions;
- show that configuration is missing or invalid;
- expose non-secret diagnostics;
- never invent object IDs, capabilities, or default commands;
- keep local UI operation deterministic.

Temporary Homey disconnection is a degraded runtime state rather than a reason
to replace the validated configuration. Controls are disabled or marked
unavailable until synchronization returns.

## Diagnostics contract

The diagnostic surface may expose:

- `panel_id`;
- `display_name`;
- `profile_id`;
- `hardware_id`;
- firmware version;
- configuration version;
- active dashboard;
- Homey connection state;
- synchronization state;
- last sanitized error code.

It must not expose Wi-Fi credentials, tokens, raw Homey IDs, account details,
IP addresses, OAuth data, or unredacted network errors.

## Configuration versioning

Panel configuration must contain:

- `schema_version` for structural compatibility;
- `config_version` for the configuration revision;
- `profile_id` for profile selection.

Unknown major schema versions must be rejected into safe fallback rather than
partially interpreted.

## Out of scope for Patch 003

Patch 003 does not implement or validate:

- LVGL or any display or touch driver;
- Wi-Fi or other networking;
- Homey authentication, API access, or command transport;
- actual Homey inventory data;
- real device, Flow, Advanced Flow, Mood, or capability identifiers;
- runtime, hardware, protocol, firmware, or Homey integration behavior.

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

## Patch 012 multi-page UI foundation

Patch 012 implements the first visible multi-page dashboard and settings shell in bounded stages. Package 1 establishes only the platform-independent model:

- three pages with Favoriter first;
- six fixed read-only favorite widgets;
- read-only availability labels;
- dashboard, settings and confirmation views;
- active, dimmed and off power states;
- consumed wake touch after dimming or display-off;
- bounded display settings and Europe/Stockholm time presentation;
- explicit confirmation state for Homey wipe and Athom account change.

The Package 1 model contains no raw Homey IDs, capability IDs, command payloads, mutation functions or protocol calls. Actual LVGL rendering, persistence, SNTP and provisioning integration remain later steps inside Patch 012 and require their own evidence.

## Patch 012 Package 3A display power behavior

The production OFF state is a visual-off implementation. The active dashboard
is covered by a reusable, fully opaque black full-screen LVGL overlay before
the BSP brightness is set to 0 percent. The ST7701 controller is not sent
Display Off, sleep, reset or reinitialization commands.

Normal brightness defaults to 80 percent. Dimmed brightness is restricted to
10, 30 or 50 percent. Stored legacy values are normalized to the nearest
allowed value with an upward tie: 20 becomes 30 and 40 becomes 50. Wake on
touch is permanent. The first touch from DIMMED or OFF wakes the panel and is
consumed; the following touch is handled normally.

This OFF state is not an electrical shutdown. On the current hardware GPIO 4
acts through the AP3032 feedback/dimming network, while AP3032 CTRL remains
pulled high. The AP3032 therefore remains electrically active at brightness 0.
A true hard-off requires a separately reviewed hardware change that exposes or
controls AP3032 CTRL with a safe boot and fail-safe design.


## Patch 013 read-only snapshot boundary

Patch 013 does not change the dashboard model, LVGL rendering, widget interaction or display-power behavior. It establishes a separate sanitized Homey read-snapshot boundary for a later explicitly scoped dashboard-binding patch. The UI must never consume raw Homey IDs, JSON objects, response buffers or credential-bearing runtime structures.

## Patch 014 read-only snapshot binding

The six dashboard cards consume only a platform-independent model populated
from the sanitized Homey snapshot. Available widgets with an explicit
allowlisted boolean render `Aktiv` or `Inaktiv`; otherwise the existing
read-only availability text is used.

The runtime polls every 1000 ms. Snapshot copying and adapter execution occur
outside the LVGL display lock. Model application and `panel_ui_refresh()` occur
inside the existing display-lock context, and refresh is requested only after
an actual model change.

This patch does not make cards actionable. It adds no touch-to-command path,
pending state, success inference, device control, Flow execution, Advanced
Flow execution, Mood execution, or security mutation.

## Patch021 UI responsiveness diagnostics

Patch021 is diagnostics-only. It extends the existing bounded swipe diagnostics
with sanitized responsiveness markers for dashboard swipes and long settings
scrolls, and adds periodic display refresh/flush statistics that can be
correlated with gesture windows.

The diagnostic surface may report:

- dashboard swipe begin/end timing and resolved page;
- settings scroll begin/end timing and bounded object-count context;
- periodic LVGL refresh and flush counts, average durations and maximum
  durations;
- display rotation polling interval.

Patch021 must not change UI layout, page count, page navigation, card behavior,
settings layout, LVGL scroll tuning, gesture thresholds, Homey mutation,
clickable controls, command dispatch or Package 3B behavior.

## Patch022 bounded panel UI scroll responsiveness

Patch022 is a separately scoped bounded optimization after passive Patch021 UI
evidence correlated dashboard and settings scroll intervals with high LVGL
refresh and flush measurements. The first candidate changes only the LVGL input
scroll decay configuration: `scroll_limit` remains `4` and `scroll_throw` is
tested at `20`.

The change is owned by `components/secure_bootstrap/secure_bootstrap_esp.c`.
`panel_ui.c`, page count, layout, navigation, snap behavior, gesture thresholds,
refresh ownership and display flush callbacks remain unchanged in the first
candidate. Runtime acceptance must compare dashboard and settings scroll timing
against the accepted Patch021 UI evidence and must verify page/settings
correctness, privacy and runtime safety.

Patch022 does not change Homey transport, OAuth, token refresh, retry, timeout,
Favorites, mutation, command dispatch, Package 3B, allocator, PSRAM, MbedTLS or
`sdkconfig*` policy.

## Patch023 UI render-path requirements and scope lock

Patch023 is documentation-only. It does not change firmware, scroll parameters,
UI layout, page navigation, display ownership or Homey behavior.

The analysis must distinguish these paths before any future production change:

- touch and LVGL scroll begin/end handling for dashboard and settings;
- page selection and settings-layer object traversal;
- `panel_ui_refresh()` calls caused by model or Favorites changes;
- explicit invalidation and immediate refresh calls used by power transitions;
- display refresh and flush callbacks, including display-lock duration;
- concurrent Homey snapshot publication or refresh activity.

The existing Patch021/Patch022 evidence shows scroll/performance overlap and
high refresh/flush maxima, but does not establish which path is causal. A future
implementation may consider only a separately approved bounded change in
`panel_ui.c` or `secure_bootstrap_esp.c` after that mechanism is demonstrated.
No change to either component is authorized by Patch023.

Package 3B remains separately scoped and `NOT_STARTED`. Homey mutation,
command dispatch, OAuth, transport, retry, timeout, Favorites and endpoint
policy remain outside this analysis.

## Patch024 render-path attribution diagnostics

Patch024 is diagnostics-only. It records one sanitized attribution line for
each full `panel_ui_refresh()` call and bounded five-second summaries for
application display-lock timing and Homey refresh causes. It does not log every
LVGL frame.

The diagnostics distinguish:

- refresh phase durations for Favorites application, widget/view rendering and
  page reassertion;
- whether the pager or settings layer was scrolling at refresh start/end;
- display-lock wait and hold time for the Homey poll and rotation task;
- Homey model changes, Favorites changes and refresh requests;
- existing LVGL refresh/flush windows through correlation with
  `PATCH021_DISPLAY_PERF`.

Patch024 does not change scroll parameters, page selection behavior, layout,
refresh ownership, display flush callbacks, Homey transport, OAuth, retry,
timeout, Favorites semantics or mutation boundaries. The managed LVGL port and
Waveshare BSP remain outside scope. Any later production optimization requires
separate evidence and explicit scope approval.

#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

pass() {
  printf 'PASS: %s\n' "$*"
}

patch_files=(
  config/panel_binding.yaml
  docs/architecture/DESIGN_DECISIONS.md
  docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md
  docs/architecture/HOMEY_INVENTORY_CONTRACT.md
  docs/handoff/CURRENT_STATE.md
  docs/handoff/HANDOFF.md
  docs/handoff/MASTER_INDEX.md
  docs/history/PATCH_003_DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md
  docs/history/PATCH_HISTORY.md
  scripts/validate_patch_003.sh
)

for path in "${patch_files[@]}"; do
  [[ -f "$path" ]] || fail "missing expected Patch 003 file: $path"
done
pass "all ten expected Patch 003 files exist"

python3 - <<'PY'
from pathlib import Path
import re
import sys

try:
    import yaml
except ModuleNotFoundError:
    raise SystemExit(
        'FAIL: PyYAML is required. Install it with '
        '`python3 -m pip install --user PyYAML`; for a PEP 668/Homebrew '
        'environment, add `--break-system-packages` only when required.'
    )

path = Path('config/panel_binding.yaml')
try:
    data = yaml.safe_load(path.read_text(encoding='utf-8'))
except Exception as exc:
    raise SystemExit(f'FAIL: invalid YAML: {exc}')

if not isinstance(data, dict):
    raise SystemExit('FAIL: panel binding root must be a mapping')
if data.get('schema_version') != 1:
    raise SystemExit('FAIL: schema_version must be 1')
if data.get('config_version') != 1:
    raise SystemExit('FAIL: config_version must be 1')
if 'binding_version' in data:
    raise SystemExit('FAIL: binding_version is not part of the Patch 003 version model')

homey = data.get('homey')
if homey != {'alias': 'primary-homey'}:
    raise SystemExit('FAIL: homey must contain only logical alias primary-homey')

defaults = data.get('defaults')
try:
    sensitive = defaults['interaction']['sensitive_action']
except (TypeError, KeyError):
    raise SystemExit('FAIL: missing defaults.interaction.sensitive_action policy')
if sensitive != {
    'applies_to_widget_types': ['security'],
    'confirmation_required': True,
    'confirmation_method': 'long_press',
}:
    raise SystemExit('FAIL: security widgets must inherit the exact long-press confirmation policy')

profiles = data.get('profiles')
if not isinstance(profiles, dict) or set(profiles) != {'shared-favorites-v1'}:
    raise SystemExit('FAIL: expected only shared-favorites-v1 profile')
slots = profiles['shared-favorites-v1'].get('slots')
if not isinstance(slots, list) or len(slots) != 6:
    raise SystemExit('FAIL: shared profile must contain exactly six slots')

slot_numbers = [item.get('slot') for item in slots]
if slot_numbers != [1, 2, 3, 4, 5, 6]:
    raise SystemExit('FAIL: slots must be unique and ordered 1 through 6')
widget_ids = [item.get('widget_id') for item in slots]
if len(widget_ids) != len(set(widget_ids)):
    raise SystemExit('FAIL: widget_id values must be unique')
widget_types = [item.get('widget_type') for item in slots]
if widget_types.count('awning') != 3:
    raise SystemExit('FAIL: expected exactly three awning widgets')
if widget_types.count('security') != 1:
    raise SystemExit('FAIL: expected exactly one security widget')
if widget_types.count('light') != 2:
    raise SystemExit('FAIL: expected exactly two light widgets')
for item in slots:
    binding = item.get('binding')
    if binding != {'type': 'unresolved'}:
        raise SystemExit(f"FAIL: slot {item.get('slot')} binding must contain only type: unresolved")

panels = data.get('panels')
expected_panels = {'wall-panel-stairs', 'wall-panel-upstairs'}
if not isinstance(panels, dict) or set(panels) != expected_panels:
    raise SystemExit('FAIL: exact canonical panel-map keys are required')
expected_names = {
    'wall-panel-stairs': 'Väggpanel Trappa',
    'wall-panel-upstairs': 'Väggpanel Ovanvåning',
}
for panel_id, panel in panels.items():
    if panel.get('display_name') != expected_names[panel_id]:
        raise SystemExit(f'FAIL: incorrect display_name for {panel_id}')
    if panel.get('profile_id') != 'shared-favorites-v1':
        raise SystemExit(f'FAIL: invalid profile reference for {panel_id}')
    if panel.get('hardware_id') != 'unresolved':
        raise SystemExit(f'FAIL: hardware_id must remain unresolved for {panel_id}')
    if panel.get('overrides') != []:
        raise SystemExit(f'FAIL: initial overrides must be empty for {panel_id}')

text = path.read_text(encoding='utf-8')
for forbidden_key in ('token:', 'secret:', 'password:', 'device_id:', 'flow_id:', 'capability:'):
    if forbidden_key in text.lower():
        raise SystemExit(f'FAIL: forbidden binding key found: {forbidden_key}')

uuid_pattern = re.compile(r'\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}\b')
if uuid_pattern.search(text):
    raise SystemExit('FAIL: UUID-like raw identifier found in panel binding')

print('PASS: panel_binding.yaml version, profile, panel, layout, binding, and safety invariants')
PY

architecture_files=(
  docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md
  docs/architecture/HOMEY_INVENTORY_CONTRACT.md
  docs/architecture/DESIGN_DECISIONS.md
)
for binding_type in unresolved device_capability flow advanced_flow mood; do
  grep -Rqs -- "$binding_type" "${architecture_files[@]}" \
    || fail "binding type is not documented: $binding_type"
done
pass "all allowed binding types are documented"

for required_text in \
  'communicates directly with Homey' \
  'no Homey companion app' \
  'LOCAL_SECURE' \
  'REMOTE_FORWARDED' \
  'canonical `panel_id`' \
  'never be guessed' \
  'animations'; do
  grep -Rqs -- "$required_text" "${architecture_files[@]}" \
    || fail "missing required architecture statement: $required_text"
done
pass "direct runtime, connection order, canonical identity, and ESP32 animation ownership are explicit"

for decision in DD-002 DD-003 DD-004 DD-005 DD-006 DD-007 DD-008 DD-009 DD-010 DD-011; do
  grep -q -- "$decision" docs/architecture/DESIGN_DECISIONS.md \
    || fail "missing design decision: $decision"
done
pass "DD-002 through DD-011 are present"

grep -q 'STATIC VALIDATION PASS / COMMITTED / PUBLISHED ON ACTIVE BRANCH / NOT MERGED TO MAIN' \
  docs/history/PATCH_HISTORY.md \
  || fail "Patch 002 status was not corrected"
grep -q 'e42283df9e256cca32cc1790ff8f14824edc7365' docs/history/PATCH_HISTORY.md \
  || fail "Patch 002 commit is missing from history"
grep -q 'Remote verification:' docs/history/PATCH_HISTORY.md \
  || fail "Patch 002 remote verification is missing"
pass "Patch 002 status correction is present"

for required in \
  'ESP-IDF build: NOT IN SCOPE' \
  'Runtime: NOT RUN' \
  'Hardware: NOT RUN' \
  'Homey integration: NOT RUN' \
  'Protocol: NOT RUN' \
  'Firmware: NOT MODIFIED'; do
  grep -q -- "$required" docs/history/PATCH_HISTORY.md \
    || fail "missing Patch 003 evidence boundary: $required"
done
pass "Patch 003 evidence boundaries are explicit"

{
  git diff --name-only --diff-filter=ACMRTUXB 2>/dev/null || true
  git diff --cached --name-only --diff-filter=ACMRTUXB 2>/dev/null || true
  git ls-files --others --exclude-standard 2>/dev/null || true
} | awk 'NF && !seen[$0]++' > "${TMPDIR:-/tmp}/patch003_changed_paths.$$"
changed_paths_file="${TMPDIR:-/tmp}/patch003_changed_paths.$$"
trap 'rm -f "$changed_paths_file"' EXIT

if grep -Eq '^(main/|components/|CMakeLists\.txt$|sdkconfig\.defaults$|partitions\.csv$|package\.json$|package-lock\.json$)' "$changed_paths_file"; then
  printf 'Forbidden changed paths:\n' >&2
  grep -E '^(main/|components/|CMakeLists\.txt$|sdkconfig\.defaults$|partitions\.csv$|package\.json$|package-lock\.json$)' "$changed_paths_file" >&2
  fail "out-of-scope modified, staged, or untracked implementation/package path detected"
fi
pass "no forbidden modified, staged, or untracked implementation/package path detected"

python3 - "${patch_files[@]}" <<'PY'
from pathlib import Path
import re
import sys

patterns = {
    'credential assignment': re.compile(r'(?i)\b(?:access[_-]?token|refresh[_-]?token|client[_-]?secret|personal[_-]?access[_-]?token|api[_-]?key|password)\s*[:=]\s*["\']?(?!\[?redacted\]?|missing\b|not configured\b)[A-Za-z0-9._~+/=-]{8,}'),
    'bearer value': re.compile(r'(?i)\bBearer\s+[A-Za-z0-9._~+/=-]{8,}'),
    'private IPv4 address': re.compile(r'\b(?:10\.(?:\d{1,3}\.){2}\d{1,3}|192\.168\.(?:\d{1,3}\.)\d{1,3}|172\.(?:1[6-9]|2\d|3[01])\.(?:\d{1,3}\.)\d{1,3})\b'),
    'MAC address': re.compile(r'(?i)\b(?:[0-9a-f]{2}:){5}[0-9a-f]{2}\b'),
    'UUID-like raw identifier': re.compile(r'\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}\b'),
}
for filename in sys.argv[1:]:
    text = Path(filename).read_text(encoding='utf-8')
    for label, pattern in patterns.items():
        if pattern.search(text):
            raise SystemExit(f'FAIL: {label} found in {filename}')
print('PASS: all ten Patch 003 files are free of detected secret values, private IPs, MAC addresses, and UUID-like raw IDs')
PY

pass "Patch 003 static validation complete"

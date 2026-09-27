#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import ipaddress
import json
from pathlib import Path
import stat
import sys
from urllib import error, parse, request

DEFAULT_BINDING = Path(
    "/Users/petter/Library/Application Support/ESP32-Homey-Wall-Panel/"
    "patch043-remote-awning/awning_runtime_binding.private.json"
)

ROLES = {
    "awning_1": 0,
    "awning_2": 1,
    "awning_3": 2,
}

def load_private_binding(path: Path) -> dict:
    st = path.lstat()
    if stat.S_ISLNK(st.st_mode) or not stat.S_ISREG(st.st_mode):
        raise RuntimeError("private binding must be a regular non-symlink file")
    if stat.S_IMODE(st.st_mode) & 0o077:
        raise RuntimeError("private binding permissions must not allow group/other access")

    document = json.loads(path.read_text())

    if set(document) != {
        "schema_version",
        "purpose",
        "generation",
        "selected_homey_id_sha256",
        "entries",
    }:
        raise RuntimeError("private binding schema fields mismatch")
    if document["schema_version"] != 1:
        raise RuntimeError("private binding schema_version mismatch")
    if document["purpose"] != "panel_homey_awning_runtime_binding":
        raise RuntimeError("private binding purpose mismatch")
    if not isinstance(document["generation"], int) or document["generation"] <= 0:
        raise RuntimeError("private binding generation invalid")

    digest = document["selected_homey_id_sha256"]
    if not isinstance(digest, str) or len(digest) != 64:
        raise RuntimeError("private binding selected-Homey digest invalid")
    int(digest, 16)

    entries = document["entries"]
    if not isinstance(entries, list) or len(entries) != 3:
        raise RuntimeError("private binding must contain exactly three entries")

    by_role = {}
    for entry in entries:
        if not isinstance(entry, dict) or set(entry) != {
            "role",
            "dashboard_binding_index",
            "raw_device_id",
            "raw_capability_id",
        }:
            raise RuntimeError("private binding entry schema mismatch")

        role = entry["role"]
        if role not in ROLES or role in by_role:
            raise RuntimeError("private binding role invalid or duplicated")
        if entry["dashboard_binding_index"] != ROLES[role]:
            raise RuntimeError("private binding dashboard index mismatch")

        limits = {
            "raw_device_id": 128,
            "raw_capability_id": 64,
        }
        for key in ("raw_device_id", "raw_capability_id"):
            value = entry[key]
            if not isinstance(value, str) or not value:
                raise RuntimeError("private binding raw identifier invalid")
            if not value.isascii() or any(ord(ch) < 0x21 or ord(ch) > 0x7e for ch in value):
                raise RuntimeError("private binding raw identifier must be printable ASCII")
            if len(value) >= limits[key]:
                raise RuntimeError(f"private binding {key} exceeds panel capacity")

        by_role[role] = entry

    if set(by_role) != set(ROLES):
        raise RuntimeError("private binding roles incomplete")

    device_ids = [by_role[role]["raw_device_id"] for role in ("awning_1", "awning_2", "awning_3")]
    if len(set(device_ids)) != 3:
        raise RuntimeError("private binding awning device IDs must be distinct")

    document["_by_role"] = by_role
    return document

def panel_endpoint(panel_url: str) -> str:
    parsed = parse.urlparse(panel_url)
    if parsed.scheme != "http" or not parsed.hostname:
        raise RuntimeError("--panel-url must be an explicit http://IP address")
    if parsed.username or parsed.password or parsed.query or parsed.fragment:
        raise RuntimeError("--panel-url must not include credentials, query or fragment")
    try:
        ipaddress.ip_address(parsed.hostname)
    except ValueError as exc:
        raise RuntimeError("--panel-url must use an explicit IP address; mDNS/DNS discovery is not allowed") from exc
    if parsed.path not in ("", "/"):
        raise RuntimeError("--panel-url must not include a path")
    return panel_url.rstrip("/") + "/homey/awnings"

def form_fields(document: dict) -> dict[str, str]:
    by_role = document["_by_role"]
    fields = {
        "schema_version": "1",
        "purpose": "panel_homey_awning_runtime_binding",
        "generation": str(document["generation"]),
        "selected_homey_id_sha256": document["selected_homey_id_sha256"],
    }
    for role in ("awning_1", "awning_2", "awning_3"):
        fields[f"{role}_device_id"] = by_role[role]["raw_device_id"]
        fields[f"{role}_capability_id"] = by_role[role]["raw_capability_id"]
    return fields

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--private-binding", type=Path, default=DEFAULT_BINDING)
    parser.add_argument("--panel-url")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    document = load_private_binding(args.private_binding)
    binding_sha = hashlib.sha256(args.private_binding.read_bytes()).hexdigest()

    print(f"PATCH051_PRIVATE_BINDING_SHA256={binding_sha}")
    print("PATCH051_PRIVATE_BINDING_SCHEMA=PASS")
    print("PATCH051_RAW_IDENTIFIER_LOGGING=NONE")

    if args.dry_run:
        print("PATCH051_PANEL_PROVISION=NOT_RUN_DRY_RUN")
        return 0

    if not args.panel_url:
        raise RuntimeError("--panel-url is required unless --dry-run is used")

    endpoint = panel_endpoint(args.panel_url)
    body = parse.urlencode(form_fields(document)).encode("ascii")
    if len(body) >= 2048:
        raise RuntimeError("encoded private binding request exceeds panel body capacity")

    req = request.Request(
        endpoint,
        data=body,
        headers={"Content-Type": "application/x-www-form-urlencoded"},
        method="POST",
    )

    try:
        with request.urlopen(req, timeout=12) as response:
            status = response.status
            response.read(1024)
    except error.HTTPError as exc:
        print(f"PATCH051_PANEL_PROVISION_HTTP_STATUS={exc.code}")
        print("PATCH051_PANEL_PROVISION=FAIL")
        return 2
    except error.URLError:
        print("PATCH051_PANEL_PROVISION_HTTP_STATUS=UNAVAILABLE")
        print("PATCH051_PANEL_PROVISION=FAIL")
        return 3

    print(f"PATCH051_PANEL_PROVISION_HTTP_STATUS={status}")
    if status < 200 or status >= 300:
        print("PATCH051_PANEL_PROVISION=FAIL")
        return 4

    print("PATCH051_PANEL_PROVISION=PASS")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"PATCH051_PANEL_PROVISION_PRECHECK=FAIL:{type(exc).__name__}:{exc}", file=sys.stderr)
        raise SystemExit(1)

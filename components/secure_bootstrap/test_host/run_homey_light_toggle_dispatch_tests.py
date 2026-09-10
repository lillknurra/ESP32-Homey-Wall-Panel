#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import re
import subprocess
import sys
import tempfile

BASE = "a59db1915294eec2a2402d91f087333b812c7b22"
BRANCH = "patch-037-verified-homey-favorite-light-toggle-command-transport-foundation"

EXPECTED = [
    "components/secure_bootstrap/athom_cloud_client.c",
    "components/secure_bootstrap/athom_oauth_runtime.c",
    "components/secure_bootstrap/include/athom_cloud_client.h",
    "components/secure_bootstrap/include/athom_oauth_runtime.h",
    "components/secure_bootstrap/test_host/run_homey_light_toggle_dispatch_tests.py",
    "components/secure_bootstrap/test_host/test_homey_light_toggle_dispatch.c",
    "scripts/validate_patch_037.sh",
]


def root() -> Path:
    return Path(__file__).resolve().parents[3]


def run(cmd: list[str], cwd: Path, check: bool = True) -> subprocess.CompletedProcess[str]:
    p = subprocess.run(cmd, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if check and p.returncode != 0:
        print(p.stdout, end="")
        raise SystemExit(p.returncode)
    return p


def extract_function(src: str, signature: str) -> str:
    start = src.find(signature)
    if start < 0:
        raise AssertionError(f"missing function: {signature}")
    brace = src.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing function body: {signature}")
    depth = 0
    for i in range(brace, len(src)):
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
            if depth == 0:
                return src[start:i + 1]
    raise AssertionError(f"unterminated function: {signature}")


def changed_paths(repo: Path) -> list[str]:
    tracked = run(["git", "diff", "--name-only"], repo).stdout.splitlines()
    untracked = run(["git", "ls-files", "--others", "--exclude-standard"], repo).stdout.splitlines()
    return sorted(set(x for x in tracked + untracked if x))


def validate_source(repo: Path) -> None:
    assert run(["git", "branch", "--show-current"], repo).stdout.strip() == BRANCH
    assert run(["git", "rev-parse", "HEAD"], repo).stdout.strip() == BASE
    assert run(["git", "diff", "--cached", "--name-only"], repo).stdout.strip() == ""
    assert changed_paths(repo) == sorted(EXPECTED)

    cloud_h = (repo / "components/secure_bootstrap/include/athom_cloud_client.h").read_text()
    cloud_c = (repo / "components/secure_bootstrap/athom_cloud_client.c").read_text()
    oauth_h = (repo / "components/secure_bootstrap/include/athom_oauth_runtime.h").read_text()
    oauth_c = (repo / "components/secure_bootstrap/athom_oauth_runtime.c").read_text()

    assert "athom_cloud_set_favorite_light_onoff(" in cloud_h
    assert "size_t widget_index" in cloud_h
    assert "bool value" in cloud_h
    cloud_decl = cloud_h[cloud_h.index("athom_homey_light_write_result_t athom_cloud_set_favorite_light_onoff("):]
    cloud_decl = cloud_decl[:cloud_decl.index(";") + 1]
    assert "const char *" not in cloud_decl
    assert "url" not in cloud_decl.lower()
    assert "method" not in cloud_decl.lower()
    assert "capability" not in cloud_decl.lower()
    assert "payload" not in cloud_decl.lower()
    assert "json" not in cloud_decl.lower()

    assert "athom_oauth_runtime_dispatch_light_toggle(" in oauth_h
    runtime_decl = oauth_h[oauth_h.index("athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle("):]
    runtime_decl = runtime_decl[:runtime_decl.index(";") + 1]
    assert "const char *" not in runtime_decl
    assert "url" not in runtime_decl.lower()
    assert "method" not in runtime_decl.lower()
    assert "capability" not in runtime_decl.lower()
    assert "payload" not in runtime_decl.lower()
    assert "json" not in runtime_decl.lower()

    write_fn = extract_function(
        cloud_c,
        "athom_homey_light_write_result_t athom_cloud_set_favorite_light_onoff(")
    assert "patch037_build_light_write_body" in write_fn
    assert write_fn.count("patch037_homey_put_once(") == 1
    assert "http_request_limited(" not in write_fn
    assert "HTTP_METHOD_GET" not in write_fn
    assert "HTTP_METHOD_POST" not in write_fn
    assert "HTTP_METHOD_DELETE" not in write_fn
    assert "transactionId" not in write_fn
    assert "opts" not in write_fn
    assert "athom_cloud_fetch_inventory(" not in write_fn
    assert "panel_homey_favorites_parse_and_publish" not in write_fn
    assert "panel_homey_snapshot" not in write_fn
    assert re.search(r"\bfor\s*\(", write_fn) is None
    assert re.search(r"\bwhile\s*\(", write_fn) is None

    path_fn = extract_function(cloud_c, "static bool patch037_build_light_write_path(")
    assert '"/api/manager/devices/device/%s/capability/onoff"' in path_fn
    assert "capabilityId" not in path_fn

    body_fn = extract_function(cloud_c, "static bool patch037_build_light_write_body(")
    assert '"{\\"value\\":true}"' in body_fn
    assert '"{\\"value\\":false}"' in body_fn
    assert "opts" not in body_fn
    assert "transactionId" not in body_fn

    target_fn = extract_function(cloud_c, "static patch037_private_target_result_t patch037_copy_private_light_target(")
    assert "dashboard_binding_index" in target_fn
    assert 'PATCH037_LIGHT_CAPABILITY_ID' in target_fn
    assert 'strcmp(match->raw_capability_id, PATCH037_LIGHT_CAPABILITY_ID)' in target_fn

    classify_fn = extract_function(cloud_c, "static athom_homey_light_write_result_t patch037_classify_light_write(")
    assert "bool write_attempted" in classify_fn
    assert "bool fresh_response_received" in classify_fn
    assert "if (!write_attempted)" in classify_fn
    assert "if (!fresh_response_received)" in classify_fn
    assert classify_fn.index("if (!write_attempted)") < classify_fn.index("if (!fresh_response_received)")
    assert classify_fn.index("if (!fresh_response_received)") < classify_fn.index("fresh_http_status >= 200")
    assert "fresh_http_status >= 200 && fresh_http_status <= 299" in classify_fn
    assert "fresh_http_status == 401" in classify_fn
    assert "ATHOM_HOMEY_LIGHT_WRITE_TRANSPORT_AMBIGUOUS" in classify_fn

    put_once_fn = extract_function(cloud_c, "static esp_err_t patch037_homey_put_once(")
    assert ".disable_auto_redirect = true" in put_once_fn
    assert ".max_authorization_retries = -1" in put_once_fn
    assert "HTTP_METHOD_PUT" in put_once_fn
    assert '"Authorization"' in put_once_fn
    assert '"Content-Type"' in put_once_fn
    assert '"application/json"' in put_once_fn
    assert put_once_fn.count("esp_http_client_perform(") == 1
    assert "esp_http_client_set_redirection" not in put_once_fn
    assert "http_request_limited(" not in put_once_fn
    assert "s_homey_http" not in put_once_fn
    attempted_pos = put_once_fn.index("out->write_attempted = true")
    perform_pos = put_once_fn.index("esp_http_client_perform(")
    assert attempted_pos < perform_pos
    shared_fn = extract_function(cloud_c, "static esp_err_t http_request_limited(")
    assert "disable_auto_redirect" not in shared_fn
    assert "max_authorization_retries" not in shared_fn

    dispatch_fn = extract_function(
        oauth_c,
        "athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle(")
    assert "widget_index != 4U && widget_index != 5U" in dispatch_fn
    ready_call = dispatch_fn.index("panel_homey_favorites_light_toggle_execution_ready(")
    write_call = dispatch_fn.index("athom_cloud_set_favorite_light_onoff(")
    assert ready_call < write_call
    assert "ATHOM_HOMEY_DATA_READY" in dispatch_fn
    assert "homey_session_token" in dispatch_fn
    assert "selected_homey.id" in dispatch_fn
    assert "optimistic_state=no" in dispatch_fn
    assert "athom_cloud_fetch_inventory(" not in dispatch_fn
    assert "xQueueSend(" not in dispatch_fn

    enum_match = re.search(
        r"typedef enum\s*\{\s*ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA\s*=\s*1,\s*\}\s*athom_homey_command_t;",
        oauth_c,
        flags=re.S)
    assert enum_match is not None
    assert "ATHOM_HOMEY_COMMAND_LIGHT" not in oauth_c

    assert 'automatic_retry=no' in cloud_c
    assert 'state_authority=read_only_refresh' in cloud_c
    assert 'state_authority=read_only_refresh' in oauth_c

    for forbidden in [
        "components/secure_bootstrap/panel_ui.c",
        "components/secure_bootstrap/include/panel_ui.h",
        "components/secure_bootstrap/panel_ui_model.c",
        "components/secure_bootstrap/include/panel_ui_model.h",
        "components/secure_bootstrap/panel_homey_favorites.c",
        "components/secure_bootstrap/include/panel_homey_favorites.h",
    ]:
        assert forbidden not in changed_paths(repo)

    added_diff = run(["git", "diff", "--no-ext-diff", "--no-color"], repo).stdout
    assert "LV_OBJ_FLAG_CLICKABLE" not in added_diff
    assert "awning_1" not in added_diff
    assert "awning_2" not in added_diff
    assert "awning_3" not in added_diff
    assert "security" not in added_diff
    assert "transactionId" not in added_diff
    assert '"opts"' not in added_diff

    print("PATCH037_STATIC_VALIDATOR PASS")


def host_test(repo: Path) -> None:
    with tempfile.TemporaryDirectory() as td:
        exe = Path(td) / "patch037_host"
        cmd = [
            "cc",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-I",
            str(repo / "components/secure_bootstrap/include"),
            str(repo / "components/secure_bootstrap/test_host/test_homey_light_toggle_dispatch.c"),
            "-o",
            str(exe),
        ]
        p = run(cmd, repo, check=False)
        if p.returncode != 0:
            print(p.stdout, end="")
            raise SystemExit(p.returncode)
        p = run([str(exe)], repo, check=False)
        print(p.stdout, end="")
        if p.returncode != 0:
            raise SystemExit(p.returncode)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validate-source", action="store_true")
    args = parser.parse_args()
    repo = root()
    if args.validate_source:
        validate_source(repo)
    else:
        host_test(repo)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import re
import subprocess
import tempfile

BASE = "24405241476901170a75321aeeb938cc4b3faf5c"
BRANCH = "patch-038-async-favorite-light-toggle-dispatch-and-authoritative-refresh"
EXPECTED = sorted([
    "components/secure_bootstrap/athom_oauth_runtime.c",
    "components/secure_bootstrap/include/athom_oauth_runtime.h",
    "components/secure_bootstrap/panel_ui.c",
    "components/secure_bootstrap/include/panel_ui.h",
    "components/secure_bootstrap/secure_bootstrap_esp.c",
    "components/secure_bootstrap/test_host/run_patch038_async_light_toggle_tests.py",
    "scripts/validate_patch_038.sh",
])


def root() -> Path:
    return Path(__file__).resolve().parents[3]


def run(cmd: list[str], cwd: Path, check: bool = True) -> subprocess.CompletedProcess[str]:
    p = subprocess.run(cmd, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if check and p.returncode != 0:
        print(p.stdout, end="")
        raise SystemExit(p.returncode)
    return p


def changed_paths(repo: Path) -> list[str]:
    tracked = run(["git", "diff", "--name-only"], repo).stdout.splitlines()
    untracked = run(["git", "ls-files", "--others", "--exclude-standard"], repo).stdout.splitlines()
    return sorted(set(x for x in tracked + untracked if x))


def extract_function(src: str, signature: str) -> str:
    start = src.find(signature)
    if start < 0:
        raise AssertionError(f"missing function: {signature}")
    brace = src.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing body: {signature}")
    depth = 0
    for i in range(brace, len(src)):
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
            if depth == 0:
                return src[start:i + 1]
    raise AssertionError(f"unterminated function: {signature}")


def validate_source(repo: Path) -> None:
    assert run(["git", "branch", "--show-current"], repo).stdout.strip() == BRANCH
    assert run(["git", "rev-parse", "HEAD"], repo).stdout.strip() == BASE
    assert run(["git", "diff", "--cached", "--name-only"], repo).stdout.strip() == ""
    assert changed_paths(repo) == EXPECTED

    oauth_c = (repo / "components/secure_bootstrap/athom_oauth_runtime.c").read_text()
    oauth_h = (repo / "components/secure_bootstrap/include/athom_oauth_runtime.h").read_text()
    ui_c = (repo / "components/secure_bootstrap/panel_ui.c").read_text()
    ui_h = (repo / "components/secure_bootstrap/include/panel_ui.h").read_text()
    esp_c = (repo / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text()

    assert "ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE" in oauth_c
    assert "athom_oauth_runtime_queue_light_toggle(" in oauth_h
    assert "athom_oauth_runtime_light_toggle_pending(" in oauth_h
    assert "athom_oauth_runtime_light_toggle_completion_generation(" in oauth_h
    assert "bool (*request_light_toggle)(void *, size_t widget_index, bool value);" in ui_h
    assert "panel_ui_set_light_toggle_pending(" in ui_h

    queue_fn = extract_function(oauth_c, "athom_light_toggle_queue_result_t athom_oauth_runtime_queue_light_toggle(")
    assert "widget_index != 4U && widget_index != 5U" in queue_fn
    assert "panel_homey_favorites_light_toggle_execution_ready(" in queue_fn
    assert "!s_refresh_job_reserved && !s_light_toggle_job_reserved" in queue_fn
    assert queue_fn.count("xQueueSend(") == 1
    assert "athom_oauth_runtime_dispatch_light_toggle(" not in queue_fn
    assert "athom_cloud_set_favorite_light_onoff(" not in queue_fn

    worker_fn = extract_function(oauth_c, "static void homey_command_worker(")
    assert "command.kind == ATHOM_HOMEY_COMMAND_LIGHT_TOGGLE" in worker_fn
    assert worker_fn.count("athom_oauth_runtime_dispatch_light_toggle(") == 1
    assert "patch038_refresh_authoritative_state_after_write()" in worker_fn
    assert worker_fn.index("athom_oauth_runtime_dispatch_light_toggle(") < worker_fn.index("patch038_refresh_authoritative_state_after_write()")
    assert "patch038_complete_light_toggle_job();" in worker_fn
    assert "automatic_write_retry=no" in worker_fn
    assert "optimistic_state=no" in worker_fn
    assert "state_authority=read_only_refresh" in worker_fn

    refresh_fn = extract_function(oauth_c, "static bool patch038_refresh_authoritative_state_after_write(")
    assert refresh_fn.count("connect_and_fetch_inventory(") == 1
    assert re.search(r"\bfor\s*\(", refresh_fn) is None
    assert re.search(r"\bwhile\s*\(", refresh_fn) is None
    assert "homey_inventory_result_verified(" in refresh_fn
    assert "ATHOM_HOMEY_DATA_READY" in refresh_fn
    assert "ATHOM_HOMEY_DATA_ERROR" in refresh_fn

    classify_fn = extract_function(oauth_c, "static bool __attribute__((unused)) patch038_dispatch_result_requires_authoritative_refresh(")
    for token in [
        "ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED",
        "ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED",
        "ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED",
        "ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS",
        "ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR",
    ]:
        assert token in classify_fn

    sync_fn = extract_function(oauth_c, "athom_light_toggle_dispatch_result_t athom_oauth_runtime_dispatch_light_toggle(")
    assert sync_fn.index("panel_homey_favorites_light_toggle_execution_ready(") < sync_fn.index("athom_cloud_set_favorite_light_onoff(")
    assert "widget_index != 4U && widget_index != 5U" in sync_fn

    ui_event = extract_function(ui_c, "static void panel_light_card_event_for_index(")
    assert "LV_EVENT_CLICKED" in ui_event
    assert "panel_light_card_actionable(" in ui_event
    assert "!ui->model->widget_boolean_value[widget_index]" in ui_event
    assert "request_light_toggle(" in ui_event
    assert "Väntar..." in ui_event
    assert "widget_boolean_value[widget_index] =" not in ui_event
    assert "athom_oauth_runtime" not in ui_event
    assert "athom_cloud" not in ui_event

    actionable = extract_function(ui_c, "static bool panel_light_card_actionable(")
    assert "panel_light_widget_index(widget_index)" in actionable
    assert "panel_homey_favorites_light_toggle_execution_ready(" in actionable
    assert "panel_any_light_toggle_pending(ui)" in actionable
    assert "widget_has_boolean[widget_index]" in actionable

    card_fn = extract_function(ui_c, "static lv_obj_t *create_dashboard_card(")
    assert "index == 4U" in card_fn and "index == 5U" in card_fn
    assert "panel_light_widget4_event" in card_fn and "panel_light_widget5_event" in card_fn
    assert "index == 0U" not in card_fn and "index == 1U" not in card_fn
    assert "index == 2U" not in card_fn and "index == 3U" not in card_fn

    platform_cb = extract_function(esp_c, "static bool panel_light_toggle_request(")
    assert "athom_oauth_runtime_queue_light_toggle(" in platform_cb
    assert "athom_oauth_runtime_dispatch_light_toggle(" not in platform_cb
    assert "athom_cloud_set_favorite_light_onoff(" not in platform_cb

    rotation_fn = extract_function(esp_c, "static void rotation_task(")
    assert "athom_oauth_runtime_light_toggle_completion_generation()" in rotation_fn
    assert "s_homey_dashboard_last_poll_ms = 0U" in rotation_fn
    assert "poll_homey_dashboard_if_due(panel_now_ms)" in rotation_fn
    assert rotation_fn.index("s_homey_dashboard_last_poll_ms = 0U") < rotation_fn.index("poll_homey_dashboard_if_due(panel_now_ms)")
    assert "panel_ui_set_light_toggle_pending(" in rotation_fn

    assert "athom_cloud_set_favorite_light_onoff(" not in ui_c
    assert "athom_cloud_set_favorite_light_onoff(" not in esp_c
    assert "esp_http_client_perform(" not in ui_c
    assert "esp_http_client_perform(" not in esp_c

    diff = run(["git", "diff", "--no-ext-diff", "--no-color"], repo).stdout
    for forbidden in ["awning_1", "awning_2", "awning_3", "transactionId", '"opts"']:
        assert forbidden not in diff

    print("PATCH038_STATIC_VALIDATOR PASS")


def host_test(repo: Path) -> None:
    with tempfile.TemporaryDirectory() as td:
        source = Path(td) / "patch038_host.c"
        source.write_text('''#include <assert.h>\n#include <stdio.h>\n#include "athom_oauth_runtime.c"\nint main(void) {\n    assert(patch037_light_toggle_dispatch_gate(4U, true, true));\n    assert(!patch037_light_toggle_dispatch_gate(3U, true, true));\n    assert(patch037_map_light_write_result(ATHOM_HOMEY_LIGHT_WRITE_ACCEPTED) == ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED);\n    assert(patch037_light_toggle_dispatch_result_name(ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED) != NULL);\n    assert(patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_ACCEPTED));\n    assert(patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_UNAUTHORIZED));\n    assert(patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_REJECTED));\n    assert(patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_TRANSPORT_AMBIGUOUS));\n    assert(patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_INTERNAL_ERROR));\n    assert(!patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_INVALID_WIDGET));\n    assert(!patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_NOT_READY));\n    assert(!patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_NOT_FOUND));\n    assert(!patch038_dispatch_result_requires_authoritative_refresh(ATHOM_LIGHT_TOGGLE_DISPATCH_TARGET_INVALID));\n    puts("PATCH038_ASYNC_LIGHT_TOGGLE_HOST PASS");\n    return 0;\n}\n''')
        exe = Path(td) / "patch038_host"
        cmd = [
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-I", str(repo / "components/secure_bootstrap/include"),
            "-I", str(repo / "components/secure_bootstrap"),
            str(source), "-o", str(exe),
        ]
        print("HOST_COMPILE:", " ".join(cmd))
        run(cmd, repo)
        p = run([str(exe)], repo)
        print(p.stdout, end="")


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

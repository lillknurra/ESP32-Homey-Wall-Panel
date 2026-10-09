#!/usr/bin/env python3
"""Measure the ESP32-S3 status handler frame from the generated IDF build."""

from __future__ import annotations

import json
import os
import re
import shlex
import subprocess
import tempfile
from pathlib import Path


FRAME_LIMIT_BYTES = 512
HTTPD_DEFAULT_STACK_BYTES = 4096
SOURCE_SUFFIX = "/components/secure_bootstrap/athom_oauth_runtime.c"
ALIAS_STORE_SUFFIX = "/components/secure_bootstrap/panel_homey_alias_store.c"
PROVISIONING_SUFFIX = "/components/secure_bootstrap/phone_provisioning_store.c"
PROVISIONING_CHAIN_LIMIT_BYTES = 1024


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


def validate_alias_store_source(root: Path) -> None:
    source = (root / "components/secure_bootstrap/panel_homey_alias_store.c").read_text(
        encoding="utf-8"
    )
    load = function_body(
        source,
        "panel_homey_alias_store_result_t panel_homey_alias_store_load(",
        "panel_homey_alias_store_result_t panel_homey_alias_store_publish(",
    )
    publish = function_body(
        source,
        "panel_homey_alias_store_result_t panel_homey_alias_store_publish(",
        "panel_homey_alias_store_result_t panel_homey_alias_store_wipe(",
    )
    readslot = function_body(source, "static bool readslot(", "static bool inspect_persisted_slot(")
    decode = function_body(
        source,
        "bool panel_homey_alias_record_decode(",
        "panel_homey_alias_slot_t panel_homey_alias_select_slot(",
    )
    for name, body in (("load", load), ("publish", publish)):
        if "calloc(1U, sizeof(*workspace))" not in body:
            fail(f"{name} workspace is not bounded by a fixed zeroed struct")
        if "workspace == NULL" not in body:
            fail(f"{name} allocation failure does not fail closed")
        wiped_frees = len(re.findall(
            r"sensitive_zero\(workspace, sizeof\(\*workspace\)\);\s*free\(workspace\);",
            body,
        ))
        if wiped_frees != body.count("free(workspace);") or wiped_frees == 0:
            fail(f"{name} workspace is not zeroed immediately before free")
    if "NVS_READONLY" not in load or load.count("readslot(") != 2:
        fail("load no longer reads both slots through the read-only NVS path")
    if publish.count("readslot(") != 2:
        fail("publish no longer reads both slots before choosing a target")
    if re.search(r"^\s+uint8_t blob\[RECORD_SIZE\];", readslot, re.MULTILINE):
        fail("readslot has reintroduced a stack-resident record blob")
    if "uint8_t tmp[RECORD_SIZE]" in decode:
        fail("record decode has reintroduced a stack-resident CRC copy")
    write_order = (
        "nvs_set_blob(",
        "nvs_commit(",
        "nvs_get_blob(",
        "memcmp(",
        "nvs_set_u8(",
        "nvs_commit(",
    )
    offset = 0
    for operation in write_order:
        found = publish.find(operation, offset)
        if found < 0:
            fail(f"publication order is missing {operation}")
        offset = found + len(operation)
    if publish.count("nvs_set_blob(") != 1 or publish.count("nvs_set_u8(") != 1:
        fail("publication contains unexpected NVS write operations")
    if publish.count("free(workspace);") != 1 or "goto publish_cleanup;" not in publish:
        fail("publish does not use one cleanup path after allocation")
    print("PATCH057_ALIAS_STORE_SOURCE_INVARIANTS=PASS")


def main() -> None:
    root = Path(__file__).resolve().parents[3]
    validate_alias_store_source(root)
    build = root / "build"
    compile_commands = build / "compile_commands.json"
    if not compile_commands.is_file():
        fail("build/compile_commands.json is missing; run the ESP-IDF build first")

    entries = json.loads(compile_commands.read_text(encoding="utf-8"))
    def measure(source_suffix: str, output_name: str, symbols: dict[str, str]) -> dict[str, int]:
        entry = next(
            (item for item in entries if item.get("file", "").endswith(source_suffix)),
            None,
        )
        if entry is None:
            fail(f"compile database has no {source_suffix.rsplit('/', 1)[-1]} entry")

        command = list(entry.get("arguments") or shlex.split(entry["command"]))
        try:
            output_index = command.index("-o")
            source_index = next(
                i for i, arg in enumerate(command) if arg.endswith(source_suffix)
            )
        except (ValueError, StopIteration):
            fail("could not locate compiler output/source arguments")

        source = root / source_suffix.lstrip("/")
        if not source.is_file():
            fail(f"compiler source is missing: {source_suffix}")

        with tempfile.TemporaryDirectory(prefix="http_stack_usage_") as temp:
            output = Path(temp) / output_name
            command[output_index + 1] = str(output)
            command[source_index] = str(source)
            if "-MF" in command:
                command[command.index("-MF") + 1] = str(output.with_suffix(".d"))
            if "-MT" in command:
                command[command.index("-MT") + 1] = str(output)
            command.insert(output_index, "-fstack-usage")
            result = subprocess.run(
                command,
                cwd=entry["directory"],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            if result.returncode != 0:
                fail("Xtensa compiler failed:\n" + result.stderr[-4000:])

            usage_file = output.with_suffix(".su")
            if not usage_file.is_file():
                fail("compiler did not produce stack-usage information")
            usage_text = usage_file.read_text(encoding="utf-8")
            measured: dict[str, int] = {}
            for symbol, filename in symbols.items():
                match = re.search(
                    rf"(?:^|/){re.escape(filename)}:\d+:\d+:{re.escape(symbol)}\t(\d+)\t(\w+)",
                    usage_text,
                    re.MULTILINE,
                )
                if match is None:
                    fail(f"stack-usage output does not contain {symbol}")
                if match.group(2) != "static":
                    fail(f"{symbol} stack frame is not statically bounded: {match.group(2)}")
                measured[symbol] = int(match.group(1))
            return measured

    status_frames = measure(
        SOURCE_SUFFIX,
        "athom_oauth_runtime.o",
        {"status_get": "athom_oauth_runtime.c"},
    )
    status_frame = status_frames["status_get"]
    if status_frame > FRAME_LIMIT_BYTES:
        fail(f"status_get frame {status_frame} exceeds {FRAME_LIMIT_BYTES}-byte guard")

    provisioning_frames = measure(
        PROVISIONING_SUFFIX,
        "panel_homey_awning_provisioning.o",
        {
            "awning_bindings_post": "phone_provisioning_store.c",
            "patch051_read_body": "phone_provisioning_store.c",
        },
    )
    store_frames = measure(
        ALIAS_STORE_SUFFIX,
        "panel_homey_alias_store.o",
        {
            "panel_homey_alias_store_load": "panel_homey_alias_store.c",
            "readslot": "panel_homey_alias_store.c",
            "panel_homey_alias_record_decode": "panel_homey_alias_store.c",
            "panel_homey_alias_store_publish": "panel_homey_alias_store.c",
            "panel_homey_alias_record_encode": "panel_homey_alias_store.c",
            "panel_homey_alias_runtime_activate": "panel_homey_alias_store.c",
            "panel_homey_alias_sha256": "panel_homey_alias_store.c",
            "crc32": "panel_homey_alias_store.c",
            "crc32_record_with_zeroed_crc": "panel_homey_alias_store.c",
        },
    )

    load_chain = provisioning_frames["awning_bindings_post"] + sum(
        store_frames[name] for name in (
            "panel_homey_alias_store_load",
            "readslot",
            "panel_homey_alias_record_decode",
        )
    )
    load_activation_chain = (
        provisioning_frames["awning_bindings_post"]
        + store_frames["panel_homey_alias_store_load"]
        + store_frames["panel_homey_alias_runtime_activate"]
    )
    load_crc_chain = load_chain + store_frames["crc32_record_with_zeroed_crc"]
    publish_chain = provisioning_frames["awning_bindings_post"] + sum(
        store_frames[name] for name in (
            "panel_homey_alias_store_publish",
            "readslot",
            "panel_homey_alias_record_decode",
        )
    )
    publish_crc_chain = publish_chain + store_frames["crc32_record_with_zeroed_crc"]
    publish_encode_chain = (
        provisioning_frames["awning_bindings_post"]
        + store_frames["panel_homey_alias_store_publish"]
        + store_frames["panel_homey_alias_record_encode"]
        + store_frames["crc32"]
    )
    publish_digest_chain = (
        provisioning_frames["awning_bindings_post"]
        + store_frames["panel_homey_alias_store_publish"]
        + store_frames["panel_homey_alias_sha256"]
    )
    deepest_chain = max(
        load_chain,
        load_activation_chain,
        load_crc_chain,
        publish_chain,
        publish_crc_chain,
        publish_encode_chain,
        publish_digest_chain,
    )
    if deepest_chain > PROVISIONING_CHAIN_LIMIT_BYTES:
        fail(
            f"cumulative HTTP awning application chain {deepest_chain} exceeds "
            f"{PROVISIONING_CHAIN_LIMIT_BYTES}-byte guard; "
            f"load={load_chain}, load_activation={load_activation_chain}, "
            f"load_crc={load_crc_chain}, publish={publish_chain}, "
            f"publish_crc={publish_crc_chain}, "
            f"publish_encode={publish_encode_chain}, "
            f"publish_digest={publish_digest_chain}, "
            f"frames={provisioning_frames | store_frames}"
        )

    idf_path = Path(os.environ.get("IDF_PATH", "~/GitHub/esp-idf-v6.0.1")).expanduser()
    httpd_header = idf_path / "components/esp_http_server/include/esp_http_server.h"
    if not httpd_header.is_file():
        fail("ESP-IDF HTTP server header is unavailable at IDF_PATH")
    header_text = httpd_header.read_text(encoding="utf-8")
    default_config = re.search(
        r"#define\s+HTTPD_DEFAULT_CONFIG\(\).*?\.stack_size\s*=\s*(\d+)",
        header_text,
        re.DOTALL,
    )
    if default_config is None or int(default_config.group(1)) != HTTPD_DEFAULT_STACK_BYTES:
        fail("HTTPD_DEFAULT_CONFIG stack is not the audited 4096 bytes")

    print(
        "PASS: ESP32-S3 compiler-derived frames: "
        f"status_get={status_frame} bytes; "
        f"awning_bindings_post={provisioning_frames['awning_bindings_post']} bytes; "
        f"patch051_read_body={provisioning_frames['patch051_read_body']} bytes; "
        f"alias_load={store_frames['panel_homey_alias_store_load']} bytes; "
        f"readslot={store_frames['readslot']} bytes; "
        f"decode={store_frames['panel_homey_alias_record_decode']} bytes; "
        f"alias_publish={store_frames['panel_homey_alias_store_publish']} bytes; "
        f"encode={store_frames['panel_homey_alias_record_encode']} bytes; "
        f"alias_sha256={store_frames['panel_homey_alias_sha256']} bytes; "
        f"deepest_app_chain={deepest_chain} bytes; "
        f"chain_guard={PROVISIONING_CHAIN_LIMIT_BYTES}; "
        f"HTTPD default={HTTPD_DEFAULT_STACK_BYTES} bytes"
    )


if __name__ == "__main__":
    main()

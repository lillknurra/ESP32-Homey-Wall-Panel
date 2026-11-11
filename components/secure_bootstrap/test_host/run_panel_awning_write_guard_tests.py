#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"

ESP_ERR_H = r"""
#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_INVALID_SIZE 0x104
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_INVALID_CRC 0x107
"""

NVS_H = r"""
#pragma once
#include <stddef.h>
#include "esp_err.h"
typedef int nvs_handle_t;
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define ESP_ERR_NVS_NOT_FOUND 0x1102
esp_err_t nvs_open(const char *, int, nvs_handle_t *);
esp_err_t nvs_get_blob(nvs_handle_t, const char *, void *, size_t *);
esp_err_t nvs_set_blob(nvs_handle_t, const char *, const void *, size_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
"""

with tempfile.TemporaryDirectory() as temporary:
    temp = Path(temporary)
    include = temp / "include"
    include.mkdir()
    (include / "esp_err.h").write_text(ESP_ERR_H)
    (include / "nvs.h").write_text(NVS_H)
    binary = temp / "test_panel_awning_write_guard"
    command = [
        "cc", "-DESP_PLATFORM", "-I", str(include),
        "-I", str(COMPONENT / "include"), "-std=c11", "-Wall", "-Wextra",
        "-Werror", "-pedantic",
        str(COMPONENT / "panel_awning_write_guard.c"),
        str(COMPONENT / "test_host/test_panel_awning_write_guard.c"),
        "-o", str(binary),
    ]
    print("PANEL_AWNING_GUARD_ADAPTER_COMMAND:", " ".join(command))
    subprocess.run(command, check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)

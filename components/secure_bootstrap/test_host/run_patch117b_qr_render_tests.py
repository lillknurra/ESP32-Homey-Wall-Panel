#!/usr/bin/env python3
"""Render the pinned LVGL QR widget and independently decode its pixels."""
from pathlib import Path
import argparse
import subprocess
import tempfile

from PIL import Image

root = Path(__file__).resolve().parents[3]
component = root / "components/secure_bootstrap"
lvgl = root / "managed_components/lvgl__lvgl"
parser = argparse.ArgumentParser()
parser.add_argument("--output-dir", type=Path, required=True)
args = parser.parse_args()
out = args.output_dir.resolve()
out.mkdir(parents=True, exist_ok=True)

with tempfile.TemporaryDirectory(prefix="patch117b-lvgl-qr-") as temp:
    temp_path = Path(temp)
    cmake_text = f'''cmake_minimum_required(VERSION 3.16)
project(patch117b_qr C CXX)
set(LV_CONF_SKIP ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("{lvgl}" lvgl)
target_compile_definitions(lvgl PUBLIC LV_USE_QRCODE=1 LV_USE_LOG=0 LV_MEM_SIZE=262144 LV_DRAW_BUF_STRIDE_ALIGN=1 LV_FONT_MONTSERRAT_28=1)
add_executable(qr_render "{component / 'panel_wifi_qr.c'}" "{component / 'homey_panel_font_22.c'}" "{component / 'test_host/test_patch117b_qr_render.c'}")
target_include_directories(qr_render PRIVATE "{component / 'include'}")
target_link_libraries(qr_render PRIVATE lvgl m)
'''
    (temp_path / "CMakeLists.txt").write_text(cmake_text, encoding="utf-8")
    subprocess.run(["cmake", "-S", str(temp_path), "-B", str(temp_path / "build"), "-G", "Ninja"], check=True)
    subprocess.run(["cmake", "--build", str(temp_path / "build"), "-j", "8"], check=True)
    subprocess.run([str(temp_path / "build/qr_render"), str(out)], check=True)

decoder = out / "decode_qr_vision"
subprocess.run([
    "swiftc", str(component / "test_host/decode_qr_vision.swift"),
    "-framework", "AppKit", "-framework", "Vision", "-o", str(decoder),
], check=True)
expected = {
    "first-install": "WIFI:T:WPA;S:HomeyPanel-Setup;P:SYNTHETIC88AB;;",
    "wifi-change": "WIFI:T:WPA;S:HomeyPanel-Setup;P:SYNTHETIC88AB;;",
    "rotated-escaped": "WIFI:T:WPA;S:AP\\;Lab\\,Unit\\\";P:PASS\\\\\\;WORD8;;",
    "max-credentials": "WIFI:T:WPA;S:" + r"\;" * 32 + ";P:" + r"\;" * 63 + ";;",
}
for name, value in expected.items():
    ppm = out / f"{name}.ppm"
    png = out / f"{name}.png"
    with Image.open(ppm) as image:
        image.save(png)
        rgb = image.convert("RGB")
        width, height = rgb.size
        pixels = rgb.load()
        assert all(pixels[x, 0] == (255, 255, 255) for x in range(width))
        assert all(pixels[x, height - 1] == (255, 255, 255) for x in range(width))
        assert all(pixels[0, y] == (255, 255, 255) for y in range(height))
        assert all(pixels[width - 1, y] == (255, 255, 255) for y in range(height))
    decoded = subprocess.run([str(decoder), str(png)], check=True, text=True, capture_output=True).stdout.rstrip("\n")
    assert decoded == value, f"{name} QR decode did not match active synthetic session: {decoded!r}"
    print(f"LVGL_QR_INDEPENDENT_DECODE {name}=PASS")
with Image.open(out / "startup-screen.ppm") as image:
    image.save(out / "startup-screen.png")
    assert image.size == (480, 480)
print("PATCH117B_QR_RENDER_AND_VISION_DECODE PASS")

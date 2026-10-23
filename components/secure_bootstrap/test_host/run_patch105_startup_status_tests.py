#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="patch105-startup-") as temp:
    binary = Path(temp) / "test_patch105_startup_status"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"),
        str(ROOT / "panel_startup_status.c"),
        str(ROOT / "test_host" / "test_patch105_startup_status.c"),
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)

source = (ROOT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
logo_source = (ROOT / "panel_startup_logo.c").read_text(encoding="utf-8")
logo_data = (ROOT / "assets" / "panel_startup_logo_a8.bin").read_bytes()
assert '"panel_startup_status.c"' in cmake
assert '"panel_startup_logo.c"' in cmake
assert "target_add_binary_data" in cmake
assert len(logo_data) == 170 * 170
assert any(logo_data) and any(value == 0 for value in logo_data)
assert ".cf = LV_COLOR_FORMAT_A8" in logo_source
assert ".w = 170U" in logo_source and ".h = 170U" in logo_source
assert "lv_image_set_src(s_startup_logo, &panel_startup_logo_image)" in source
assert "LV_ALIGN_CENTER, 0, -78" in source
assert "LV_ALIGN_BOTTOM_MID, 0, -24" in source
assert "&lv_font_montserrat_28" in source
assert '"Wi-Fi ansluten"' in source
assert '"Homey READY"' in source
assert '"Homey-data kunde inte hämtas"' in source
assert "bsp_display_lock(0)" in source
assert "vTaskDelay" not in source[source.index("static void update_homey_data_boot_view("):source.index("static void set_reconfigure_button_visible(")]
assert "panel_show_startup_shell" not in source

#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEST_DIR = Path(__file__).resolve().parent
IDF = Path("/Users/petter/GitHub/esp-idf-v6.0.1")

# Existing model/normalization tests now also cover settings navigation state.
subprocess.run(["python3", str(TEST_DIR / "run_panel_ui_tests.py")], check=True)

with tempfile.TemporaryDirectory(prefix="patch074-store-") as temp:
    binary = Path(temp) / "test_patch074_panel_ui_store"
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-DESP_PLATFORM", "-I", str(TEST_DIR / "fakes"),
        "-I", str(ROOT / "include"),
        str(ROOT / "panel_ui_model.c"),
        str(ROOT / "panel_homey_dashboard_binding.c"),
        str(ROOT / "panel_roller_model.c"),
        str(ROOT / "panel_ui_store.c"),
        str(TEST_DIR / "test_patch074_panel_ui_store.c"), "-lm",
        "-o", str(binary),
    ]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)

ui = (ROOT / "panel_ui.c").read_text(encoding="utf-8")
model = (ROOT / "panel_ui_model.c").read_text(encoding="utf-8")
secure = (ROOT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

assert '"Inställningar"' in ui and '"Avancerat"' in ui
assert '"SKÄRM"' not in ui and '"ANSLUTNINGAR"' in ui and '"SYSTEM"' in ui
assert '"Sparar..."' not in ui and '"Sparat"' not in ui
assert '"Kunde inte spara"' in ui
assert 'MODAL_SAVING' not in ui and 'MODAL_SAVED' not in ui
assert 'MODAL_SAVED' not in ui
assert 'ui->modal_kind = MODAL_NONE;' in ui
assert 'lv_obj_add_flag(ui->status_overlay, LV_OBJ_FLAG_HIDDEN);' in ui
assert 'navigation_button_new(ui->settings_header, NAV_CLOSE,' in ui
assert 'ui->settings_header, NAV_BACK, back_settings_event, ui' in ui
assert 'lv_obj_remove_flag(ui->settings_layer, LV_OBJ_FLAG_SCROLLABLE)' in ui
assert 'lv_obj_set_scroll_dir(ui->settings_content, LV_DIR_VER)' in ui
assert 'lv_obj_add_flag(ui->settings_content, LV_OBJ_FLAG_SCROLLABLE)' in ui
assert 'lv_obj_set_size(button, LV_PCT(100), 56);' in ui
assert "#define NAV_BUTTON_W 52" in ui and "#define NAV_BUTTON_H 48" in ui
assert 'lv_obj_add_flag(ui->settings_back_button, LV_OBJ_FLAG_HIDDEN)' in ui
assert 'panel_ui_open_settings_model(ui->model)' in ui
assert 'panel_ui_back_to_settings_model(ui->model)' in ui
assert 'Ej tillgänglig' in ui
assert 'settings_button_new(ui->settings_main_body, "Avancerat' in ui
assert 'panel_ui_open_advanced_settings_model' in model
assert 'panel_ui_back_to_settings_model' in model
assert 'lv_obj_get_scroll_y(ui->settings_content)' in ui
assert 'lv_obj_is_scrolling(ui->settings_content)' in ui

assert 'panel_ui_store.c' in cmake
assert secure.index('panel_ui_settings_storage_start();') < secure.index('display_init()')
assert secure.index('s_panel_model.settings = s_panel_settings_boot;') < secure.index('panel_ui_create(&s_panel_ui')
assert 'xQueueCreate(1U' in secure
assert 'xQueueOverwrite(s_panel_settings_queue, &request)' in secure
assert 'pdMS_TO_TICKS(300)' in secure and 'debounce_window < 4U' in secure
assert 'panel_ui_store_save(&request.settings)' in secure
assert 'panel_ui_settings_equal(&request.settings' in secure
worker_start = secure.index('static void panel_ui_settings_persist_worker(')
worker_end = secure.index('\nstatic void panel_ui_settings_storage_start', worker_start)
worker = secure[worker_start:worker_end]
assert 'bsp_display_lock' not in worker and 'lv_' not in worker
assert 'panel_ui_store_save' in worker
assert 'panel_ui_set_settings_save_status' in secure
assert 'panel_ui_restore_settings' in secure
assert 'nvs_flash_erase' not in worker
assert 'PANEL_UI settings_changed=true persistence=false' not in secure
assert 'panel_ui_settings_publish_result();' in secure
assert secure.index('panel_ui_settings_publish_result();') > secure.index('panel_display_lock_traced(50, false')

# Patch073 time-sync source remains present and unchanged in its essential contract.
assert 'start_network_time_sync_once();' in secure
assert 'CET-1CEST,M3.5.0/2,M10.5.0/3' in (ROOT / "include/panel_clock.h").read_text(encoding="utf-8")

print("PATCH074_SETTINGS_SOURCE_INVARIANTS PASS")

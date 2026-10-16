from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="patch076-v28-") as directory:
    binary = Path(directory) / "layout"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                    "-I", str(root / "include"),
                    str(Path(__file__).with_name("test_patch076_v28_layout.c")),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

ui = (root / "panel_ui.c").read_text()
creation = ui[ui.index("/* Create every logical widget exactly once"):
              ui.index("lv_obj_t *indicator =")]
assert "index < PANEL_UI_WIDGET_COUNT" in creation
assert creation.count("create_roller_card(") == 1
assert creation.count("create_light_card(") == 1
assert creation.count("create_security_card(") == 1
assert "ui->pages[placement->page]" in creation
assert 'settings_section_new(ui->settings_main_body, "SKÄRM")' not in ui
assert ui.count("settings_button_new(ui->settings_main_body,") == 6
assert "LV_DIR_NONE" in ui[ui.index("static void back_settings_event"):
                            ui.index("static void notify_settings")]
controls = ui[ui.index("static lv_obj_t *create_markis_control"):
              ui.index("static lv_obj_t *create_roller_card")]
assert "LV_STATE_DISABLED" in controls
assert "lv_obj_add_event_cb" not in controls
security = ui[ui.index("static lv_obj_t *create_security_card"):
              ui.index("static void dots_render")]
assert "lv_obj_add_event_cb" not in security
assert "panel_light_widget4_event, LV_EVENT_CLICKED" in ui
assert "panel_light_widget5_event, LV_EVENT_CLICKED" in ui
print("PATCH076_V28_UI_CREATION_AND_SAFETY PASS")

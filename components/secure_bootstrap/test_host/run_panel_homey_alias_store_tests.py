#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
store_source = (root / "panel_homey_alias_store.c").read_text()
inspect_begin = store_source.index("static bool inspect_persisted_slot(")
inspect_end = store_source.index(
    "panel_homey_alias_store_result_t panel_homey_alias_store_load(",
    inspect_begin,
)
inspect_body = store_source[inspect_begin:inspect_end]
assert "panel_homey_alias_store_result_t panel_homey_alias_store_inspect(" in inspect_body
assert 'PANEL_HOMEY_ALIAS_STORE_NAMESPACE,\n        NVS_READONLY' in inspect_body
assert "nvs_get_blob(" in inspect_body and "nvs_get_u8(" in inspect_body
for forbidden in (
    "NVS_READWRITE",
    "nvs_set_",
    "nvs_erase_",
    "nvs_commit(",
):
    assert forbidden not in inspect_body, f"read-only inspector contains {forbidden}"
inspect_api = inspect_body[inspect_body.index(
    "panel_homey_alias_store_result_t panel_homey_alias_store_inspect("):
]
assert inspect_api.count("inspect_store_once(") == 2, (
    "inspector must compare two complete consecutive observations")
with tempfile.TemporaryDirectory() as temp:
    output = pathlib.Path(temp) / "test_panel_homey_alias_store"
    command = [
        "cc", "-std=c11", "-D_POSIX_C_SOURCE=200809L",
        "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(root / "include"),
        str(root / "panel_homey_alias_store.c"),
        str(root / "panel_homey_dashboard_binding.c"),
        str(root / "test_host/test_panel_homey_alias_store.c"),
        "-o", str(output),
    ]
    print("ALIAS_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True)
    subprocess.run([str(output)], check=True)
print("PATCH_015_ALIAS_HOST_RUNNER=PASS")
print("PATCH056_ALIAS_INSPECTOR_READ_ONLY_SOURCE_GATE=PASS")

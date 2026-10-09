#!/usr/bin/env python3
from pathlib import Path
import json
import subprocess
root=Path(__file__).resolve().parents[3]
out=root/"build_host"/"test_athom_cloud_model"
out.parent.mkdir(parents=True,exist_ok=True)
cmd=[
    "cc","-std=c11","-Wall","-Wextra","-Werror",
    "-I",str(root/"components/secure_bootstrap/include"),
    str(root/"components/secure_bootstrap/athom_cloud_model.c"),
    str(root/"components/secure_bootstrap/panel_homey_dashboard_binding.c"),
    str(root/"components/secure_bootstrap/test_host/test_athom_cloud_model.c"),
    "-o",str(out)
]
subprocess.run(cmd,check=True)
result=subprocess.run([str(out)],check=True,capture_output=True,text=True)
print(result.stdout,end="")
lines=result.stdout.splitlines()
status_start=lines.index("LIVE_STATUS_JSON_BEGIN")
status_end=lines.index("LIVE_STATUS_JSON_END",status_start+1)
assert status_end == status_start + 2, "expected one live-status JSON line"
status=json.loads(lines[status_start+1])
assert status["selected_homey"] == {"name": "Mamma"}
assert status["homeys"] == [
    {"name": "Mamma"}, {"name": "Sommarhus"}, {"name": "Reserv"}
]
assert all("id" not in item for item in status["homeys"])
assert "id" not in status["selected_homey"]
for forbidden in (
    "PRIVATE_SELECTED_HOMEY_ID_FIXTURE",
    "PRIVATE_HOMEY_LIST_ID_FIXTURE_B",
    "PRIVATE_HOMEY_LIST_ID_FIXTURE_C",
    "PRIVATE_LOCAL_SECURE_URL_FIXTURE",
    "PRIVATE_LOCAL_URL_FIXTURE",
    "PRIVATE_REMOTE_URL_FIXTURE",
    "PRIVATE_ACCESS_TOKEN_FIXTURE",
    "PRIVATE_REFRESH_TOKEN_FIXTURE",
    "PRIVATE_SESSION_TOKEN_FIXTURE",
):
    assert forbidden not in result.stdout
start=lines.index("PATCH056_ALIAS_STORE_JSON_BEGIN")
end=lines.index("PATCH056_ALIAS_STORE_JSON_END",start+1)
assert end == start + 2, "expected one bounded JSON line"
payload=json.loads(lines[start+1])
store=payload["alias_store"]
assert store["active_slot_hint"] == "a"
assert store["effective_selected_slot"] == "a"
assert store["selection_basis"] == "active_hint"
assert store["slots"]["a"]["generation"] == 1
assert store["slots"]["a"]["entry_count"] == 0
assert store["slots"]["a"]["awning_1_present"] is False
assert store["slots"]["b"]["generation"] == 2
assert store["slots"]["b"]["entry_count"] == 6
assert store["slots"]["b"]["awning_3_present"] is True
for slot in store["slots"].values():
    assert set(slot) == {
        "present", "structurally_valid", "generation", "entry_count",
        "selected_homey_match", "awning_1_present", "awning_2_present",
        "awning_3_present",
    }
assert "raw_device_id" not in result.stdout
assert "raw_capability_id" not in result.stdout
assert "digest" not in result.stdout
print("PATCH056_ALIAS_STORE_JSON_SCHEMA=PASS")

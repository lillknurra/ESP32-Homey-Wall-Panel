#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory() as t:
 o=Path(t)/"test"
 subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic","-I",str(r/"components/secure_bootstrap/include"),"-I",str(r/"managed_components/espressif__cjson/cJSON"),str(r/"components/secure_bootstrap/panel_homey_favorites.c"),str(r/"components/secure_bootstrap/panel_ui_model.c"),str(r/"managed_components/espressif__cjson/cJSON/cJSON.c"),str(r/"components/secure_bootstrap/test_host/test_panel_homey_favorites.c"),"-lm","-o",str(o)],cwd=r,check=True)
 subprocess.run([str(o)],cwd=r,check=True)

#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
 b=Path(td)/"test_panel_homey_light_provisioning"
 cmd=["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic","-I",str(root/"include"),str(root/"panel_homey_light_provisioning.c"),str(Path(__file__).with_name("test_panel_homey_light_provisioning.c")),"-o",str(b)]
 print("HOST_COMPILE:"," ".join(cmd));subprocess.run(cmd,check=True);subprocess.run([str(b)],check=True)
 print("PANEL_HOMEY_LIGHT_PROVISIONING_RUNNER PASS")

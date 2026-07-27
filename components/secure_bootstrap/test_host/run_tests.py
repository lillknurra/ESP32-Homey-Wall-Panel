#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
 out=Path(td)/'test_phone_provisioning'
 cmd=['cc','-std=c11','-Wall','-Wextra','-Werror','-I',str(root/'include'),str(root/'phone_provisioning_logic.c'),str(root/'phone_provisioning_mock_provider.c'),str(Path(__file__).with_name('test_phone_provisioning.c')),'-o',str(out)]
 subprocess.run(cmd,check=True); subprocess.run([str(out)],check=True)

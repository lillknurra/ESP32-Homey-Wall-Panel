#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory() as td:
 out=Path(td)/"test_athom_oauth_flow"
 cmd=["cc","-std=c11","-Wall","-Wextra","-Werror","-I",str(root/"components/secure_bootstrap/include"),str(root/"components/secure_bootstrap/athom_oauth_flow.c"),str(root/"components/secure_bootstrap/test_host/test_athom_oauth_flow.c"),"-o",str(out)]
 subprocess.run(cmd,check=True)
 subprocess.run([str(out)],check=True)

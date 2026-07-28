#!/usr/bin/env python3
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[3]
out=root/"build_host"/"test_athom_cloud_model"
out.parent.mkdir(parents=True,exist_ok=True)
cmd=[
    "cc","-std=c11","-Wall","-Wextra","-Werror",
    "-I",str(root/"components/secure_bootstrap/include"),
    str(root/"components/secure_bootstrap/athom_cloud_model.c"),
    str(root/"components/secure_bootstrap/test_host/test_athom_cloud_model.c"),
    "-o",str(out)
]
subprocess.run(cmd,check=True)
subprocess.run([str(out)],check=True)

from pathlib import Path
import re
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
runtime = (component / "athom_oauth_runtime.c").read_text()
secure = (component / "secure_bootstrap_esp.c").read_text()
assert '"/homey/debug/runtime-events",HTTP_GET,runtime_events_get,NULL' in runtime
routes = runtime.split("const httpd_uri_t handlers[]={", 1)[1].split("};", 1)[0]
assert len(re.findall(r'\{"/[^\"]+",HTTP_(?:GET|POST),', routes)) == 12
assert "cfg.max_uri_handlers=26" in secure
assert "athom_oauth_runtime_report_bootstrap_failure(registration_err)" in secure
assert "homey_registration_sequence_run(" in runtime
assert "homey_start_runtime_after_registration" in runtime

with tempfile.TemporaryDirectory() as td:
    binary = Path(td) / "registration"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(component / "include"),
        str(component / "homey_registration_sequence.c"),
        str(component / "test_host/test_homey_registration_sequence.c"),
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("Patch117H bootstrap handler capacity tests: PASS")

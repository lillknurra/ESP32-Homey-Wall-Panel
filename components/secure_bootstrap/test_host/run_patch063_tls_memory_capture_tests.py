#!/usr/bin/env python3
"""Compile and exercise the production bounded TLS allocation observer."""
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
CLIENT_PATH = ROOT / "components/secure_bootstrap/athom_cloud_client.c"
TEMPLATE = Path(__file__).with_name("test_patch063_tls_memory_capture.c")
CLIENT = CLIENT_PATH.read_text(encoding="utf-8")
MARKER_BEGIN = "/* PATCH063_TLS_MEMORY_CAPTURE_BEGIN */"
MARKER_END = "/* PATCH063_TLS_MEMORY_CAPTURE_END */"
TEMPLATE_MARKER = "/* PATCH063_PRODUCTION_CAPTURE */"

if CLIENT.count(MARKER_BEGIN) != 1 or CLIENT.count(MARKER_END) != 1:
    raise SystemExit("expected one bounded TLS memory capture source block")
if CLIENT.index(MARKER_BEGIN) >= CLIENT.index(MARKER_END):
    raise SystemExit("invalid TLS memory capture source markers")
production = CLIENT[CLIENT.index(MARKER_BEGIN):CLIENT.index(MARKER_END) + len(MARKER_END)]
test_source = TEMPLATE.read_text(encoding="utf-8")
if test_source.count(TEMPLATE_MARKER) != 1:
    raise SystemExit("expected one production capture insertion point")


def function_body(signature: str) -> str:
    start = CLIENT.index(signature)
    scrubbed = re.sub(
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
        lambda match: " " * len(match[0]), CLIENT, flags=re.S)
    brace = scrubbed.index("{", start + len(signature))
    depth = 0
    for index in range(brace, len(CLIENT)):
        depth += (scrubbed[index] == "{") - (scrubbed[index] == "}")
        if depth == 0:
            return CLIENT[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")


request = function_body("static esp_err_t http_request_limited(")
begin = request.index("patch019a16e_begin_transport_alloc_capture();")
perform = request.index("esp_http_client_perform(ctx->handle)")
finish = request.index("patch019a16e_finish_transport_alloc_capture();")
copy = request.index("last_tls_memory_diagnostic = s_patch019a16e_memory_diagnostic;")
assert begin < perform < finish < copy
assert request.count("patch019a16e_begin_transport_alloc_capture();") == 1
assert request.count("patch019a16e_finish_transport_alloc_capture();") == 1
assert "tls_error == 141" in request
assert "patch019a16e_log_failed_alloc(ctx->role" in request

generated = test_source.replace(TEMPLATE_MARKER, production)
with tempfile.TemporaryDirectory() as temp_dir:
    source = Path(temp_dir) / "test_patch063_tls_memory_capture.c"
    binary = Path(temp_dir) / "test_patch063_tls_memory_capture"
    source.write_text(generated, encoding="utf-8")
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
         str(source), "-o", str(binary)], check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=ROOT)

print("PATCH063_TLS_MEMORY_CAPTURE_RUNNER PASS")

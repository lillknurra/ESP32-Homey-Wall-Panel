#!/usr/bin/env python3
import pathlib
import os
import re
import subprocess
import tempfile

component = pathlib.Path(__file__).resolve().parents[1]
candidate_sockets = component / "lwip" / "src" / "api" / "sockets.c"
idf_path = os.environ.get("IDF_PATH")
if not idf_path:
    raise SystemExit("IDF_PATH must point to the pinned ESP-IDF source tree")
idf_root = pathlib.Path(idf_path).resolve()
baseline_sockets = idf_root / "components" / "lwip" / "lwip" / "src" / "api" / "sockets.c"
transport = component.parent / "tcp_transport" / "transport_ssl.c"
baseline_transport = idf_root / "components" / "tcp_transport" / "transport_ssl.c"
diag_source = component / "port" / "close_diag.c"
test_source = component / "test_host" / "test_close_diag.c"

def function_body(path, signature):
    text = path.read_text(encoding="utf-8")
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    raise AssertionError(f"unclosed function: {signature}")

def strip_diagnostic_hooks(body):
    body = re.sub(r"\s*lwip_close_diag_(?:socket|transport)_event\(.*?\);", "", body, flags=re.S)
    body = re.sub(r"\s*lwip_close_diag_scope_(?:begin|end)\([^;]*\);", "", body)
    body = re.sub(r"^\s*const uint32_t (?:connect_correlation_id|close_correlation_id) =\s*;\s*", "", body, flags=re.M)
    body = re.sub(r"^\s*const uint32_t (?:connect_correlation_id|close_correlation_id) =\s*$\n?", "", body, flags=re.M)
    body = re.sub(r"^\s*const uint32_t (?:connect_correlation_id|close_correlation_id) = .*?;\s*", "", body, flags=re.M)
    body = re.sub(r"^\s*const uint8_t connect_flags =.*?;\s*", "", body, flags=re.M | re.S)
    body = re.sub(r"^\s*const bool close_call_expected =.*?;\s*", "", body, flags=re.M | re.S)
    body = re.sub(r"^\s*int tls_sockfd =.*?;\s*", "", body, flags=re.M | re.S)
    body = re.sub(r"^\s*const bool tls_socket_valid =.*?;\s*", "", body, flags=re.M | re.S)
    body = re.sub(r"^\s*const uint8_t close_flags =.*?;\s*", "", body, flags=re.M | re.S)
    body = re.sub(r"^\s*const uint8_t flags =.*?;\s*", "", body, flags=re.M | re.S)
    body = body.replace("const int destroy_result = esp_tls_conn_destroy(ssl->tls);",
                        "esp_tls_conn_destroy(ssl->tls);")
    body = re.sub(r"exit_failure:\s*\{", "exit_failure:", body)
    body = body.replace("return -1;\n        }\n}", "return -1;\n}")
    return re.sub(r"\s+", " ", body).strip()

baseline_close = function_body(baseline_sockets, "lwip_close(int s)")
candidate_close = function_body(candidate_sockets, "lwip_close(int s)")
candidate_close = re.sub(
    r"\s*lwip_close_diag_socket_event\(.*?\);", "", candidate_close, flags=re.S
)
assert re.sub(r"\s+", " ", candidate_close) == re.sub(r"\s+", " ", baseline_close), \
    "lwip_close behavior changed beyond diagnostic calls"
assert "netconn_prepare_delete(sock->conn)" in candidate_close
assert "free_socket(sock, is_tcp)" in candidate_close

base_close = function_body(transport, "static int base_close(")
baseline_base_close = function_body(baseline_transport, "static int base_close(")
assert base_close.count("esp_tls_conn_destroy(ssl->tls)") == 1
assert base_close.count("close(ssl->sockfd)") == 1
assert "LWIP_CLOSE_DIAG_NO_CLOSE_ATTEMPT" in base_close
assert "LWIP_CLOSE_DIAG_TRANSPORT_CLOSE_END" in base_close
assert "LWIP_CLOSE_DIAG_RELEASE_CONFIRMED" not in base_close, \
    "wrapper result must not be called proof of socket release"
assert "ssl->ssl_initialized" in baseline_base_close

assert strip_diagnostic_hooks(base_close) == strip_diagnostic_hooks(baseline_base_close), \
    "base_close control flow changed beyond diagnostics"

ssl_connect = function_body(transport, "static int ssl_connect(")
baseline_ssl_connect = function_body(baseline_transport, "static int ssl_connect(")
scope_begin = ssl_connect.index("lwip_close_diag_scope_begin()")
tls_connect = ssl_connect.index("esp_tls_conn_new_sync(")
assert scope_begin < tls_connect, "TLS connect scope must cover ESP-TLS's internal socket close"
assert ssl_connect.count("lwip_close_diag_scope_end(connect_correlation_id)") == 2, \
    "success and failure paths must both end the connection scope"
assert "LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_BEGIN" in ssl_connect
assert "LWIP_CLOSE_DIAG_TLS_CONNECT_ATTEMPT_END" in ssl_connect
assert "const uint32_t close_correlation_id = lwip_close_diag_scope_begin()" not in ssl_connect, \
    "failure cleanup must use the enclosing TLS-attempt scope"
assert "correlation_id" in (component / "include" / "lwip" / "close_diag.h").read_text()
assert strip_diagnostic_hooks(ssl_connect) == strip_diagnostic_hooks(baseline_ssl_connect), \
    "ssl_connect control flow changed beyond diagnostics"

with tempfile.TemporaryDirectory(prefix="p4n9-close-diag-") as temp:
    binary = pathlib.Path(temp) / "close_diag_test"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-pthread",
        "-I", str(component / "test_host" / "include"),
        "-I", str(component / "include"),
        str(diag_source), str(test_source), "-o", str(binary),
    ], check=True)
    output = subprocess.check_output([str(binary)], text=True)
    undefined = subprocess.check_output(["nm", "-u", str(binary)], text=True)
    assert " malloc" not in undefined and " calloc" not in undefined
    assert " realloc" not in undefined and " free" not in undefined
    print(output, end="")

print("PRODUCTION_LWIP_CLOSE_CONTROL_FLOW_UNCHANGED=PASS")
print("TRANSPORT_WRAPPER_RESULT_NOT_RELEASE_PROOF=PASS")

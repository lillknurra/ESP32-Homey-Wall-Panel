#!/usr/bin/env python3
"""Guard Patch064's bounded mbedTLS large-allocation placement policy."""
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parents[3]
DEFAULTS = (ROOT / "sdkconfig.defaults").read_text(encoding="utf-8")
IDF_ROOT = Path(os.environ.get("IDF_PATH", "/Users/petter/GitHub/esp-idf-v6.0.1"))
IDF_KCONFIG = (IDF_ROOT / "components/mbedtls/Kconfig").read_text(encoding="utf-8")
IDF_MEM = (IDF_ROOT / "components/mbedtls/port/esp_mem.c").read_text(encoding="utf-8")
IDF_HEAP = (IDF_ROOT / "components/heap/heap_caps.c").read_text(encoding="utf-8")
CLIENT = (ROOT / "components/secure_bootstrap/athom_cloud_client.c").read_text(encoding="utf-8")
RUNTIME = (ROOT / "components/secure_bootstrap/athom_oauth_runtime.c").read_text(encoding="utf-8")

assert "CONFIG_MBEDTLS_DEFAULT_MEM_ALLOC=y" in DEFAULTS
assert "CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC=y" not in DEFAULTS
assert "CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC=y" not in DEFAULTS
assert "CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384" in DEFAULTS

assert 'config MBEDTLS_DEFAULT_MEM_ALLOC' in IDF_KCONFIG
assert 'default MBEDTLS_INTERNAL_MEM_ALLOC' in IDF_KCONFIG
assert "return calloc(n, size);" in IDF_MEM
assert "if (size <= (size_t)malloc_alwaysinternal_limit)" in IDF_HEAP
assert "heap_caps_malloc_base( size, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM )" in IDF_HEAP

# Patch063's same-attempt diagnostics remain available for the next runtime gate.
for marker in (
    "PATCH063_TLS_MEMORY_CAPTURE_BEGIN",
    "patch019a16e_begin_transport_alloc_capture();",
    "patch019a16e_finish_transport_alloc_capture();",
    "last_tls_memory_diagnostic",
):
    assert marker in CLIENT
assert "raw_tls_memory_diagnostic" in RUNTIME
assert "tls_memory" in RUNTIME

# The allocator policy changes no request role, route, or Homey write operation.
assert "HTTP_ROLE_CLOUD" in CLIENT and "HTTP_ROLE_HOMEY_REMOTE" in CLIENT
assert "http_request_limited(" in CLIENT
print("PATCH064_TLS_ALLOCATOR_POLICY PASS")

#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile

component = pathlib.Path(__file__).resolve().parents[1]
source = component / "runtime_event_ring.c"
test = component / "test_host" / "test_runtime_event_ring.c"
with tempfile.TemporaryDirectory(prefix="runtime-event-ring-") as temp:
    obj = pathlib.Path(temp) / "runtime_event_ring.o"
    binary = pathlib.Path(temp) / "test_runtime_event_ring"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread",
                    "-DRUNTIME_EVENT_RING_HOST_TEST", "-I", str(component / "include"),
                    "-c", str(source), "-o", str(obj)], check=True)
    undefined = subprocess.check_output(["nm", "-u", str(obj)], text=True)
    forbidden = ("malloc", "calloc", "realloc", "free")
    assert not any(symbol in undefined for symbol in forbidden), undefined
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pthread",
                    "-DRUNTIME_EVENT_RING_HOST_TEST", "-I", str(component / "include"),
                    str(source), str(test), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

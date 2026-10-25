#!/usr/bin/env python3
"""Verify production per-widget arm logic and default-deny behavior."""
from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
header = (component / "include/athom_cloud_client.h").read_text(encoding="utf-8")
source = (component / "athom_cloud_client.c").read_text(encoding="utf-8")
macro_start = header.index("/* Solskydd (awning_2) and Balkongmarkis (awning_3)")
macro_end = header.index("#endif", header.index("#if (PATCH085_AWNING_3_WRITE_ARMED", macro_start)) + len("#endif")
macros = header[macro_start:macro_end]
signature = "bool athom_cloud_awning_write_is_armed(size_t widget_index)"
start = source.index(signature)
brace = source.index("{", start)
depth = 0
end = None
for index in range(brace, len(source)):
    if source[index] == "{":
        depth += 1
    elif source[index] == "}":
        depth -= 1
        if depth == 0:
            end = index + 1
            break
if end is None:
    raise SystemExit("production awning arm function is incomplete")
production = source[start:end]

test = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
int main(void)
{
    assert(athom_cloud_awning_write_is_armed(3U) == false);
    assert(athom_cloud_awning_write_is_armed(0U) == false);
    assert(athom_cloud_awning_write_is_armed(1U) == (PATCH085_AWNING_2_WRITE_ARMED == 1));
    assert(athom_cloud_awning_write_is_armed(2U) == (PATCH085_AWNING_3_WRITE_ARMED == 1));
    return 0;
}
'''

cases = [
    ("default-enabled-both", []),
    ("explicit-disarmed-both", ["-DPATCH085_AWNING_2_WRITE_ARMED=0",
                                 "-DPATCH085_AWNING_3_WRITE_ARMED=0"]),
    ("awning-2-only", ["-DPATCH085_AWNING_2_WRITE_ARMED=1",
                       "-DPATCH085_AWNING_3_WRITE_ARMED=0"]),
    ("awning-3-only", ["-DPATCH085_AWNING_2_WRITE_ARMED=0",
                       "-DPATCH085_AWNING_3_WRITE_ARMED=1"]),
    ("both-explicit", ["-DPATCH085_AWNING_2_WRITE_ARMED=1",
                        "-DPATCH085_AWNING_3_WRITE_ARMED=1"]),
]
with tempfile.TemporaryDirectory() as temporary:
    temp = Path(temporary)
    for name, flags in cases:
        cfile = temp / f"{name}.c"
        binary = temp / name
        cfile.write_text("#include <stdbool.h>\n#include <stddef.h>\n" +
                          macros + "\n" + production + "\n" + test, encoding="utf-8")
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", *flags,
                        str(cfile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print("PATCH085_AWNING_ARM_MATRIX PASS (default on; explicit disarm; independent widget gates)")

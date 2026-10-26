#!/usr/bin/env python3
"""Patch116I host tests. No external requests, firmware writes or Git operations."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'components/panel_update_service'
TEST = ROOT / 'tests/patch116i'


def cjson_source():
    managed = ROOT / 'managed_components/espressif__cjson'
    if managed.exists():
        sources = list(managed.rglob('cJSON.c'))
        headers = list(managed.rglob('cJSON.h'))
        if len(sources) == 1 and len(headers) == 1:
            return [str(sources[0])], ['-I' + str(headers[0].parent), '-lm']
    # Local fallback only; Linux development image has system libcjson.so.1.
    if sys.platform == 'linux' and Path('/lib/x86_64-linux-gnu/libcjson.so.1').exists():
        return [], ['-I' + str(TEST/'shim'), '-l:libcjson.so.1', '-lm']
    raise RuntimeError('Pinned managed cJSON source not found. Run an ESP-IDF build to resolve espressif__cjson.')


def main():
    sources, dep_flags = cjson_source()
    with tempfile.TemporaryDirectory(prefix='p116i-host-') as temporary:
        exe = Path(temporary) / 'test_manifest'
        for cc in ('gcc', 'clang'):
            compiled_sources = []
            if sources:
                # Keep the dependency's legacy sprintf warnings out of the
                # project/test -Werror build while still compiling the pinned
                # cJSON source with sanitizers.
                for index, source in enumerate(sources):
                    obj = Path(temporary) / f'{cc}_dependency_{index}.o'
                    subprocess.run([
                        cc, '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-Wno-deprecated-declarations', '-fsanitize=address,undefined',
                        '-fno-omit-frame-pointer', *dep_flags[:1], '-c', source, '-o', str(obj),
                    ], check=True)
                    compiled_sources.append(str(obj))
            cmd = [cc, '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                   '-I' + str(SRC / 'include'),
                   str(SRC / 'panel_update_manifest.c'),
                   str(SRC / 'panel_update_manifest_json.c'),
                   str(SRC / 'panel_update_check.c'),
                   str(TEST / 'test_manifest.c'), *compiled_sources, *dep_flags,
                   '-o', str(exe)]
            print('COMPILER=' + cc, flush=True)
            subprocess.run(cmd, check=True)
            # Apple's bundled sanitizer runtime does not implement LeakSanitizer.
            # AddressSanitizer and UBSan remain enabled on macOS.
            detect_leaks = '0' if sys.platform == 'darwin' else '1'
            env = dict(os.environ, ASAN_OPTIONS=f'detect_leaks={detect_leaks}:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1')
            subprocess.run([str(exe)], check=True, env=env)
        subprocess.run([sys.executable, str(TEST / 'test_manifest_publisher.py'), str(exe)], check=True)
    print('PATCH116I_OFFLINE_TESTS=PASS')


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Alias of original test entrypoint, updated for Patch116F transport."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name("run_patch116e_https_stage_tests.py")), run_name="__main__")

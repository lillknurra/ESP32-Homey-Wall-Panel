#!/usr/bin/env python3
"""Focused regression for Patch102's hash-pinned BSP PCLK override."""

from __future__ import annotations

import hashlib
from pathlib import Path
import tempfile
import unittest

import override_waveshare_rgb_pclk12 as patch


class Patch102BspPclkOverrideTests(unittest.TestCase):
    def test_locked_source_gets_only_the_pclk_assignment(self):
        repo = Path(__file__).resolve().parents[1]
        source = repo / "managed_components/waveshare__esp32_s3_touch_lcd_4b/esp32_s3_touch_lcd_4b.c"
        if not source.is_file():
            self.skipTest("locked managed component is not materialized yet")
        original_bytes = source.read_bytes()
        original = original_bytes.decode("utf-8")
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "esp32_s3_touch_lcd_4b.c"
            patch.generate(source, output)
            generated = output.read_text(encoding="utf-8")
        self.assertEqual(generated, original.replace(patch.ANCHOR, patch.OVERRIDE + patch.ANCHOR, 1))
        self.assertEqual(generated.count("rgb_config.timings.pclk_hz = 12 * 1000 * 1000;"), 1)
        self.assertEqual(generated.count("rgb_config.timings.h_res = BSP_LCD_H_RES;"), 1)
        self.assertEqual(generated.count("rgb_config.timings.v_res = BSP_LCD_V_RES;"), 1)
        self.assertEqual(hashlib.sha256(source.read_bytes()).digest(), hashlib.sha256(original_bytes).digest())

    def test_modified_vendor_translation_unit_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "vendor.c"
            output = Path(tmp) / "generated.c"
            source.write_text("not the pinned Waveshare translation unit", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                patch.generate(source, output)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()

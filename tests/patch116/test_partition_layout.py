"""Patch116 host-only tests: protect every existing partition and reject bad OTA layouts."""

from importlib import util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = util.spec_from_file_location(
    "patch116_partition_check", ROOT / "scripts/validate_patch116_ota_partitions.py"
)
assert SPEC and SPEC.loader
import sys
module = util.module_from_spec(SPEC)
sys.modules[SPEC.name] = module
SPEC.loader.exec_module(module)

BASE = (ROOT / "partitions.csv").read_text(encoding="utf-8")
CANDIDATE = (ROOT / "partitions_ota_candidate.csv").read_text(encoding="utf-8")

class PartitionLayoutTest(unittest.TestCase):
    def assert_rejected(self, candidate: str) -> None:
        with self.assertRaises(module.PartitionError):
            module.validate(BASE, candidate)

    def test_valid_layout(self):
        result = module.validate(BASE, CANDIDATE)
        self.assertEqual(result["result"], "PASS_STATIC_PARTITION_LAYOUT_ONLY")
        self.assertEqual(result["ota_slot_bytes"], 0x400000)
        self.assertEqual(result["tail_free_bytes"], 0xE0000)
        self.assertEqual(set(result["existing_partitions_preserved"]),
                         {"nvs", "phy_init", "factory", "diaglog"})

    def test_installed_image_size_fits(self):
        self.assertEqual(module.validate(BASE, CANDIDATE, (1749184,))
                         ["ota_slot_bytes"], 0x400000)

    def test_oversized_image_rejected(self):
        with self.assertRaises(module.PartitionError):
            module.validate(BASE, CANDIDATE, (0x400001,))

    def test_zero_image_rejected(self):
        with self.assertRaises(module.PartitionError):
            module.validate(BASE, CANDIDATE, (0,))

    def test_moved_nvs_rejected(self):
        self.assert_rejected(CANDIDATE.replace("nvs,0x9000", "nvs,0xA000"))

    def test_shrunk_factory_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0x10000,0x600000", "0x10000,0x500000"))

    def test_moved_diaglog_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0x610000,0x100000", "0x620000,0x100000"))

    def test_shrunk_diaglog_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0x610000,0x100000", "0x610000,0x80000"))

    def test_missing_ota_slot_rejected(self):
        self.assert_rejected(CANDIDATE.replace("ota_1,app,ota_1,0xB20000,0x400000,", ""))

    def test_extra_partition_rejected(self):
        self.assert_rejected(CANDIDATE + "hidden,data,undefined,0xF20000,0x1000,\n")

    def test_moved_otadata_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0x710000,0x2000", "0x711000,0x2000"))

    def test_unaligned_app_rejected(self):
        self.assert_rejected(CANDIDATE.replace("ota_0,app,ota_0,0x720000", "ota_0,app,ota_0,0x721000"))

    def test_overlap_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0xB20000,0x400000", "0xA20000,0x400000"))

    def test_beyond_flash_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0xB20000,0x400000", "0xD20000,0x400000"))

    def test_duplicate_partition_rejected(self):
        self.assert_rejected(CANDIDATE + "ota_0,app,ota_0,0xF20000,0x10000,\n")

    def test_invalid_numeric_value_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0xB20000", "not_hex"))

    def test_unexpected_flags_rejected(self):
        self.assert_rejected(CANDIDATE.replace("0x400000,\n", "0x400000,encrypted\n", 1))

    def test_old_layout_not_matching_expected_rejected(self):
        with self.assertRaises(module.PartitionError):
            module.validate(BASE.replace("0x9000", "0xA000"), CANDIDATE)

    def test_partition_table_overlap_rejected(self):
        self.assert_rejected(CANDIDATE.replace("nvs,data,nvs,0x9000", "nvs,data,nvs,0x8000"))

    def test_truncated_name_error(self):
        self.assert_rejected(CANDIDATE + "this_name_is_over_16,data,undefined,0xF20000,0x1000,\n")

if __name__ == "__main__":
    unittest.main()

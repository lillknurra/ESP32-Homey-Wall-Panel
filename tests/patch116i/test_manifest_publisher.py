#!/usr/bin/env python3
"""Offline Python manifest publisher tests; also cross-checks its output with C JSON decoder."""
import json
import struct
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/ota'))
from panel_release_manifest import create, load_validate, validate, _url, MODEL

C_DECODER = sys.argv[1] if len(sys.argv) > 1 else None


class PublisherTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='p116i-publisher-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.image = self.root / 'fake_signed_image_FOR_TEST_ONLY.bin'
        self.image.write_bytes(self.esp_image('1.2.0'))
        self.public_key = self.root / 'trusted-public.pem'
        self.public_key.write_text('TEST PUBLIC KEY FIXTURE; never used for cryptography\n')
        self.verifier = self.root / 'espsecure-test'
        self.verifier.write_text('#!/bin/sh\nexit 0\n')
        self.verifier.chmod(0o755)
        self.data = json.loads(self.make('stable', '1.2.0', 1002000, 'Fixes').decode())

    @staticmethod
    def esp_image(version, project='esp32_homey_wall_panel', chip_id=9):
        header = bytearray(24)
        header[0] = 0xE9
        header[1] = 1
        header[12:14] = struct.pack('<H', chip_id)
        segment = bytearray(8)
        struct.pack_into('<I', segment, 4, 256)
        desc = bytearray(256)
        struct.pack_into('<II', desc, 0, 0xABCD5432, 0)
        desc[16:48] = version.encode('ascii') + b'\0' * (32 - len(version))
        desc[48:80] = project.encode('ascii') + b'\0' * (32 - len(project))
        return bytes(header + segment + desc) + b'\x00' * (1024 - 24 - 8 - 256)

    def make(self, channel, version, sequence, notes):
        return create(self.image, MODEL, channel, version, sequence, notes,
                      self.public_key, str(self.verifier))

    def test_generate_and_validate(self):
        raw = self.make('stable', '1.2.0', 1002000, 'Förbättrad Wi-Fi')
        x = json.loads(raw)
        self.assertEqual(x['image']['size_bytes'], 1024)
        self.assertEqual(x['release_sequence'], 1002000)
        self.assertEqual(len(x['image']['sha256']), 64)
        self.assertTrue(x['image']['url'].startswith(
            'https://lillknurra.github.io/ESP32-Homey-Wall-Panel/updates/releases/'))
        self.assertIn('Förbättrad', raw.decode())
        path = self.root / 'manifest.json'
        path.write_bytes(raw)
        self.assertEqual(load_validate(path), x)
        if C_DECODER:
            p = subprocess.run([C_DECODER, str(path)], capture_output=True, text=True)
            self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
            self.assertIn('VALID_MANIFEST version=1.2.0', p.stdout)

    def test_beta_release_is_separate(self):
        self.image.write_bytes(self.esp_image('1.3.0'))
        raw = self.make('beta', '1.3.0', 1003000, 'Beta')
        obj = json.loads(raw)
        self.assertIn('/updates/releases/1.3.0/', obj['image']['url'])
        self.assertEqual(obj['channel'], 'beta')
        if C_DECODER:
            p = self.root / 'beta.json'
            p.write_bytes(raw)
            res = subprocess.run([C_DECODER, str(p)], capture_output=True, text=True)
            self.assertEqual(res.returncode, 0, res.stderr)

    def test_invalid_image_sizes(self):
        self.image.write_bytes(b'abc')
        with self.assertRaisesRegex(ValueError, 'image size'):
            self.make('stable', '1.2.0', 1002000, '')
        self.image.write_bytes(b'X' * (0x400000 + 1))
        with self.assertRaisesRegex(ValueError, 'exceeds'):
            self.make('stable', '1.2.0', 1002000, '')

    def test_invalid_schema(self):
        for field, value in [('schema_version', 2), ('release_sequence', 0),
                             ('release_sequence', True), ('version', '1.2.x'),
                             ('release_sequence', 1002001),
                             ('version', '01.2.3'), ('channel', 'nightly'),
                             ('hardware_model', 'not allowed/'), ('notes', 'a\nline'),
                             ('notes', 'ö' * 150), ('notes', '\ud800')]:
            x = dict(self.data)
            x[field] = value
            with self.subTest(field=field, value=value):
                with self.assertRaises(ValueError):
                    validate(x)

    def test_bad_image_fields(self):
        for field, val in [('size_bytes', -1), ('size_bytes', 4194305),
                           ('size_bytes', True), ('sha256', '0' * 64),
                           ('sha256', 'A' * 64), ('url', 'http://github.com/')]:
            x = {**self.data, 'image': {**self.data['image'], field: val}}
            with self.subTest(field=field):
                with self.assertRaises(ValueError):
                    validate(x)

    def test_duplicate_keys_rejected(self):
        invalid = b'{"schema_version":1,"schema_version":1}'
        path = self.root / 'dup.json'
        path.write_bytes(invalid)
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            load_validate(path)

    def test_unknown_fields_rejected(self):
        x = {**self.data, 'command': 'run'}
        with self.assertRaises(ValueError):
            validate(x)
        x = {**self.data, 'image': {**self.data['image'], 'install': 'true'}}
        with self.assertRaises(ValueError):
            validate(x)

    def test_exact_release_url(self):
        expected = _url(MODEL, '1.2.0')
        self.assertEqual(self.data['image']['url'], expected)
        for replacement in [expected + '?redirect=1', expected.replace('github.io', 'github.evil'),
                            expected.replace('/updates/releases/1.2.0/', '/updates/releases/1.2.1/'),
                            expected.replace('_signed.bin', '_unsigned.bin')]:
            x = {**self.data, 'image': {**self.data['image'], 'url': replacement}}
            with self.assertRaises(ValueError):
                validate(x)

    def test_signed_image_release_identity(self):
        with self.assertRaisesRegex(ValueError, 'sequence'):
            self.make('stable', '1.2.0', 1002001, 'Mismatch')
        self.image.write_bytes(self.esp_image('1.2.1'))
        with self.assertRaisesRegex(ValueError, 'descriptor'):
            self.make('stable', '1.2.0', 1002000, 'Version mismatch')
        self.image.write_bytes(self.esp_image('1.2.0', project='other_project'))
        with self.assertRaisesRegex(ValueError, 'descriptor'):
            self.make('stable', '1.2.0', 1002000, 'Project mismatch')
        self.image.write_bytes(self.esp_image('1.2.0', chip_id=5))
        with self.assertRaisesRegex(ValueError, 'ESP32-S3'):
            self.make('stable', '1.2.0', 1002000, 'Wrong target')

    def test_create_cli_requires_native_signature_verification(self):
        output = self.root / 'output.json'
        verifier = self.root / 'espsecure-test'
        public_key = self.root / 'trusted-public.pem'
        public_key.write_text('TEST PUBLIC KEY FIXTURE; never used for cryptography\n')
        verifier.write_text('#!/bin/sh\nexit 0\n')
        verifier.chmod(0o755)
        cmd = [sys.executable, str(ROOT/'tools/ota/panel_release_manifest.py'),
               'create', '--signed-image', str(self.image), '--version', '1.2.0',
               '--release-sequence', '1002000', '--notes', 'Fixes', '--out', str(output),
               '--public-key', str(public_key), '--espsecure', str(verifier)]
        no = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(no.returncode, 0, no.stderr)
        self.assertTrue(output.exists())
        output.unlink()
        missing_key = subprocess.run(cmd[:-4], capture_output=True, text=True)
        self.assertNotEqual(missing_key.returncode, 0)
        self.assertFalse(output.exists())
        failing_verifier = self.root / 'espsecure-fail-test'
        failing_verifier.write_text('#!/bin/sh\nexit 1\n')
        failing_verifier.chmod(0o755)
        failed = subprocess.run(cmd[:-1] + [str(failing_verifier)],
                                capture_output=True, text=True)
        self.assertNotEqual(failed.returncode, 0)
        self.assertFalse(output.exists())
        ok = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(ok.returncode, 0, ok.stderr)
        self.assertTrue(output.exists())
        second = subprocess.run(cmd, capture_output=True, text=True)
        self.assertNotEqual(second.returncode, 0)
        check = subprocess.run([sys.executable, str(ROOT/'tools/ota/panel_release_manifest.py'),
                                'validate', str(output)], capture_output=True, text=True)
        self.assertEqual(check.returncode, 0)


if __name__ == '__main__':
    print('PATCH116I_PYTHON_PUBLISHER_TESTS')
    unittest.main(argv=[sys.argv[0]], verbosity=1)

#!/usr/bin/env python3
"""Package guard tests using synthetic format fixtures, never runnable firmware demos."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fm1pkg_make', ROOT/'platform/fm1-base/tools/fm1pkg_make.py')
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)

class PackageGuard(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        # Deliberately synthetic package-resource bytes: only parser/CRC tests, not SDK validation.
        for rel in ('uboot.boot', 'cfg_tool.bin', 'cfg/eq_cfg_hw.bin'):
            path = self.root/'sdk/cpu/wl82/tools'/rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'unit test resource')
        pack.SDK = self.root/'sdk'
        self.app = self.root/'app.bin'
        self.app.write_bytes(b'unit fixture BLACKWATER\0')
        self.pkg = self.root/'test.fwsc'
        self.make()

    def make(self, identity='FM-1_4257400'):
        flash = pack.flash_image(self.app.read_bytes(), pack.KEY)
        self.pkg.write_bytes(pack.ufw(flash, b'unit loader', identity))

    def run_guard(self):
        return subprocess.run(['node', str(ROOT/'tools/package_metadata.mjs'), str(self.pkg), str(self.app)],
                              capture_output=True, text=True)

    def test_matching_binary_and_crc(self):
        result = self.run_guard()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['app_bytes'], self.app.stat().st_size)

    def test_mismatched_build_is_rejected(self):
        self.app.write_bytes(b'another BLACKWATER build')
        self.assertNotEqual(self.run_guard().returncode, 0)

    def test_unrelated_application_is_rejected(self):
        self.app.write_bytes(b'unrelated application')
        self.make()
        self.assertNotEqual(self.run_guard().returncode, 0)

    def test_template_identity_is_rejected(self):
        self.make('FM-1_1100')
        self.assertNotEqual(self.run_guard().returncode, 0)

    def test_corrupt_package_is_rejected(self):
        data = bytearray(self.pkg.read_bytes())
        data[-1] ^= 1
        self.pkg.write_bytes(data)
        self.assertNotEqual(self.run_guard().returncode, 0)

if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
"""Landing content and installer URL contracts; synthetic packages are parser fixtures, never emulated firmware."""
import importlib.util
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from site_content import copy_site_assets,installer_pages,render_landing

class Site(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.out=Path(self.temp.name)

    def test_current_media_and_landing(self):
        metadata={'emulator_path':'emulator-abc123/','package_path':'firmware/blackwater-abc123.fwsc',
                  'source_commit':'a'*40,'package_sha256':'b'*64}
        page=render_landing(ROOT,metadata,'0.4',('blackwater-0.4.fwsc','FM-1_77000400'))
        self.assertNotRegex(page,r'\{\{[A-Z_]+\}\}')
        self.assertIn('href="install/"',page)
        self.assertIn('emulator-abc123/?fw=blackwater&amp;mhz=240&amp;embed=1',page)
        self.assertIn('firmware/blackwater-abc123.fwsc',page)
        self.assertIn('FREE PLAY',page)
        self.assertNotIn('pixel-art river',page)
        copy_site_assets(ROOT,self.out)
        for photo in ('river','growth','night','strike','free-play','fish-guide'):
            with Image.open(self.out/'assets/screens'/f'{photo}.png') as im:
                self.assertEqual(im.size,(240,240))
        self.assertGreater((self.out/'assets/current.mp4').stat().st_size,10000)
        provenance=json.loads((ROOT/'site/media.json').read_text())
        self.assertIn('host',provenance['capture'])

    def test_preview_is_not_a_fake_release(self):
        page=render_landing(ROOT,{'source_commit':'<unsafe>'})
        self.assertIn('Local website preview',page)
        self.assertNotIn('<iframe',page)
        self.assertNotIn('href="install/"',page)
        self.assertIn('&lt;unsafe',page)

    def test_verified_development_package_keeps_its_emulator(self):
        page=render_landing(ROOT,{'emulator_path':'emulator-dev/','package_path':'firmware/dev.fwsc'})
        self.assertIn('<iframe',page)
        self.assertNotIn('href="install/"',page)
        self.assertNotIn('Local website preview',page)

    def test_installer_fetches_content_addressed_release(self):
        spec=importlib.util.spec_from_file_location('fm1pkg_make',ROOT/'platform/fm1-base/tools/fm1pkg_make.py')
        pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
        sdk=self.out/'sdk'
        for rel in ('uboot.boot','cfg_tool.bin','cfg/eq_cfg_hw.bin'):
            path=sdk/'cpu/wl82/tools'/rel;path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(b'unit fixture resource')
        pack.SDK=sdk
        raw=pack.ufw(pack.flash_image(b'unit fixture BLACKWATER\0',pack.KEY),b'unit loader','FM-1_77000400')
        package=self.out/'fixture.fwsc';package.write_bytes(raw)
        (self.out/'firmware').mkdir()
        name,identity=installer_pages(ROOT,self.out,package,'0.4')
        self.assertEqual((name,identity),('blackwater-0.4.fwsc','FM-1_77000400'))
        page=(self.out/'install/index.html').read_text()
        hashed='blackwater-'+hashlib.sha256(raw).hexdigest()[:10]+'.fwsc'
        self.assertIn('"pkg": "../firmware/'+hashed+'"',page)
        self.assertNotIn('"pkg": "../firmware/blackwater-0.4.fwsc"',page)
        self.assertEqual((self.out/'firmware'/name).read_bytes(),raw)
        self.assertEqual((self.out/'firmware'/hashed).read_bytes(),raw)
        self.assertTrue((self.out/'backup/index.html').is_file())

if __name__=='__main__': unittest.main()

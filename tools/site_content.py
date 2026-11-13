#!/usr/bin/env python3
"""BLACKWATER landing content and the platform's installer; no firmware/emulation gate bypass."""
import hashlib
import html
import json
from pathlib import Path
import re
import shutil


def copy_site_assets(root, out):
    assets=out/'assets'
    assets.mkdir()
    for name in ('style.css','icon.svg','current.mp4'):
        shutil.copy2(root/'site'/name,assets/name)
    shutil.copytree(root/'site/screens',assets/'screens')


def render_landing(root, metadata, version=None, installed=None):
    """A preview is explicitly labelled and has no working installer or substitute emulator."""
    esc=html.escape
    emulator=esc(metadata.get('emulator_path',''))
    release=bool(installed)
    instrument=bool(metadata.get('emulator_path')) and not metadata.get('preview_only',False)
    action=('<a class="button primary" href="install/">Install on your FM-1 <span aria-hidden="true">→</span></a>'
            if release else '<span class="button disabled" aria-disabled="true">Installer requires a release build</span>')
    player=('<div class="player-frame"><iframe src="'+emulator+'?fw=blackwater&amp;mhz=240&amp;embed=1" '
            'title="BLACKWATER firmware in fm1-emu" loading="lazy" allow="autoplay"></iframe></div>') if instrument else (
            '<div class="player-frame player-preview"><img src="assets/screens/free-play.png" alt="Actual firmware screenshot" '
            'width="240" height="240"><p>The verified firmware emulator appears here in a release build.</p></div>')
    values={
        'PREVIEW_NOTICE':'' if instrument else '<div class="preview-notice">Local website preview — installer and emulator are not included.</div>',
        'INSTALL_NAV':'<a class="nav-install" href="install/">Install →</a>' if release else '',
        'INSTALL_ACTION':action,
        'VERSION_LABEL':esc('v'+version+' / River edition' if version else 'Development build' if instrument else 'Local preview'),
        'PLAYER':player,
        'EMULATOR_LINK':('<a href="'+emulator+'?fw=blackwater&amp;mhz=240">Open the full emulator →</a>') if instrument else '',
        'PACKAGE_LINK':('<a href="'+esc(metadata['package_path'])+'">Download this firmware package →</a>') if instrument else '',
        'SOURCE_SHORT':esc(metadata.get('source_commit','preview')[:7]),
        'PACKAGE_HASH':esc(metadata.get('package_sha256','Not built in this local preview.')),
    }
    template=(root/'site/index.html').read_text()
    return re.sub(r'\{\{([A-Z_]+)\}\}',lambda match:values[match[1]],template)


def installer_pages(root, out, package, version):
    """fm1-base's own web installer (Chrome or Edge, Web MIDI), manual page and backup page, as its make_site.py
    makes them, for a release package: install/, manual/, backup/, firmware/blackwater-VERSION.fwsc"""
    import re
    import sys as _sys
    _sys.path.insert(0, str(root/'platform/fm1-base/tools'))
    import make_site as ms
    app = root/'firmware'
    conf = ms.read_conf(app/'fm1.toml')
    raw = package.read_bytes()
    product = ms.product_of(raw)
    if not re.fullmatch(re.escape(conf.get('id_prefix', 'FM-1_0')) + r'\d{6}', product):
        raise SystemExit(f'{package}: identity {product}: the installer needs a release build (./build.sh --release X.Y)')
    name = f"blackwater-{version}.fwsc"
    shutil.copy2(package, out/'firmware'/name)
    hashed_name='blackwater-'+hashlib.sha256(raw).hexdigest()[:10]+'.fwsc'
    shutil.copy2(package,out/'firmware'/hashed_name)
    boot = ms.SRC/'web'/'m-bootloader'/f'fm1-uboot-{ms.MBOOT_VERSION}.fwsc'
    boot_name = f'm-bootloader-{ms.MBOOT_VERSION}.fwsc'
    shutil.copy2(boot, out/'firmware'/boot_name)
    shutil.copy2(ms.SRC/'web'/'m-bootloader'/'LICENSE', out/'firmware'/'m-bootloader-LICENSE.txt')
    for lic in sorted((ms.SRC/'LICENSES').glob('*.txt')):
        shutil.copy2(lic, out/'firmware'/lic.name)
    page_html = (ms.SRC/'web'/'installer.html').read_text(encoding='utf-8')
    lib = '\n'.join(ms.strip_module((ms.SRC/'web'/f).read_text(encoding='utf-8'))
                    for f in ('fm1ports.js', 'fm1pkg.js', 'fm1ota.js'))
    meta = json.dumps({'version': version, 'product': product, 'pkg': '../firmware/' + hashed_name,
                       'backup': bool(conf.get('backup')), 'res': None,
                       'boot': {'pkg': '../firmware/' + boot_name, 'version': ms.MBOOT_VERSION,
                                'sha256': hashlib.sha256(boot.read_bytes()).hexdigest()}})
    repo, credits = conf.get('repo', ''), conf.get('credits', '')
    for key, val in (('RES', "BLACKWATER's data"), ('RES_JA', 'BLACKWATER のデータ'), ('NAME', conf['name']),
                     ('REPO', repo), ('REPO_SHORT', re.sub(r'^https?://', '', repo)),
                     ('LICENSE', conf.get('licence', 'GPL-3.0-or-later')), ('CREDITS', ' ' + credits if credits else '')):
        page_html = page_html.replace('{{' + key + '}}', val)
    (out/'install').mkdir()
    (out/'install'/'index.html').write_text(page_html.replace('/*LIB*/', lib).replace('/*META*/', meta)
                                            .replace('/*APP*/', 'null'), encoding='utf-8')
    (out/'manual').mkdir()
    ms.page(root/'docs'/'MANUAL.md', out/'manual'/'index.html', conf, False)
    for path, html_text in ms.tool_pages(app, conf).items():
        (out/path).parent.mkdir(parents=True, exist_ok=True)
        (out/path).write_text(html_text, encoding='utf-8')
    (out/'.nojekyll').write_text('')
    return name, product

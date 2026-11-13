#!/usr/bin/env python3
"""Make an explicitly labelled landing-only preview. Does not create a package, installer or emulator."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
from site_content import copy_site_assets,render_landing

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',type=Path,default=ROOT/'build/site-preview')
a=p.parse_args();out=a.out.resolve()
if out.exists() and any(out.iterdir()):p.error('choose a new, empty output directory')
out.mkdir(parents=True,exist_ok=True)
copy_site_assets(ROOT,out)
metadata={'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
          'preview_only':True,'physical_hardware_tested':False}
(out/'index.html').write_text(render_landing(ROOT,metadata))
(out/'build.json').write_text(json.dumps(metadata,indent=2)+'\n')
sys.path.insert(0,str(ROOT/'platform/fm1-base/tools'))
import make_site as ms
conf=ms.read_conf(ROOT/'firmware/fm1.toml')
(out/'manual').mkdir();ms.page(ROOT/'docs/MANUAL.md',out/'manual/index.html',conf,False)
for path,content in ms.tool_pages(ROOT/'firmware',conf).items():
    target=out/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(content)
(out/'licenses').mkdir();shutil.copy2(ROOT/'THIRD_PARTY.md',out/'licenses/THIRD_PARTY.md')
print(out)

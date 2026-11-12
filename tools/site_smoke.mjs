// SPDX-License-Identifier: GPL-3.0-or-later
// Browser checks for the real landing content; --preview explicitly excludes package/emulator validation.
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {existsSync,readFileSync,mkdirSync} from 'node:fs';
import {extname,join,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createRequire} from 'node:module';

export async function checkLanding(page,base,out,{preview=false}={}) {
    const errors=[];
    const onError=e=>errors.push(String(e));page.on('pageerror',onError);
    await page.goto(base);
    assert.equal(await page.locator('h1').count(),1);
    assert.match(await page.locator('h1').innerText(),/A river\s+you can play/);
    await page.waitForFunction(()=>document.querySelector('#river-film').readyState>=1);
    assert.ok(await page.locator('#river-film').evaluate(v=>v.duration>20),'current recording did not load');
    await page.locator('#river-film').evaluate(async v=>{v.muted=true;await v.play();});
    await page.waitForFunction(()=>document.querySelector('#river-film').currentTime>0.3);
    await page.locator('#river-film').evaluate(v=>{v.pause();v.currentTime=Math.min(22,v.duration-0.5);});
    await page.waitForFunction(()=>document.querySelector('#river-film').readyState>=2);
    await page.locator('img').evaluateAll(imgs=>imgs.forEach(img=>img.loading='eager'));
    await page.locator('img').evaluateAll(imgs=>Promise.all(imgs.map(img=>img.decode())));
    if(preview) {
        assert.ok(await page.locator('.preview-notice').isVisible());
        assert.equal(await page.locator('iframe').count(),0);
    } else {
        const meta=await (await page.request.get(new URL('build.json',base).href)).json();
        if(meta.release_identity) {
            assert.equal(await page.getByRole('link',{name:'Install on your FM-1'}).first().getAttribute('href'),'install/');
            const install=await page.request.get(new URL('install/',base).href);
            assert.ok(install.ok(),'installer missing');
            const html=await install.text();
            assert.ok(html.includes('"pkg": "../'+meta.package_path+'"'),'installer must fetch this exact content-hashed package');
            assert.ok(html.includes('"product": "'+meta.release_identity+'"'),'installer identity mismatch');
        }
        assert.equal(await page.locator('iframe').getAttribute('src'),meta.emulator_path+'?fw=blackwater&mhz=240&embed=1');
    }
    for(const [name,width,height] of [['desktop',1440,1100],['mobile',390,844]]) {
        await page.setViewportSize({width,height});
        assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'horizontal overflow at '+width);
        await page.screenshot({path:join(out,'site-'+name+'.png'),fullPage:true});
    }
    await page.setViewportSize({width:1280,height:1000});
    page.off('pageerror',onError);
    assert.deepEqual(errors,[],'landing page errors');
}

if(process.argv[1] && import.meta.url===pathToFileURL(resolve(process.argv[1])).href) {
    const [siteArg,outArg,flag]=process.argv.slice(2);
    if(!outArg) throw Error('usage: node tools/site_smoke.mjs SITE_DIR OUT_DIR [--preview]');
    const root=resolve(siteArg),out=resolve(outArg);mkdirSync(out,{recursive:true});
    const types={'.html':'text/html','.css':'text/css','.png':'image/png','.svg':'image/svg+xml',
                 '.mp4':'video/mp4','.json':'application/json'};
    const server=createServer((req,res)=>{
        let path=join(root,decodeURIComponent(new URL(req.url,'http://x').pathname));
        if(path.endsWith('/'))path=join(path,'index.html');
        if(!path.startsWith(root+'/') || !existsSync(path)){res.writeHead(404);res.end();return;}
        res.writeHead(200,{'content-type':types[extname(path)]||'application/octet-stream'});res.end(readFileSync(path));
    }).listen(0,'127.0.0.1');
    await new Promise(r=>server.once('listening',r));
    const require=createRequire(import.meta.url),{chromium}=require('playwright');
    const browser=await chromium.launch({executablePath:process.env.BLACKWATER_CHROMIUM||undefined});
    try {const page=await browser.newPage();await checkLanding(page,`http://127.0.0.1:${server.address().port}/`,out,{preview:flag==='--preview'});}
    finally {await browser.close();server.close();}
    console.log('landing: desktop/mobile, current video, images and links passed');
}

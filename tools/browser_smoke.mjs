// SPDX-License-Identifier: GPL-3.0-or-later
// Real-browser check of the assembled site: Chromium opens fm1-emu's own page with BLACKWATER's
// package, a visitor's keyboard drives the firmware, and the evidence is the page's own output:
// screenshots of its panel, the level of its AudioWorklet's output, and a save surviving a reload.
// The audio tap only listens: an AnalyserNode beside the page's own connection to the speakers.
//   node tools/browser_smoke.mjs SITE_DIR OUT_DIR
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {mkdirSync, mkdtempSync, readFileSync, writeFileSync, existsSync} from 'node:fs';
import {createRequire} from 'node:module';
import {extname, join, resolve} from 'node:path';
import {tmpdir} from 'node:os';

const [siteArg, outArg] = process.argv.slice(2);
if (!outArg) throw Error('usage: node tools/browser_smoke.mjs SITE_DIR OUT_DIR');
const site = resolve(siteArg), out = resolve(outArg);
mkdirSync(out, {recursive: true});
const require = createRequire(import.meta.url);
let playwright;
try { playwright = require('playwright'); }
catch { playwright = require(require.resolve('playwright', {paths: [resolve(process.execPath, '../../lib/node_modules')]})); }

const types = {'.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript', '.wasm': 'application/wasm',
    '.json': 'application/json', '.css': 'text/css', '.png': 'image/png', '.pdf': 'application/pdf'};
const server = createServer((req, res) => {
    const path = join(site, decodeURIComponent(new URL(req.url, 'http://x').pathname));
    const file = existsSync(path) && !path.endsWith('/') ? path : join(path, 'index.html');
    if (!file.startsWith(site) || !existsSync(file)) { res.writeHead(404); res.end(); return; }
    res.writeHead(200, {'content-type': types[extname(file)] || 'application/octet-stream'});
    res.end(readFileSync(file));
}).listen(0, '127.0.0.1');
await new Promise(r => server.once('listening', r));
const url = `http://127.0.0.1:${server.address().port}/emulator/?fw=blackwater`;

const tap = () => {
    const connect = AudioNode.prototype.connect;
    AudioNode.prototype.connect = function (dest, ...rest) {
        if (dest instanceof AudioDestinationNode) {
            const a = this.context.createAnalyser();
            a.fftSize = 2048;
            connect.call(this, a);
            window.__bwAnalyser = a;
        }
        return connect.call(this, dest, ...rest);
    };
    window.__bwLevel = () => {
        const a = window.__bwAnalyser;
        if (!a) return -1;
        const d = new Float32Array(a.fftSize);
        a.getFloatTimeDomainData(d);
        return Math.sqrt(d.reduce((s, v) => s + v * v, 0) / d.length);
    };
};

// the panel's display, cut from the page's own panel canvas (the knob graphics around it move too)
async function lcdShot(page, name) {
    const b = await page.locator('#panel').boundingBox();
    await page.locator('#panel').screenshot({path: join(out, name + '-panel.png')});
    return page.screenshot({path: join(out, name + '.png'),
        clip: {x: b.x + b.width * 0.28, y: b.y + b.height * 0.036, width: b.width * 0.435, height: b.height * 0.615}});
}

// open the page and press its Start button (again, if the first click was lost while the page loaded)
async function startPage(page) {
    await page.goto(url);
    for (let i = 0; ; i++) {
        await page.locator('#start').click();
        try { await page.waitForFunction(() => document.getElementById('start').hidden, null, {timeout: 30000}); break; }
        catch (e) { if (i === 2) throw e; await page.reload(); }
    }
    await page.waitForTimeout(2500);                       // the firmware's boot and first screen
    await page.locator('body').click({position: {x: 5, y: 5}});   // keyboard focus on the page
}

const profile = mkdtempSync(join(tmpdir(), 'bw-browser-'));
const browser = await playwright.chromium.launchPersistentContext(profile, {
    args: ['--autoplay-policy=no-user-gesture-required'], viewport: {width: 1280, height: 1000}});
const result = {url, steps: []};
const log = (step, extra = {}) => { result.steps.push({step, ...extra}); console.log(step, JSON.stringify(extra)); };
const errors = [];
try {
    const page = browser.pages()[0] || await browser.newPage();
    page.on('pageerror', e => errors.push(String(e)));
    await page.addInitScript(tap);

    // peak output level over ms, sampled every 50 ms
    const level = async ms => {
        let peak = 0;
        for (let t = 0; t < ms; t += 50) { peak = Math.max(peak, await page.evaluate(() => window.__bwLevel())); await page.waitForTimeout(50); }
        return peak;
    };
    const press = async (key, ms = 400) => { await page.keyboard.press(key); await page.waitForTimeout(ms); };
    const shot = name => lcdShot(page, name);
    const boot = () => startPage(page);

    await boot();
    const status = await page.locator('#status').textContent().catch(() => '');
    log('booted', {status});
    const river = await shot('1-river');
    const idle = await level(1000);
    log('stopped level', {rms: idle});

    await press('r');            // REC: cast the selected track's hook
    await press(' ');            // PLAY
    const playing = await level(3000);
    log('playing level', {rms: playing});
    assert.ok(playing > 0.001 && playing > idle * 4, `no audio from playback (${playing} vs ${idle})`);
    const cast = await shot('2-hook-placed');
    assert.notDeepEqual(cast, river, 'REC/PLAY did not change the display');

    await press('0');            // SEQ: fishing screen
    const fishing = await shot('3-fishing');
    assert.notDeepEqual(fishing, cast, 'SEQ did not change the display');

    // wait for automatic catches (default waits 8-32 s)
    await page.waitForTimeout(40000);
    await press('2');            // SEL: catch statistics
    const stats = await shot('4-stats-before-save');
    assert.notDeepEqual(stats, fishing, 'SEL did not change the display');
    log('statistics shown');

    await press(' ');            // stop, so the saved state is quiet
    await press('8', 1500);      // SAVE (the firmware's own storage object)
    await shot('5-after-save');
    await page.locator('#save-now').click();          // the page writes the emulated flash to this browser
    await page.waitForTimeout(1500);
    log('saved');

    await boot();                // reload: the same browser profile, a fresh firmware boot
    await press('2');
    const restored = await shot('6-stats-after-reload');
    log('reloaded');

    // the same screen in a browser profile that never saved: a fresh session's statistics
    const other = await playwright.chromium.launchPersistentContext(mkdtempSync(join(tmpdir(), 'bw-fresh-')), {
        args: ['--autoplay-policy=no-user-gesture-required'], viewport: {width: 1280, height: 1000}});
    const freshPage = other.pages()[0] || await other.newPage();
    await freshPage.addInitScript(tap);
    await startPage(freshPage);
    await freshPage.keyboard.press('2');
    await freshPage.waitForTimeout(400);
    const fresh = await lcdShot(freshPage, '7-stats-fresh-profile');
    // encoders, as the page's keys turn them (fm1-emu: arrows left/right SELECT, up/down PRESETS, , . ALGORITHM)
    const turn = async (key, name) => { await freshPage.keyboard.press(key); await freshPage.waitForTimeout(400); return lcdShot(freshPage, name); };
    await freshPage.keyboard.press('7');                 // HOME: the river
    await freshPage.waitForTimeout(400);
    const home = await lcdShot(freshPage, '8-river');
    const track2 = await turn('.', '9-algorithm-track2');
    assert.notDeepEqual(track2, home, 'ALGORITHM did not select another track');
    const presetsOutsideEdit = await turn('ArrowUp', '10-presets-outside-edit');
    assert.deepEqual(presetsOutsideEdit, track2, 'PRESETS changed a screen other than EDIT');
    const next = await turn('ArrowRight', '11-select-next-screen');
    assert.notDeepEqual(next, track2, 'SELECT did not move to the next screen');
    log('encoders change the display');
    await other.close();
    assert.notDeepEqual(restored, fresh, 'reloaded statistics equal a fresh session: the save did not persist');
    log('restored statistics differ from a fresh profile');
    result.errors = errors;
} finally {
    await browser.close();
    server.close();
}
writeFileSync(join(out, 'result.json'), JSON.stringify(result, null, 2) + '\n');
assert.deepEqual(errors, [], 'page errors');
console.log('browser_smoke: ok, evidence in', out);

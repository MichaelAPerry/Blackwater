// SPDX-License-Identifier: GPL-3.0-or-later
// Real-browser check of the assembled site: Chromium opens fm1-emu's own page with BLACKWATER's
// package, a visitor's keyboard drives the firmware, and the evidence is the page's own output:
// screenshots of its panel, the level of its AudioWorklet's output, and a save surviving a reload.
// The audio tap only listens: an AnalyserNode beside the page's own connection to the speakers.
//   node tools/browser_smoke.mjs SITE_DIR OUT_DIR
import assert from 'node:assert/strict';
import {createServer} from 'node:http';
import {mkdirSync, mkdtempSync, readFileSync, writeFileSync, existsSync, readdirSync} from 'node:fs';
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
const emuDir = readdirSync(site).find(d => d.startsWith('emulator-')) || 'emulator';   // this build's emulator
const url = `http://127.0.0.1:${server.address().port}/${emuDir}/?fw=blackwater&mhz=240`; // the chip's clock

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
    // a press as long as a finger's (a press shorter than the firmware's key scan can go unseen)
    const press = async (key, ms = 400) => { await page.keyboard.down(key); await page.waitForTimeout(120);
        await page.keyboard.up(key); await page.waitForTimeout(ms); };
    const shot = name => lcdShot(page, name);
    const part = async (name, y0, h) => {
        const b = await page.locator('#panel').boundingBox();
        const ly = b.y + b.height * 0.036, lh = b.height * 0.615;
        return page.screenshot({path: join(out, name + '.png'),
            clip: {x: b.x + b.width * 0.285, y: ly + lh * y0 / 240, width: b.width * 0.42, height: lh * h / 240}});
    };
    const wheel = async (cx, cy, clicks) => {
        const b = await page.locator('#panel').boundingBox();
        await page.mouse.move(b.x + cx / 1280 * b.width, b.y + cy / 820 * b.height);
        for (let i = 0; i < Math.abs(clicks); i++) { await page.mouse.wheel(0, clicks > 0 ? -40 : 40); await page.waitForTimeout(60); }
        await page.waitForTimeout(500);
    };
    const boot = () => startPage(page);

    await boot();
    const status = await page.locator('#status').textContent().catch(() => '');
    log('booted', {status});
    await press('7');                               // HOME on the river: the catch log
    const emptyLog = await shot('0-log-empty');   // the catch log before any fish
    const rowsEmpty = await part('0b-log-rows-empty', 54, 150);
    await press('7');                               // HOME again: back to the river
    const river = await shot('1-river');
    // the panel's own knobs, with the mouse (the wheel over a knob turns it a detent per notch), as a person does:
    // K1 on the river is the bluegill's level, shown in the knob bar; PRESET there is the weather, in the header
    const barBefore = await part('k1-bar-before', 211, 29);
    await wheel(935, 120, 5);                                  // KNOB1
    const barAfter = await part('k1-bar-after', 211, 29);
    assert.ok(!barAfter.equals(barBefore), 'K1 (mouse) did not change the knob bar');
    await wheel(935, 120, -5);
    // PRESETS: the weather (the header's icon and a message); a turn may land between two of the page's frames,
    // so up to three tries, each one kept as evidence
    let presetChanged = false;
    for (let i = 0; i < 3 && !presetChanged; i++) {
        const headBefore = await part(`preset-head-before-${i}`, 0, 47);
        await wheel(110, 295, 1);                              // PRESETS
        await page.waitForTimeout(400);
        const headAfter = await part(`preset-head-after-${i}`, 0, 47);
        presetChanged = !headAfter.equals(headBefore);
        log('preset try', {try: i, changed: presetChanged});
    }
    assert.ok(presetChanged, 'PRESET (mouse) did not change the header (the weather), three tries');
    log('mouse: K1 and PRESET change the screen');
    // encoders, as the page's keys turn them (fm1-emu: arrows left/right SELECT, up/down PRESETS, , . ALGORITHM)
    const turn = async (key, name) => { await page.keyboard.press(key); await page.waitForTimeout(400); return lcdShot(page, name); };
    const home = await lcdShot(page, '8-river');
    const track2 = await turn('.', '9-algorithm-track2');
    assert.ok(!track2.equals(home), 'ALGORITHM did not select another track');
    const next = await turn('ArrowRight', '11-select-next-screen');
    assert.ok(!next.equals(track2), 'SELECT did not move to the next screen');
    log('encoders change the display');
    await press(',');                                    // back to the first fish
    await press('7');                                    // and the river

    const idle = await level(1000);
    log('stopped level', {rms: idle});

    await press('r');            // REC: cast the selected track's hook
    await press(' ');            // PLAY
    const playing = await level(3000);
    log('playing level', {rms: playing});
    assert.ok(playing > 0.001 && playing > idle * 4, `no audio from playback (${playing} vs ${idle})`);
    const cast = await shot('2-hook-placed');
    assert.ok(!cast.equals(river), 'REC/PLAY did not change the display');

    await press('0');            // SEQ: fishing screen
    const fishing = await shot('3-fishing');
    assert.ok(!fishing.equals(cast), 'SEQ did not change the display');

    // play: answer the bites and reel in, as a player does, with REC (R), for forty seconds; screens on the way
    let shots = 0;
    for (let t = 0; t < 40000; t += 700) {
        await press('r', 580);
        if (t % 7000 < 700 && shots < 6) await shot(`3-playing-${shots++}`);
    }
    await press('7');            // HOME: the river (from FISHING)
    await press('7');            // HOME again: the catch log
    const stats = await shot('4-stats-before-save');
    assert.ok(!stats.equals(fishing), 'SEL did not change the display');
    log('statistics shown');

    log('after a minute', {status: await page.locator('#status').textContent().catch(() => '')});
    await press(' ');            // stop, so the saved state is quiet
    await press('8', 1500);      // SAVE (the firmware's own storage object)
    await shot('5-after-save');
    await page.locator('#save-now').click();          // the page writes the emulated flash to this browser
    await page.waitForTimeout(1500);
    const rowsBeforeSave = await part('4b-log-rows-at-save', 54, 150);   // the log as saved
    log('saved');

    await press('6');            // GLO: the audio's own load and xruns, after a minute of playing
    await shot('5b-global-load');
    await boot();                // reload: the same browser profile, a fresh firmware boot
    await press('7');            // the river is first: HOME once is the catch log
    const restored = await shot('6-stats-after-reload');
    log('reloaded');

    // the catch log's rows (not the header): playing caught fish, and the reload shows exactly those
    const rows = async name => part(name, 54, 150);
    const rowsRestored = await rows('6b-log-rows-after-reload');
    assert.ok(!rowsBeforeSave.equals(rowsEmpty), 'a minute of play caught nothing: the log rows are as empty as at the start');
    assert.ok(rowsRestored.equals(rowsBeforeSave), 'the catch log after the reload is not the one saved');
    log('playing caught fish; the catch log survived the reload');

    result.errors = errors;
} finally {
    await browser.close().catch(() => {});
    server.close();
}
writeFileSync(join(out, 'result.json'), JSON.stringify(result, null, 2) + '\n');
assert.deepEqual(errors, [], 'page errors');
console.log('browser_smoke: ok, evidence in', out);

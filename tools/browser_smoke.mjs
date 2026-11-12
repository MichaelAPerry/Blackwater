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
import {checkLanding} from './site_smoke.mjs';

const [siteArg, outArg] = process.argv.slice(2);
if (!outArg) throw Error('usage: node tools/browser_smoke.mjs SITE_DIR OUT_DIR');
const site = resolve(siteArg), out = resolve(outArg);
mkdirSync(out, {recursive: true});
const require = createRequire(import.meta.url);
let playwright;
try { playwright = require('playwright'); }
catch { playwright = require(require.resolve('playwright', {paths: [resolve(process.execPath, '../../lib/node_modules')]})); }

const types = {'.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript', '.wasm': 'application/wasm',
    '.json': 'application/json', '.css': 'text/css', '.png': 'image/png', '.pdf': 'application/pdf',
    '.svg': 'image/svg+xml', '.mp4': 'video/mp4'};
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
    executablePath: process.env.BLACKWATER_CHROMIUM || undefined,
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

    await checkLanding(page, `http://127.0.0.1:${server.address().port}/`, out);
    log('landing: desktop/mobile, recording and current installer checked');
    await boot();
    log('booted', {status: await page.locator('#status').textContent().catch(() => '')});
    await press('7'); // HOME -> catch log, before any encounters.
    const rowsEmpty = await part('0-log-empty', 54, 150);
    await press('7');

    // PRESET loads a sound world while stopped, and changes its actual heading.
    const worldBefore = await part('world-before', 0, 47);
    await wheel(110, 295, 1);
    const worldAfter = await part('world-after', 0, 47);
    assert.ok(!worldBefore.equals(worldAfter), 'PRESET did not load another world');
    await wheel(110, 295, -1); // DUSK again.
    await press('5'); // EDIT has the real parameter bar; RIVER is deliberately a clean landscape.
    const ratioBefore = await part('ratio-before', 211, 29);
    await wheel(935, 120, 4);
    const ratioAfter = await part('ratio-after', 211, 29);
    assert.ok(!ratioBefore.equals(ratioAfter), 'K1 did not edit the FM ratio');
    await wheel(935, 120, -4);
    const voiceBefore = await part('voice-before', 0, 70);
    await page.keyboard.press('.'); await page.waitForTimeout(500);
    const voiceAfter = await part('voice-after', 0, 70);
    assert.ok(!voiceBefore.equals(voiceAfter), 'ALGO did not select another voice');
    await page.keyboard.press('ArrowRight'); await page.waitForTimeout(500);
    const envelope = await part('select-envelope', 0, 70);
    assert.ok(!envelope.equals(voiceAfter), 'SELECT did not open the next editor');
    await page.keyboard.press(','); await page.waitForTimeout(400);
    log('mouse/encoders: world, FM ratio, voice and page controls work');

    await press('6'); // GLOBAL page 1. A persistent, unambiguous saved parameter as well as catch records.
    const tempoDefault = await part('tempo-default', 50, 50);
    await wheel(935, 120, 7); // 96 -> 103 BPM.
    const tempoSaved = await part('tempo-edited', 50, 50);
    assert.ok(!tempoSaved.equals(tempoDefault), 'tempo did not change');
    await press('7');
    const idle = await level(1000);
    await press(' '); // The line starts in the water; PLAY also sounds with it lifted.
    const playing = await level(4000);
    assert.ok(playing > 0.001 && playing > idle * 4, `no river playback (${playing} vs ${idle})`);
    await shot('river-playing');
    log('river audio', {rms: playing});

    await press('0'); // SEQ -> line settings.
    await wheel(935, 120, -25);  // minimum wait -> 0 (five seconds per detent).
    await wheel(1028, 120, -50); // maximum wait -> 0.
    await wheel(1121, 120, -1);  // FISH -> LISTEN: retain the real approach/strike/fight/reveal timings.
    await shot('line-listen-zero-wait');
    await press('7'); await press('7'); // Catch log: the encounter continues off-page.
    let caught = false;
    for (let second = 0; second < 90; second++) {
        await page.waitForTimeout(1000);
        const rows = await part('log-waiting', 54, 150);
        if (!rows.equals(rowsEmpty)) { caught = true; break; }
    }
    assert.ok(caught, 'LISTEN did not complete an encounter while the catch log was open');
    await shot('catch-recorded');
    await press(' '); // pause before saving; no additional encounter can change the record.
    await press('8', 1500);
    await page.locator('#save-now').click(); await page.waitForTimeout(1500);
    const rowsAtSave = await part('log-at-save', 54, 150);
    await press('6'); await shot('global-load');
    log('catch and tempo saved', {status: await page.locator('#status').textContent().catch(() => '')});

    await boot();
    await press('7');
    const restored = await part('log-restored', 54, 150);
    assert.ok(restored.equals(rowsAtSave), 'catch records did not survive reload');
    await press('6');
    const tempoRestored = await part('tempo-restored', 50, 50);
    assert.ok(tempoRestored.equals(tempoSaved), 'edited tempo did not survive reload');
    log('saved catch records and tempo survived reload');

    // New performance page and SELECT behaviour in FREE, using only the emulator's real panel.
    for (let i = 0; i < 3; i++) { await page.keyboard.press('ArrowUp'); await page.waitForTimeout(400); }
    await wheel(1028, 120, 1); // unlock all 27 river keys.
    await wheel(935, 120, 1);  // FREE.
    await press('7');
    const operatorBefore = await part('free-operator-1', 0, 70);
    await page.keyboard.press('ArrowRight'); await page.waitForTimeout(400);
    const operatorAfter = await part('free-operator-2', 0, 70);
    assert.ok(!operatorAfter.equals(operatorBefore), 'FREE SELECT did not choose an operator');
    await page.waitForTimeout(8000); // let earlier river/reverb tails settle.
    const freeIdle = await level(500);
    await page.keyboard.down('w'); // a black piano key must be a note, not a macro, in FREE.
    const freeNote = await level(2000);
    await page.keyboard.up('w');
    assert.ok(freeNote > 0.0003 && freeNote > freeIdle * 2, 'FREE black key did not play the FM voice');
    await shot('free-play');
    log('FREE: SELECT operator and chromatic key audio', {idle: freeIdle, note: freeNote});

    result.errors = errors;
} finally {
    await browser.close().catch(() => {});
    server.close();
}
writeFileSync(join(out, 'result.json'), JSON.stringify(result, null, 2) + '\n');
assert.deepEqual(errors, [], 'page errors');
console.log('browser_smoke: ok, evidence in', out);

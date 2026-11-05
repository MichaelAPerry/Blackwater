// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the existing emulator's Wasm with the actual device package, without a browser claim.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {packageMetadata} from './package_metadata.mjs';
const [dir, pkg, app] = process.argv.slice(2);
if (!app) throw Error('usage: node tools/emulator_wasm_smoke.mjs EMULATOR_DIR PACKAGE APP');
const metadata = packageMetadata(pkg, app);
const base = pathToFileURL(resolve(dir) + '/');
const {jitImports} = await import(new URL('jit.js', base));
let x;
x = (await WebAssembly.instantiate(readFileSync(new URL('fm1_web.wasm', base)),
    {env: jitImports(() => x)})).instance.exports;
x.fm1_jit(1,16);
x.fm1_ahead(1024);
const bytes = readFileSync(pkg), ptr = x.fm1_alloc(bytes.length);
new Uint8Array(x.memory.buffer,ptr,bytes.length).set(bytes);
assert.equal(x.fm1_boot_fwsc(ptr,bytes.length,0,0,100),0,'package rejected');
function run(seconds) {
    let frames=0, power=0;
    for(let f=0;f<seconds*44100;f+=128){
        frames+=x.fm1_render(128,50_000_000);
        for(const sample of new Float32Array(x.memory.buffer,x.fm1_out_l(),128)){
            assert.ok(Number.isFinite(sample));power+=sample*sample;
        }
        assert.equal(x.fm1_halted(),0,'firmware/emulator halted');
    }
    return {frames,rms:Math.sqrt(power/(Math.ceil(seconds*44100/128)*128))};
}
function screen(){return Buffer.from(new Uint8Array(x.memory.buffer,x.fm1_lcd(),240*240*2));}
function tap(key){x.fm1_key(key,1);run(0.16);x.fm1_key(key,0);run(0.1);}
run(1.2);
assert.ok(new Set(new Uint16Array(x.memory.buffer,x.fm1_lcd(),240*240)).size>2,'blank display');
tap(13); // REC, mapping verified in fm1-soc's periph::key_id.
tap(12); // PLAY.
const playing=run(1);
assert.ok(playing.frames>40000 && playing.rms>0.0003,'playback did not produce audio');
const river=screen();tap(11); // SEQ.
assert.notDeepEqual(screen(),river,'SEQ did not change display');
const fishing=screen();tap(3); // SEL/SCL -> catch statistics.
assert.notDeepEqual(screen(),fishing,'SEL did not change display');
console.log(JSON.stringify({...metadata,wasm_audio_rms:playing.rms,wasm_audio_frames:playing.frames},null,2));

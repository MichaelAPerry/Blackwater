// SPDX-License-Identifier: GPL-3.0-or-later
// Use the established package parsers; do not invent a second firmware format.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import {pathToFileURL} from 'node:url';
import {appFromPackage} from '../platform/fm1-emu/web/fwsc.js';
import {productOf, logicalImage, checkUfw} from '../platform/fm1-base/web/fm1pkg.js';
export function packageMetadata(packagePath, appPath) {
    const bytes = readFileSync(packagePath);
    const hash = data => createHash('sha256').update(data).digest('hex');
    // Hash the file as read: the upstream parsers decode the buffers they are given in place,
    // so each one gets its own copy.
    const packageSha = hash(bytes);
    checkUfw(logicalImage(Uint8Array.from(bytes)));
    const identity = productOf(Uint8Array.from(bytes));
    assert.match(identity, /^FM-1_\d+$/, 'invalid device package identity');
    assert.ok(!['FM-1_000', 'FM-1_1100'].includes(identity), 'placeholder/template identity');
    const app = Buffer.from(appFromPackage(Uint8Array.from(bytes)));
    let expected = readFileSync(appPath);
    while (expected.length && expected.at(-1) === 255) expected = expected.subarray(0, -1);
    assert.ok(expected.includes(Buffer.from('BLACKWATER')), 'app is not identified as BLACKWATER');
    assert.deepEqual(app, expected, 'package app differs from the intended build binary');
    assert.equal(hash(readFileSync(packagePath)), packageSha, 'package changed while being checked');
    return {identity, package_bytes: bytes.length, package_sha256: packageSha, app_bytes: app.length,
        app_sha256: hash(app)};
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
    if (process.argv.length !== 4) throw Error('usage: node tools/package_metadata.mjs PACKAGE.fwsc APP.bin');
    console.log(JSON.stringify(packageMetadata(process.argv[2], process.argv[3]), null, 2));
}

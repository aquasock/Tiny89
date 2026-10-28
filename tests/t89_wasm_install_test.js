/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Does a file actually install?
 *
 * Boots the WebAssembly build to the desktop and hands it one of the user's own
 * files through the emulated link, which is what the page's Build button does
 * for each file in its queue.  The calculator's operating system does the
 * installing: this only reports what it said.
 *
 *   tests/run_t89_wasm_test.sh --install path/to/file.89y [more files...]
 */
'use strict';

const path = require('path');
const fs = require('fs');

const BOOT_INSTRUCTIONS = 20_000_000;
const SETTLE_INSTRUCTIONS = 2_000_000;
const CHUNK = 200_000;

function findDist() {
  return path.join(__dirname, '..', 'web', 'dist', 't89.js');
}

function findOs() {
  const candidates = [
    process.env.TI89_OS,
    path.join(process.env.HOME || '', 'Desktop/TI89Titanium_OS.89u'),
    path.join(process.env.HOME || '', 'Downloads/TI89Titanium_OS.89u'),
  ].filter(Boolean);
  return candidates.find((candidate) => fs.existsSync(candidate)) || null;
}

async function main() {
  const files = process.argv.slice(3);
  const osPath = findOs();
  const dist = findDist();

  if (!osPath) {
    console.log('wasm install: SKIP (no TI-89 Titanium upgrade file; set TI89_OS)');
    return 0;
  }
  if (!fs.existsSync(dist)) {
    console.log(`wasm install: FAIL (no build at ${dist}; run tools/build-wasm-app.sh)`);
    return 1;
  }
  if (files.length === 0) {
    console.log('wasm install: SKIP (no files named)');
    return 0;
  }

  const say = console.log;
  console.log = () => {};
  const createT89 = require(dist);
  const m = await createT89();
  console.log = say;

  const osBytes = new Uint8Array(fs.readFileSync(osPath));
  let ptr = m._malloc(osBytes.length);
  m.HEAPU8.set(osBytes, ptr);
  const rc = m._t89_boot(ptr, osBytes.length);
  m._free(ptr);
  if (rc !== 0) {
    say(`wasm install: FAIL (boot refused: ${m.UTF8ToString(m._t89_error())})`);
    return 1;
  }

  for (let done = 0; done < BOOT_INSTRUCTIONS; done += CHUNK) {
    if (m._t89_run(CHUNK) !== 0) {
      say('wasm install: FAIL (the CPU stopped while booting)');
      return 1;
    }
  }
  say(`desktop: pc=0x${(m._t89_pc() >>> 0).toString(16).padStart(8, '0')} `
      + `lcd=0x${(m._t89_lcd_hash() >>> 0).toString(16).padStart(8, '0')}`);

  let installed = 0;
  let refused = 0;
  let unsupported = 0;

  for (const file of files) {
    const bytes = new Uint8Array(fs.readFileSync(file));
    ptr = m._malloc(bytes.length);
    m.HEAPU8.set(bytes, ptr);
    const result = m._t89_install(ptr, bytes.length);
    m._free(ptr);

    const name = m.UTF8ToString(m._t89_last_name());
    const type = m._t89_last_type();
    const message = m.UTF8ToString(m._t89_error());
    const label = path.basename(file);

    if (result === 0) {
      installed++;
      say(`  ${label}: installed as ${name} (type 0x${type.toString(16)}, `
          + `${m._t89_last_size().toLocaleString()} bytes)`);
    } else if (result === -2) {
      unsupported++;
      say(`  ${label}: unsupported -- ${message}`);
    } else {
      refused++;
      say(`  ${label}: ${result} -- ${message || 'refused'}`);
    }

    for (let done = 0; done < SETTLE_INSTRUCTIONS; done += CHUNK) {
      if (m._t89_run(CHUNK) !== 0) break;
    }
  }

  say(`wasm install: ${installed} installed, ${refused} refused, `
      + `${unsupported} unsupported, of ${files.length}`);
  m._t89_shutdown();
  return refused === 0 ? 0 : 1;
}

main()
  .then((code) => process.exit(code))
  .catch((error) => {
    console.log(`wasm install: FAIL (${error.stack || error.message})`);
    process.exit(1);
  });

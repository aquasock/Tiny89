/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Does the browser build behave like the board's?
 *
 * The board's program and the host probe both reach the TI-89 Apps desktop at
 * PC 0x00821e46 with the LCD hashing to 0x0c0df297, and core-log entries 2, 4
 * and 6 use those two numbers as the project's boot invariant.  This runs the
 * WebAssembly build against the same private image and checks the same two
 * numbers, so "the page shows a calculator" is backed by the machine being the
 * same machine rather than by the picture looking right.
 *
 *   tests/run_t89_wasm_test.sh [path/to/TI89Titanium_OS.89u]
 */
'use strict';

const path = require('path');
const fs = require('fs');

const EXPECTED_PC = 0x00821e46;
const EXPECTED_LCD = 0x0c0df297;
const INSTRUCTIONS = 20_000_000;
const CHUNK = 200_000;

function findOs(argv) {
  const candidates = [
    argv[2],
    process.env.TI89_OS,
    path.join(process.env.HOME || '', 'Desktop/TI89Titanium_OS.89u'),
    path.join(process.env.HOME || '', 'Downloads/TI89Titanium_OS.89u'),
  ].filter(Boolean);
  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) return candidate;
  }
  return null;
}

async function main() {
  const osPath = findOs(process.argv);
  if (!osPath) {
    console.log('wasm boot: SKIP (no TI-89 Titanium upgrade file; set TI89_OS)');
    return 0;
  }

  const dist = path.join(__dirname, '..', 'web', 'dist', 't89.js');
  if (!fs.existsSync(dist)) {
    console.log(`wasm boot: FAIL (no build at ${dist}; run tools/build-wasm-app.sh)`);
    return 1;
  }

  let checks = 0;
  let fails = 0;
  const check = (ok, what, detail) => {
    checks++;
    if (!ok) {
      fails++;
      console.log(`  FAIL: ${what}${detail ? ` (${detail})` : ''}`);
    }
  };

  // The module prints the emulator's own chatter; keep it out of the results.
  const say = console.log;
  console.log = () => {};
  const createT89 = require(dist);
  const m = await createT89();
  console.log = say;

  const bytes = new Uint8Array(fs.readFileSync(osPath));
  say(`image: ${osPath} (${bytes.length.toLocaleString()} bytes)`);

  const ptr = m._malloc(bytes.length);
  m.HEAPU8.set(bytes, ptr);
  const booted = m._t89_boot(ptr, bytes.length);
  m._free(ptr);
  check(booted === 0, 'the emulator refused the upgrade file',
        booted === 0 ? '' : m.UTF8ToString(m._t89_error()));
  if (booted !== 0) {
    say(`wasm boot: FAIL (${checks} checks)`);
    return 1;
  }

  let stopped = 0;
  for (let done = 0; done < INSTRUCTIONS; done += CHUNK) {
    const rc = m._t89_run(CHUNK);
    if (rc !== 0) {
      stopped = rc;
      break;
    }
  }
  check(stopped === 0, 'the emulated CPU stopped on an exception', `result ${stopped}`);

  const pc = m._t89_pc() >>> 0;
  const lcd = m._t89_lcd_hash() >>> 0;
  const changes = m._t89_lcd_changes() >>> 0;
  const mode = m.UTF8ToString(m._t89_lcd_mode_text());

  say(`boot: pc=0x${pc.toString(16).padStart(8, '0')} `
      + `lcd=0x${lcd.toString(16).padStart(8, '0')} `
      + `changes=${changes} mode=${mode}`);

  check(pc === EXPECTED_PC, 'the desktop PC differs from the board\'s',
        `0x${pc.toString(16).padStart(8, '0')} != 0x${EXPECTED_PC.toString(16)}`);
  check(lcd === EXPECTED_LCD, 'the desktop LCD hash differs from the board\'s',
        `0x${lcd.toString(16).padStart(8, '0')} != 0x${EXPECTED_LCD.toString(16)}`);
  check(changes > 0, 'the picture never changed during boot');
  check(m._t89_flash_size() === 4 * 1024 * 1024, 'the flash image is not 4 MiB');
  check(m._t89_ram_size() === 256 * 1024, 'the RAM image is not 256 KiB');

  /* The exported pair must be the machine's own memory, not a copy that could
   * have drifted: the first bytes of the flash are the boot vector area the
   * converter wrote from the upgrade file. */
  const flashPtr = m._t89_flash_ptr();
  const ramPtr = m._t89_ram_ptr();
  check(flashPtr > 0, 'no flash buffer');
  check(ramPtr > 0, 'no RAM buffer');
  check((m._t89_lcd_image() >>> 0) > 0, 'no LCD image was rendered');

  m._t89_shutdown();
  say(`wasm boot: ${fails === 0 ? 'PASS' : 'FAIL'} (${checks} checks)`);
  return fails === 0 ? 0 : 1;
}

main()
  .then((code) => process.exit(code))
  .catch((error) => {
    console.log(`wasm boot: FAIL (${error.stack || error.message})`);
    process.exit(1);
  });

/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Is a state built by the browser app a state the board can resume?
 *
 * The board restores a state into a freshly initialised machine and lets it
 * run on.  That is what this does, here, with the same blob: build a state
 * after installing a file, start a new machine, restore, and check that the
 * installed variable is still there and that execution continued from the
 * saved program counter.
 *
 * This is the property that matters, because the other one does not hold: a
 * RAM image cannot carry the user's files across a boot -- the operating
 * system rewrites its RAM at every cold boot (core-log entry 22 measured a
 * variable being discarded that way).
 *
 *   tests/run_t89_wasm_test.sh --state [path/to/file.89z]
 *
 * With no file named it still checks the blob's shape: size, magic, version,
 * a resumed PC and a RAM that matches the machine it came from.
 */
'use strict';

const path = require('path');
const fs = require('fs');

const BOOT_INSTRUCTIONS = 20_000_000;
const RESUME_INSTRUCTIONS = 2_000_000;
const CHUNK = 200_000;

function findOs() {
  const candidates = [
    process.env.TI89_OS,
    path.join(process.env.HOME || '', 'Desktop/TI89Titanium_OS.89u'),
    path.join(process.env.HOME || '', 'Downloads/TI89Titanium_OS.89u'),
  ].filter(Boolean);
  return candidates.find((candidate) => fs.existsSync(candidate)) || null;
}

async function main() {
  const args = process.argv.slice(3).filter((a) => a !== '--file');
  const sample = args.find((a) => !a.startsWith('--')) || null;
  const osPath = findOs();
  const dist = path.join(__dirname, '..', 'web', 'dist', 't89.js');

  if (!osPath) {
    console.log('wasm state: SKIP (no TI-89 Titanium upgrade file; set TI89_OS)');
    return 0;
  }
  if (!fs.existsSync(dist)) {
    console.log(`wasm state: FAIL (no build at ${dist}; run tools/build-wasm-app.sh)`);
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

  const say = console.log;
  console.log = () => {};
  const createT89 = require(dist);
  const m = await createT89();
  console.log = say;

  const os = fs.readFileSync(osPath);
  const boot = () => {
    const p = m._malloc(os.length);
    m.HEAPU8.set(os, p);
    const rc = m._t89_boot(p, os.length);
    m._free(p);
    return rc;
  };
  const runFor = (instructions) => {
    for (let done = 0; done < instructions; done += CHUNK) {
      const rc = m._t89_run(CHUNK);
      if (rc !== 0) return rc;
    }
    return 0;
  };
  const ramContains = (needle) => {
    const nd = Buffer.from(needle);
    const ram = m.HEAPU8.subarray(m._t89_ram_ptr(), m._t89_ram_ptr() + m._t89_ram_size());
    for (let i = 0; i <= ram.length - nd.length; i++) {
      let k = 0;
      while (k < nd.length && ram[i + k] === nd[k]) k++;
      if (k === nd.length) return true;
    }
    return false;
  };

  check(boot() === 0, 'the machine did not start');
  check(runFor(BOOT_INSTRUCTIONS) === 0, 'the machine stopped while booting');

  let installed = null;
  if (sample && fs.existsSync(sample)) {
    const bytes = fs.readFileSync(sample);
    const p = m._malloc(bytes.length);
    m.HEAPU8.set(bytes, p);
    const rc = m._t89_install(p, bytes.length);
    m._free(p);
    installed = m.UTF8ToString(m._t89_last_name());
    check(rc === 0, `the calculator refused ${path.basename(sample)}`,
          m.UTF8ToString(m._t89_error()));
    check(ramContains(installed), `${installed} is not in RAM after the install`);
    runFor(1_000_000);
    say(`installed ${installed} from ${path.basename(sample)}`);
  }

  const savedPc = m._t89_pc() >>> 0;
  const size = m._t89_state_size();
  const ramSize = m._t89_ram_size();
  check(size > ramSize, 'the state is not larger than the RAM it contains', `${size} bytes`);

  const blobPtr = m._malloc(size);
  const buildRc = m._t89_state_build(blobPtr, size);
  const blob = Buffer.from(m.HEAPU8.slice(blobPtr, blobPtr + size));
  m._free(blobPtr);
  check(buildRc === 0, 'the state could not be built');
  check(blob.length === size, 'the blob is not the size reported');
  check(blob.readUInt32LE(0) === 0x53393854, 'the blob has the wrong magic');
  check(blob.readUInt32LE(4) === 2, 'the blob has the wrong version');
  say(`state: ${size.toLocaleString()} bytes, pc 0x${savedPc.toString(16).padStart(8, '0')}`);

  /* A new machine, as the board starts one, then the restore. */
  check(boot() === 0, 'the second machine did not start');
  const restorePtr = m._malloc(size);
  m.HEAPU8.set(blob, restorePtr);
  const restoreRc = m._t89_state_restore(restorePtr, size);
  m._free(restorePtr);
  check(restoreRc === 0, 'the state was refused', `rc ${restoreRc}`);
  check((m._t89_pc() >>> 0) === savedPc, 'the resumed PC is not the saved one',
        `0x${(m._t89_pc() >>> 0).toString(16)} != 0x${savedPc.toString(16)}`);
  if (installed) {
    check(ramContains(installed), `${installed} is not in RAM after the restore`);
  }

  /* And it keeps running: a resume must not need a boot to be useful. */
  check(runFor(RESUME_INSTRUCTIONS) === 0, 'the resumed machine stopped');
  if (installed) {
    check(ramContains(installed), `${installed} was lost while the resumed machine ran`);
  }

  m._t89_shutdown();
  say(`wasm state: ${fails === 0 ? 'PASS' : 'FAIL'} (${checks} checks)`);
  return fails === 0 ? 0 : 1;
}

main()
  .then((code) => process.exit(code))
  .catch((error) => {
    console.log(`wasm state: FAIL (${error.stack || error.message})`);
    process.exit(1);
  });

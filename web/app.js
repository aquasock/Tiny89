/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The page's half of the image builder.  Everything that matters -- booting the
 * operating system, the link port, installing a file -- is in t89_web.c and the
 * emulator; this file moves bytes in and out and draws the picture.
 *
 * The budget below is in emulated instructions, not time: the host probe
 * reaches the Apps desktop in about twenty million, so the same twenty here is
 * the same machine at the same point.  It is not a timeout, and a slower
 * machine takes longer rather than doing less.
 */
'use strict';

const BOOT_INSTRUCTIONS = 20_000_000;
const CHUNK = 200_000;
const SETTLE_INSTRUCTIONS = 2_000_000;   /* after each install */
const FINAL_INSTRUCTIONS = 4_000_000;    /* the OS writes flash, then settles */

const ui = {
  os: document.getElementById('os-name'),
  files: document.getElementById('files-name'),
  list: document.getElementById('file-list'),
  build: document.getElementById('build'),
  phase: document.getElementById('phase'),
  bar: document.getElementById('bar-fill'),
  log: document.getElementById('log'),
  results: document.getElementById('results'),
  lcd: document.getElementById('lcd'),
  status: {
    pc: document.getElementById('s-pc'),
    cycles: document.getElementById('s-cycles'),
    lcd: document.getElementById('s-lcd'),
    mode: document.getElementById('s-mode'),
  },
};

const state = {
  module: null,
  os: null,
  osName: '',
  files: [],
  busy: false,
  total: 0,
  done: 0,
};

function log(line) {
  const at = new Date().toLocaleTimeString();
  ui.log.textContent += `[${at}] ${line}\n`;
  ui.log.scrollTop = ui.log.scrollHeight;
}

function tick() {
  return new Promise((resolve) => setTimeout(resolve, 0));
}

function setProgress(fraction, phase) {
  const pct = Math.max(0, Math.min(1, fraction)) * 100;
  ui.bar.style.width = `${pct.toFixed(1)}%`;
  if (phase) ui.phase.textContent = phase;
}

/* ---- moving bytes in and out of the module's memory ---- */

function withBytes(bytes, fn) {
  const m = state.module;
  const ptr = m._malloc(bytes.length);
  if (!ptr) throw new Error(`out of memory for ${bytes.length} bytes`);
  try {
    m.HEAPU8.set(bytes, ptr);
    return fn(ptr);
  } finally {
    m._free(ptr);
  }
}

function wasmError() {
  return state.module.UTF8ToString(state.module._t89_error());
}

function copyOut(ptr, size) {
  return state.module.HEAPU8.slice(ptr, ptr + size);
}

/* ---- the picture ---- */

const palette = [0xf2, 0xb4, 0x74, 0x18];   /* paper to ink, four levels */
const lcd = {
  ctx: ui.lcd.getContext('2d'),
  image: null,
};

function drawLcd() {
  const m = state.module;
  const ptr = m._t89_lcd_image();
  if (!ptr) return;
  const width = m._t89_lcd_width();
  const height = m._t89_lcd_height();
  const stride = m._t89_lcd_stride();
  if (!lcd.image) {
    lcd.image = lcd.ctx.createImageData(width, height);
  }
  const pixels = lcd.image.data;
  const heap = m.HEAPU8;
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const level = Math.min(3, heap[ptr + y * stride + x]);
      const shade = palette[level];
      const at = (y * width + x) * 4;
      pixels[at] = shade;
      pixels[at + 1] = shade;
      pixels[at + 2] = shade;
      pixels[at + 3] = 255;
    }
  }
  lcd.ctx.putImageData(lcd.image, 0, 0);
}

function updateStatus() {
  const m = state.module;
  if (!m || !state.started) return;
  const pc = m._t89_pc() >>> 0;
  const cycles = m._t89_cycles() >>> 0;
  const hash = m._t89_lcd_hash() >>> 0;
  ui.status.pc.textContent = `0x${pc.toString(16).padStart(8, '0')}`;
  ui.status.cycles.textContent = cycles.toLocaleString();
  ui.status.lcd.textContent = `0x${hash.toString(16).padStart(8, '0')}`;
  ui.status.mode.textContent = m.UTF8ToString(m._t89_lcd_mode_text());
}

/* ---- running ---- */

async function runInstructions(count, phase) {
  const m = state.module;
  let done = 0;
  while (done < count) {
    const chunk = Math.min(CHUNK, count - done);
    const rc = m._t89_run(chunk);
    done += chunk;
    state.done += chunk;
    if (rc !== 0) {
      log(`the emulated CPU stopped: result ${rc} at PC ${ui.status.pc.textContent}`);
      return false;
    }
    setProgress(state.done / state.total, phase);
    updateStatus();
    if (done % (CHUNK * 8) === 0) {
      drawLcd();
      await tick();
    }
  }
  drawLcd();
  await tick();
  return true;
}

/* ---- the build ---- */

function fileBytes(file) {
  return file.arrayBuffer().then((buffer) => new Uint8Array(buffer));
}

async function build() {
  if (state.busy || !state.os) return;
  state.busy = true;
  ui.build.disabled = true;
  ui.results.classList.add('hidden');
  ui.log.textContent = '';
  state.done = 0;
  state.total = BOOT_INSTRUCTIONS + FINAL_INSTRUCTIONS
    + state.files.length * SETTLE_INSTRUCTIONS;

  try {
    const module = state.module;
    const osBytes = new Uint8Array(await state.os.arrayBuffer());
    log(`${state.osName}: ${osBytes.length.toLocaleString()} bytes`);
    if (osBytes.length < 16) throw new Error('that file is too small to be an upgrade');

    setProgress(0, 'converting and starting the calculator');
    const rc = withBytes(osBytes, (ptr) => module._t89_boot(ptr, osBytes.length));
    if (rc !== 0) throw new Error(wasmError());
    state.started = true;
    log('the calculator is starting');

    setProgress(0, 'booting the operating system');
    if (!await runInstructions(BOOT_INSTRUCTIONS, 'booting the operating system')) {
      throw new Error('the emulated CPU stopped while booting');
    }
    log(`desktop reached: PC ${ui.status.pc.textContent}, LCD ${ui.status.lcd.textContent}`);

    for (const entry of state.files) {
      setProgress(state.done / state.total, `installing ${entry.name}`);
      const bytes = await fileBytes(entry.file);
      let rc = 0;
      try {
        rc = withBytes(bytes, (ptr) => module._t89_install(ptr, bytes.length));
      } catch (error) {
        entry.status = `failed: ${error.message}`;
        log(`${entry.name}: ${entry.status}`);
        continue;
      }
      const name = module.UTF8ToString(module._t89_last_name());
      const type = module._t89_last_type();
      if (rc === 0) {
        entry.status = `installed as ${name} (type 0x${type.toString(16)})`;
        log(`${entry.name}: ${entry.status}`);
      } else {
        entry.status = wasmError() || `refused (${rc})`;
        log(`${entry.name}: ${entry.status}`);
      }
      renderList();
      if (!await runInstructions(SETTLE_INSTRUCTIONS, `installing ${entry.name}`)) {
        break;
      }
    }

    setProgress(state.done / state.total, 'letting the operating system settle');
    await runInstructions(FINAL_INSTRUCTIONS, 'letting the operating system settle');
    log(`settled: PC ${ui.status.pc.textContent}, LCD ${ui.status.lcd.textContent}`);

    exportImages();
    setProgress(1, 'done');
  } catch (error) {
    log(`error: ${error.message}`);
    setProgress(0, `stopped: ${error.message}`);
  } finally {
    state.busy = false;
    ui.build.disabled = !state.os;
  }
}

function exportImages() {
  const m = state.module;
  const flashSize = m._t89_flash_size();
  const ramSize = m._t89_ram_size();
  const flash = copyOut(m._t89_flash_ptr(), flashSize);
  const ram = copyOut(m._t89_ram_ptr(), ramSize);

  const base = (state.osName || 'TI89Titanium_OS').replace(/\.[^.]+$/, '');
  const flashLink = ui.results.querySelector('#dl-flash');
  const ramLink = ui.results.querySelector('#dl-ram');
  const stateLink = ui.results.querySelector('#dl-state');

  /* The machine itself: this is what keeps the installed files, because it is
   * resumed rather than booted.  A cold boot rebuilds RAM and takes them with
   * it (core-log entry 22). */
  const stateSize = m._t89_state_size();
  const statePtr = m._malloc(stateSize);
  const built = m._t89_state_build(statePtr, stateSize);
  const machine = built === 0 ? copyOut(statePtr, stateSize) : null;
  m._free(statePtr);
  if (machine) {
    stateLink.href = URL.createObjectURL(new Blob([machine], { type: 'application/octet-stream' }));
    stateLink.download = `${base}.state`;
    stateLink.textContent = `state (${(stateSize / 1024).toFixed(0)} KiB) — keeps your files`;
  } else {
    stateLink.classList.add('hidden');
    log('the state could not be built; only the images are offered');
  }

  flashLink.href = URL.createObjectURL(new Blob([flash], { type: 'application/octet-stream' }));
  flashLink.download = `${base}.flash`;
  flashLink.textContent = `flash image (${(flashSize / 1048576).toFixed(0)} MiB)`;

  ramLink.href = URL.createObjectURL(new Blob([ram], { type: 'application/octet-stream' }));
  ramLink.download = `${base}.ram`;
  ramLink.textContent = `RAM image (${(ramSize / 1024).toFixed(0)} KiB)`;

  ui.results.classList.remove('hidden');
  log(`wrote ${base}.state, ${base}.flash and ${base}.ram`);
  log('state -> /root/states/ti89/, flash and RAM -> /root/saves/ti89/');
  log('the state is the one that keeps the files: the calculator resumes it instead of booting');
}

/* ---- the file list ---- */

function renderList() {
  ui.list.innerHTML = '';
  for (const entry of state.files) {
    const row = document.createElement('li');
    const name = document.createElement('span');
    name.className = 'file-name';
    name.textContent = entry.name;
    const status = document.createElement('span');
    status.className = 'file-status';
    status.textContent = entry.status || `${entry.file.size.toLocaleString()} bytes`;
    row.append(name, status);
    ui.list.append(row);
  }
  ui.files.textContent = state.files.length
    ? `${state.files.length} file${state.files.length === 1 ? '' : 's'} queued`
    : 'Choose or drop files';
}

function addFiles(list) {
  for (const file of list) {
    state.files.push({ name: file.name, file, status: '' });
  }
  renderList();
}

/* ---- wiring ---- */

function hookDrop(element, elementName, onFiles) {
  const input = element.querySelector('input[type=file]');
  input.addEventListener('change', () => {
    if (input.files && input.files.length) {
      onFiles([...input.files]);
      if (input.id === 'os-input') {
        ui.os.textContent = input.files[0].name;
      }
    }
  });
  element.addEventListener('dragover', (event) => {
    event.preventDefault();
    element.classList.add('over');
  });
  element.addEventListener('dragleave', () => element.classList.remove('over'));
  element.addEventListener('drop', (event) => {
    event.preventDefault();
    element.classList.remove('over');
    if (event.dataTransfer.files.length) {
      onFiles([...event.dataTransfer.files]);
      if (input.id === 'os-input') {
        ui.os.textContent = event.dataTransfer.files[0].name;
      }
    }
  });
  void elementName;
}

async function main() {
  hookDrop(document.getElementById('os-drop'), 'os', (files) => {
    state.os = files[0];
    state.osName = files[0].name;
    ui.build.disabled = !state.module;
    log(`operating system: ${files[0].name}`);
  });
  hookDrop(document.getElementById('files-drop'), 'files', (files) => {
    addFiles(files);
    log(`${files.length} file${files.length === 1 ? '' : 's'} added`);
  });
  ui.build.addEventListener('click', build);

  log('loading the emulator');
  state.module = await createT89({ locateFile: (name) => `dist/${name}` });
  log('the emulator is ready');
  ui.build.disabled = !state.os;
}

main().catch((error) => log(`could not start: ${error.message}`));

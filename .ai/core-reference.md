# TINY89 TECHNICAL REFERENCE

> **Project:** Tiny89
> **Purpose:** The authoritative lookup for externally defined facts this project depends on: TI-89 Titanium hardware and behaviour, the Tang Console board and GW5AST-138 FPGA, the TinyTang core interface, external implementations used as references, and the behaviour of the build tools.
> **Authority:** Per `.ai/core.md`, consult this file before looking anything up online, and open the record behind any hardware fact before acting on it. A record is authoritative to the extent of its `status` and cited `sources`. When a better or newer source is found online, add or revise a record and notify the user.

This file records what an outside source (a datasheet, schematic, vendor SDK, vendor tool, or external repository) says, plus hardware observations that confirm or refute it. It does not record project history, build results, or discussion; those belong in `core-log.md`. It does not record project design choices unless an external fact forces them.

The board, its FPGA and AE350, the SDRAM, the HDMI path, the open toolchain and TinyTang's contracts are already recorded in TinyNES's `.ai/core-reference.md` (https://github.com/aquasock/TinyNES), TinyCreature's (https://github.com/aquasock/TinyCreature), and TinyTang's (https://github.com/aquasock/TinyTang). They are cited from here by repository, commit and record id rather than copied. TI-89 Titanium facts are not inherited from those projects and receive their own records only after their primary sources have been opened.

---

## 1. AI operating rules

```yaml
schema_version: 1

record_kinds:
  - CALCULATOR  # TI-89 Titanium model identity, revisions and fitted hardware
  - CPU         # Motorola 68000 architecture and calculator-visible CPU behaviour
  - ASIC        # TI custom logic, address decoding, timers, interrupts and buses
  - MEMORY      # ROM, RAM, flash and memory maps
  - DISPLAY     # LCD controller, raster format, timing and grayscale behaviour
  - INPUT       # Keyboard matrix and other calculator inputs
  - LINK        # Calculator link port and USB interface
  - DEVICE      # FPGA silicon: part, revision, identification and hard blocks
  - BOARD       # Tang Console wiring, fitted parts, clocks, memory and video
  - PROTOCOL    # BL616 to loaded-FPGA-core contracts
  - EXTERNAL    # Interface contracts owned by other projects
  - TOOLCHAIN   # Yosys, nextpnr, Apicula, Verilator and vendor-tool behaviour

statuses:
  VERIFIED:   "Primary source cited AND confirmed on this project's hardware or tools."
  SOURCED:    "Primary source cited; not yet confirmed on this project's hardware or tools."
  INFERRED:   "Derived from secondary sources, reverse inspection, or consistent observation; no primary source. Flag before relying on it."

rules:
  - "One fact or one tightly coupled fact group per record."
  - "Every record cites at least one source with enough detail to re-check it: repository path plus commit, file, symbol, register name, document revision, or tool version."
  - "Separate what the source says (statement) from what it means for this project (consequence)."
  - "Diagnostic or implementation limits chosen by this project are never recorded as standard or device limits."
  - "A record holds what is true now. When a fact changes or proves wrong, edit the record in place: revise statement and consequence, update verified_date and sources, and end verification with 'Revised <date> (core-log entry N): <what changed>'. Git and core-log.md keep the history; this file does not."
  - "Before acting on a record, check that the code paths, document revisions and symbols it cites still exist; fix the record in the same cycle if they do not."
  - "Never reuse a record_id. When a record is folded into another, or split, move its id to the retired-id table in section 8 with the record that now holds its facts, after checking that every fact still true has been carried across."
  - "An INFERRED record must say how it was inferred, in verification."
  - "A negative result is a fact. Record what the hardware cannot do with the same rigour as what it can, and say how many attempts established it. An open defect may be recorded as its established facts and ruled-out causes, with any lead labelled as a lead."
  - "When an online lookup was needed because this file had no answer, add the finding here with the existing syntax, then notify the user if the source is newer or more valid than a record already here."
  - "Use technical names (TI-89 Titanium, MC68000, GW5AST-138, BL616, CDC, OTG). Conversational shorthand is not used in this file."
```

---

## 2. Topic catalog

Topic IDs are the `record_id` prefix. An entry reserves a name; it does not claim coverage.

```yaml
- topic_id: CALC
  name: "TI-89 Titanium hardware revisions"
  description: "The calculator model, board and ASIC revisions, clocks, fitted devices and differences from other TI-89-family calculators."

- topic_id: CPU
  name: "Motorola 68000-compatible CPU"
  description: "The processor architecture, exception model, bus behaviour and calculator-specific CPU integration."

- topic_id: ASIC
  name: "TI calculator ASIC and peripherals"
  description: "Address decoding, interrupt control, timers, power management and other custom logic visible to software."

- topic_id: MEM
  name: "Calculator memory and persistence"
  description: "ROM, RAM and flash devices, address maps, banking, write rules and persistent state."

- topic_id: LCD
  name: "LCD and grayscale behaviour"
  description: "The native display, framebuffer representation, controller timing and software-visible grayscale effects."

- topic_id: KEY
  name: "Keyboard matrix"
  description: "Key locations, matrix scanning and electrical or software-visible input behaviour."

- topic_id: IO
  name: "Link, USB and external I/O"
  description: "The calculator link port, USB controller and externally observable transfer behaviour."

- topic_id: OS
  name: "TI operating system and file formats"
  description: "Boot expectations, executable and variable formats, ROM image handling and compatibility requirements."

- topic_id: EMU
  name: "Emulator implementation references"
  description: "External emulator implementations, reusable architecture evidence and the boundary between observed implementation behaviour and calculator specifications."

- topic_id: PLAT
  name: "Tang Console 138K platform and TinyTang integration"
  description: "The shared board, FPGA, memory, video, core-link and deployment facts owned and measured by TinyNES, TinyCreature and TinyTang."

- topic_id: TEST
  name: "Conformance and differential testing"
  description: "Reference traces, test programs, observed calculator behaviour and rules for comparing the implementation with hardware."

- topic_id: PROV
  name: "Provenance and attribution"
  description: "External reference designs, licences, notices and boundaries between reading a specification and adapting covered source."
```

---

## 3. Active routing

| Question | Consult first | Fast records |
|---|---|---|
| Which TI-89 Titanium hardware revision is the compatibility target? | CALC | No live record; obtain a primary source before deciding |
| Which CPU behaviours and exceptions must be reproduced? | CPU | No live record; obtain the architecture manual and calculator evidence |
| What is the calculator memory map? | ASIC, MEM | No live record; obtain primary hardware documentation before implementation |
| How do LCD timing and grayscale work? | LCD | No live record; obtain primary documentation and hardware observations |
| How is the keyboard scanned? | KEY | No live record; obtain a schematic or equivalent primary record |
| What do the link and USB interfaces expose? | IO | No live record; obtain primary interface documentation |
| Which emulator implementation is the current software base? | EMU | EMU-001 |
| What does the supplied TI-89 Titanium OS image contain, and has it booted? | OS | OS-001 |
| What board, FPGA, memory, clock and HDMI facts may Tiny89 rely on? | PLAT | PLAT-001 |
| How does a loaded core communicate with and deploy through TinyTang? | PLAT | PLAT-002 |
| What measured AE350/fabric costs may inform an optional split implementation? | PLAT | PLAT-003 |
| Which HDL reference conventions and licence boundary apply? | PROV | PROV-001 |
| How should a new fact be recorded? | (this file) | Section 6 |

---

## 4. Fast lookup index

One line per live record, in the order of section 5. Retired ids are in section 8.

```yaml
OS-001: "The private TI-89 Titanium 3.10 upgrade image converts deterministically to a 4 MiB raw image and has booted to the Apps desktop on Tiny89's AE350/TiEmu build"
EMU-001: "TiEmu 2b8df92 is Tiny89's current GPL-2.0 emulator implementation base; its generated MC68000 core and Titanium hardware path compiled for RV32IMAC and booted the private OS on the Tang"
EMU-002: "Graph89 659c8da is architecture evidence that TiEmu can be hosted headlessly on ARM; no Graph89 source is incorporated into Tiny89"
PLAT-001: "TinyNES f5011dd is the measured source to consult for this board's GW5AST-138 identity, AE350 and clock behaviour, SDRAM, HDMI path, controller-port wiring and open-flow timing limits; Tiny89 does not duplicate those records"
PLAT-002: "TinyNES f5011dd and TinyTang 6145683 are the source of the BL616-to-core frame, UART, text/desk-layer and deployment contracts; implementation source remains subject to each repository's licence"
PLAT-003: "TinyCreature 3ba2a9b measured its own Tang32 AE350 EXTS window and access costs; those figures are useful architecture evidence only for a matching design and are not Tiny89 or device limits"
PROV-001: "TinyTang PROV-003 records CERN colibri and the unofficial colibri-sv port as CERN-OHL-W-2.0 references; Tiny89 takes conventions and ideas only unless the user first approves an adaptation"
PROV-002: "Tiny89 combines GPL-2.0 TiEmu and UAE code with MIT-derived TinyNES platform code; the private TI OS image is excluded from the repository and distribution"
```

---

## 5. Records

Records are grouped in the order of the topic catalog, and numbered within each topic.

```yaml
- record_id: OS-001
  kind: EXTERNAL
  topic_id: OS
  title: "The supplied TI-89 Titanium OS 3.10 image boots on Tiny89"
  status: VERIFIED
  verified_date: 2026-10-15
  statement: "The user-supplied `TI89Titanium_OS.89u` is a TI flash-upgrade container with one basecode payload of 1,399,943 bytes, release date 2005-07-18, embedded version 3.10, hardware id 9 and ROM base 0x800000. The input is 1,400,021 bytes with SHA-256 39440a0864b936520026fc674f9b35260405d8696fcfcc7a2770b6f79b6a80ee. Tiny89's converter places the payload in a 4 MiB erased image and produces SHA-256 c88b453b9aa3df4d897bd9e0e410e277e9d6bbdaa34cd361f76064a53e5bd13f. That image reached the TI-89 Apps desktop on the Tang Console through the AE350-hosted TiEmu core."
  consequence: "This exact private image is the first-cycle boot target and a proven regression input. Keep the `.89u` and generated raw image outside Git and distribution; reproduce the raw image locally with `tools/convert_89u.py`. The successful boot proves this implementation path for this image, not general TI-89 Titanium compatibility."
  sources:
    - "User-supplied `/run/media/vash/GIT/TI89Titanium_OS.89u`, identified and hashed locally on 2026-10-15"
    - "Tiny89 `tools/convert_89u.py`, `tests/test_convert_89u.py` and `docs/tiemu-ae350-port.md`"
  verification: "The converter's three unit tests passed; its raw output matched the expected digest; the AE350 firmware loaded the image, reported PC 0x00821e46 and LCD FNV-1a 0c0df297 after ten million-instruction slices; and the user reported that the TI-89 Apps desktop appeared on the Tang on 2026-10-15 (core-log entry 2)."

- record_id: EMU-001
  kind: EXTERNAL
  topic_id: EMU
  title: "TiEmu is the current headless emulator implementation base"
  status: VERIFIED
  verified_date: 2026-10-15
  statement: "TiEmu commit 2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5 provides the generated UAE MC68000 interpreter and TI-89 Titanium hardware implementation used by Tiny89's first AE350 build. Its `README.linux` designates GPL version 2 and `src/core/uae/COPYING` contains GPL version 2; `src/core/images.c` implements the flash-upgrade-to-ROM layout, `src/core/mem_size.h` defines the 4 MiB ROM and 256 KiB RAM maxima used by the retained core, and `src/core/ti_hw/hw.c` selects the calculator hardware path."
  consequence: "Pin this commit for reproducible builds and treat TiEmu's behaviour as implementation evidence rather than an authoritative calculator specification. Tiny89 may retain and adapt the covered core under GPL-compatible project terms with attribution in `THIRD_PARTY.md`. Validate calculator-visible behaviour independently as primary documentation or hardware evidence becomes available."
  sources:
    - "debrouxl/tiemu commit 2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5, `README.linux`, `COPYING`, `src/core/images.c`, `src/core/mem_size.h`, `src/core/ti_hw/hw.c` and `src/core/uae/COPYING`"
    - "https://github.com/debrouxl/tiemu/tree/2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5"
    - "Tiny89 `tools/fetch-tiemu.sh`, `tools/probe-rv32.sh`, `tools/build-host-probe.sh`, `tools/build-ae350-probe.sh` and `docs/tiemu-ae350-port.md`"
  verification: "The pinned source was fetched and its retained core built and ran on the host; its generated CPU sources compiled for RV32IMAC; the complete AE350 ELF linked and was loaded through TinyTang; and the resulting core booted OS-001 to the Apps desktop on 2026-10-15 (core-log entry 2)."

- record_id: EMU-002
  kind: EXTERNAL
  topic_id: EMU
  title: "Graph89 demonstrates a headless TiEmu hosting architecture"
  status: SOURCED
  verified_date: 2026-10-15
  statement: "Graph89 commit 659c8da89699cb7acecb394ea8d79d69dc9474bb integrates TiEmu-derived calculator emulation behind an Android-native host boundary, providing architecture evidence that the emulator can run without its original GTK desktop application on an ARM host."
  consequence: "Graph89 is a design reference only. Tiny89 incorporates no Graph89 source and derives its headless host boundary directly against the pinned TiEmu core. Re-check Graph89's licences and notices before any future source adaptation."
  sources:
    - "milaq/graph89 commit 659c8da89699cb7acecb394ea8d79d69dc9474bb"
    - "https://github.com/milaq/graph89/tree/659c8da89699cb7acecb394ea8d79d69dc9474bb"
  verification: "The pinned Graph89 repository structure and its TiEmu integration were inspected locally during the first build cycle. No Graph89 code was copied, and its architecture was not needed to verify the deployed binary, so this record remains SOURCED."

- record_id: PLAT-001
  kind: BOARD
  topic_id: PLAT
  title: "The shared Tang Console platform record is owned by TinyNES"
  status: SOURCED
  verified_date: 2026-10-15
  statement: "TinyNES at commit f5011dd374cfc65f4f17616fec97c903a66b6cc8 records and hardware-verifies the Tang Console's GW5AST-138 identity in DEV-001; AE350 clock source and measured rate in DEV-008; clock-plane routing requirements in DEV-009 and DEV-017; cacheable RAM-port behaviour in DEV-010 through DEV-013; fitted SDRAM module and FPGA pins in BRD-010; HDMI pins and clocking in BRD-011; FPGA-connected USB-A controller ports in BRD-003; and the boundary of the open flow's static timing coverage in TOOL-023. Those records cite their primary sources and hardware runs."
  consequence: "Before Tiny89 chooses or changes device constraints, clocks, external memory, HDMI output, controller-port logic or timing assumptions, open the cited TinyNES record and re-check its named files and revisions. Cite the TinyNES record rather than copying its values here, unless TinyNES ceases to be a live source or Tiny89 measures a project-specific difference. The target part and revision remain governed by Tiny89's `.ai/core.md`."
  sources:
    - "aquasock/TinyNES commit f5011dd374cfc65f4f17616fec97c903a66b6cc8, .ai/core-reference.md: DEV-001, DEV-008 through DEV-013, DEV-017, BRD-003, BRD-010, BRD-011 and TOOL-023"
    - "https://github.com/aquasock/TinyNES/tree/f5011dd374cfc65f4f17616fec97c903a66b6cc8"
  verification: "The cited records and commit were opened locally on 2026-10-15. Tiny89 subsequently ran its AE350 build after the inherited 32 MiB SDRAM self-test passed and displayed the emulated calculator through the inherited video path. Tiny89 did not independently reverify every board and toolchain fact routed by this record, so it remains SOURCED."

- record_id: PLAT-002
  kind: PROTOCOL
  topic_id: PLAT
  title: "TinyTang core-link and deployment contracts are established in TinyNES and TinyTang"
  status: SOURCED
  verified_date: 2026-10-15
  statement: "TinyNES at f5011dd records the BL616-to-loaded-core frame format, command set, UART1 rate and text page in PROT-001 through PROT-004; its PROT-010 records the TinyTang desk-layer frames and toggles, EXTCTL-001 through EXTCTL-005 record the optional versioned register, validated block-write and credit-stream channels and their serialization rule, and TOOL-022 records conversion and two-wire deployment of an open-flow core through TinyTang. TinyTang at 61456831652630afe87e56403dc5e9f0c1265b1c is the live firmware source behind those host-side contracts."
  consequence: "A Tiny89 core intended to behave as a TinyTang-loaded core should design against these named contracts and should keep compatibility logic separate from calculator emulation. The extended channels are properties of a cooperating FPGA core, not automatic board features. Reading a protocol as a specification does not authorize copying implementation source; inspect each source file's licence and preserve attribution before reuse."
  sources:
    - "aquasock/TinyNES commit f5011dd374cfc65f4f17616fec97c903a66b6cc8, .ai/core-reference.md: PROT-001 through PROT-004, PROT-010, EXTCTL-001 through EXTCTL-005 and TOOL-022"
    - "aquasock/TinyTang commit 61456831652630afe87e56403dc5e9f0c1265b1c, .ai/core-reference.md and the source paths cited by those records"
    - "https://github.com/aquasock/TinyTang/tree/61456831652630afe87e56403dc5e9f0c1265b1c"
  verification: "The cited records and repository commits were opened locally on 2026-10-15. Tiny89's AE350 ELF was transferred through TinyTang and its UART result was captured successfully, confirming the deployment subset used in the first build cycle. The other routed protocol contracts were not exercised, so the record remains SOURCED."

- record_id: PLAT-003
  kind: DEVICE
  topic_id: PLAT
  title: "TinyCreature's measured AE350-to-fabric evidence is conditional, not a platform limit"
  status: SOURCED
  verified_date: 2026-10-15
  statement: "TinyCreature at commit 3ba2a9b8f1715d892d14fea5cf3ff5c39389d93d records in AE-001 that its Tang32 core reached fabric registers in the tested EXTS window and in AE-002 that it measured A25 cycle costs for uncached EXTS accesses and cacheable SDRAM accesses in that specific core at its stated clocks. Its TOOL-001 records that the tested Yosys build did not infer GW5A DSP primitives for multiplication. Each record limits its claim to the named design, build and measurements."
  consequence: "These measurements are useful if Tiny89 evaluates an AE350/software plus FPGA-peripheral split, but they cannot be treated as TI-89 timing, universal GW5AST limits or predictions for a different interconnect. Re-open the TinyCreature records, reproduce the relevant path in Tiny89, and record Tiny89's own measurements before making an architecture decision from them."
  sources:
    - "aquasock/TinyCreature commit 3ba2a9b8f1715d892d14fea5cf3ff5c39389d93d, .ai/core-reference.md: AE-001, AE-002 and TOOL-001"
    - "https://github.com/aquasock/TinyCreature/tree/3ba2a9b8f1715d892d14fea5cf3ff5c39389d93d"
  verification: "The complete TinyCreature reference and the cited commit were opened locally on 2026-10-15. Tiny89 has not reproduced the measurements, so the status is SOURCED."

- record_id: PROV-001
  kind: EXTERNAL
  topic_id: PROV
  title: "CERN colibri and colibri-sv are references under CERN-OHL-W-2.0"
  status: SOURCED
  verified_date: 2026-10-15
  statement: "TinyTang record PROV-003 at commit 61456831652630afe87e56403dc5e9f0c1265b1c identifies CERN colibri as a vendor-independent VHDL common library and kavierim/colibri-sv as its unofficial SystemVerilog port, with both covered by CERN-OHL-W-2.0 and the port pinned to an upstream colibri revision. The same record distinguishes the port's conventions and free-tooling material from CERN's authoritative intent."
  consequence: "Per Tiny89's `.ai/core.md`, use colibri-sv as a good-practice reference for ideas and conventions only. Do not copy, translate or closely port a module. If adaptation is justified, stop for user approval first, then follow CERN-OHL-W-2.0 and add the required notice to `THIRD_PARTY.md`. Re-open the primary colibri-sv licence and notice before any such action."
  sources:
    - "aquasock/TinyTang commit 61456831652630afe87e56403dc5e9f0c1265b1c, .ai/core-reference.md: PROV-003"
    - "https://github.com/aquasock/TinyTang/tree/61456831652630afe87e56403dc5e9f0c1265b1c"
  verification: "TinyTang PROV-003 was opened locally on 2026-10-15. The colibri and colibri-sv primary repositories were not re-opened during this cycle, so the status is SOURCED."

- record_id: PROV-002
  kind: EXTERNAL
  topic_id: PROV
  title: "The first Tiny89 implementation has a GPL-compatible source boundary"
  status: VERIFIED
  verified_date: 2026-10-15
  statement: "Tiny89's retained emulator sources come from TiEmu and its bundled UAE core under GPL-2.0. The AE350 runtime and deployment patterns adapted from TinyNES are MIT-licensed. Tiny89 therefore distributes its combined source under GPL-2.0-only, preserves the relevant notices in `THIRD_PARTY.md`, and does not distribute the proprietary TI operating-system upgrade or its generated raw image."
  consequence: "Keep the repository and combined emulator work GPL-compatible, retain source-level SPDX and attribution notices, and audit every new external dependency before incorporation. Private `.89u`, `.img`, `.rom` and `.raw` files and generated build products remain ignored and must not enter commits or releases."
  sources:
    - "debrouxl/tiemu commit 2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5, `README.linux`, `COPYING` and `src/core/uae/COPYING`"
    - "aquasock/TinyNES commit f5011dd374cfc65f4f17616fec97c903a66b6cc8, `LICENSE`, `firmware/rt` and AE350 support sources"
    - "Tiny89 `LICENSE`, `THIRD_PARTY.md`, `.gitignore` and source-file notices"
  verification: "The cited licence texts, adapted-file provenance and repository ignore rules were inspected on 2026-10-15. `git status --ignored` and the staged-file audit confirmed that neither the private operating-system input nor generated images were selected for version control (core-log entry 2)."
```

---

## 6. Record template

```yaml
- record_id: <TOPIC>-<NNN>
  kind: CALCULATOR | CPU | ASIC | MEMORY | DISPLAY | INPUT | LINK | DEVICE | BOARD | PROTOCOL | EXTERNAL | TOOLCHAIN
  topic_id: <TOPIC>
  title: "Short noun phrase"
  status: VERIFIED | SOURCED | INFERRED
  verified_date: YYYY-MM-DD
  statement: "What the source says, with exact values and units"
  consequence: "What it means for this project, or where it is implemented"
  sources:
    - "Document or repository, revision/commit/path/symbol, URL"
  verification: "How and when it was confirmed, or why it is not yet confirmed; then any 'Revised <date> (core-log entry N): ...' notes"
```

---

## 7. Maintenance boundary

- Keep external facts, their sources, and their verification status here.
- Board, FPGA, AE350, SDRAM, HDMI, toolchain and TinyTang contract facts measured by TinyNES, TinyCreature or TinyTang are cited by repository, commit and record id rather than copied. Copy a record only if its owner stops being a live source or Tiny89 establishes a project-specific fact that belongs here.
- Keep build results, failures, rationale, plans and chronology in `core-log.md`; cite a log entry number in `verification` instead of repeating it.
- Do not add speculative records for topics that have not been looked up. An unanswered question stays out of section 5 until a source answers it; a reserved topic in section 2 is not a claim.
- Prefer a TI, Motorola/NXP, component-vendor or other primary source for calculator facts. Community documentation and emulator source may support an INFERRED record or point to a primary source, but must not silently become a standard.
- Keep this file current rather than historical: when a record's source is replaced by a newer document or tool version, or its fact changes, revise the record in place and let git and `core-log.md` carry the history.
- A `core-syntax.md` audit is required whenever this file changes, per `.ai/core.md`.

```yaml
last_reviewed: 2026-10-15
```

---

## 8. Retired record ids

None.

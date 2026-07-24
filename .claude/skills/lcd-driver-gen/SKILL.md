---
name: lcd-driver-gen
description: >
  Generate a low-level driver for a new LCD panel in the Realtek display repository.
  Strategy: "clone an existing driver + replace only the real variables":
  From device/general/lcd/<chip>/, pick a verified driver with the same interface as a template,
  replace model/resolution/pins/color-depth/init sequence,
  translate the vendor datasheet's init paragraphs into the target driver's representation style,
  and finally register in Kconfig / CMakeLists.txt / SConscript.
  Current scope: single-chip new drivers (choose one of 8773E / 8773G / 8762G).
  Trigger: user says "generate/add a panel driver", "add an LCD panel", "port a panel",
  "add st77916 driver for 8773G", "generate init sequence from datasheet", etc.
---

# lcd-driver-gen — Generate LCD Panel Low-Level Driver

Targets the current display repository, generating a single-panel driver that implements `rtk_lcd_hal_*`. Resolve repository files from the repository root and skill resources from this skill's base directory; do not hardcode machine-specific absolute paths.

**Core principle: never write register ops from scratch.** Clone the verified driver of the same interface; replace model, resolution, pins, color depth, init sequence.

## Parameter Collection Protocol (strictly follow this)

### Mandatory Rules

1. **Ask only 1 spec-dependent question per turn.** Simple short facts (model/resolution/interface/chip) can be asked together.
2. **Never offer "use a default placeholder" options.** Timing, pins, color depth, Group, colormap, and clock have no defaults.
3. **If the user does not answer a required parameter, do not proceed.** Re-ask it when the conversation resumes; never invent a placeholder value.
4. **Keep collected parameters together as task state.** If the task must continue across sessions, save or update one task-level project memory after each answer; do not create one memory file per parameter. Remove or supersede stale task state after generation.

The following 4 short facts can be asked in one AskUserQuestion:
- Panel model (e.g. `st77916`)
- Resolution (e.g. `360x360`)
- Interface type (QSPI / RGB / 8080 / SPI / RLSPI)
- Target chip (8773G / 8773E / 8762G)

### Round 1: Basic Info

Ask the 4 facts above together. Record answers, then proceed to Round 2.

### Round 2: Interface-Specific Parameters (one at a time, do NOT batch)

Branch by interface from Round 1. Each question uses a separate AskUserQuestion.

#### QSPI Branch

1. INPUT (framebuffer) bytes per pixel? → `INPUT_PIXEL_BYTES = 2/3/4`
2. OUTPUT (on-wire) bytes per pixel? → `OUTPUT_PIXEL_BYTES = 2/3`
3. Which QSPI Group? The valid choices depend on the target chip:
   - 8773G / 8773E → Group0 / Group1
   - 8762G → Group1 / Group2 / Group3
4. Quad or Octal data width? Only offer Octal for a group documented as Octal-capable.
5. TE needed? → pin number or `TE_VALID=0`
6. RS/D-C pin needed? → pin number or not used
7. RST pin number?
8. BL or PWR_EN pin needed? → pin number or not used
9. Max SCLK from AC Timing table? (required); typical (optional)
10. Does the panel require an initialization command sequence? If yes, obtain the exact sequence; do not assume any command is universally required.
11. Select reference driver → skill recommends closest-resolution, **wait for confirmation**

#### RGB Branch

1. INPUT format? → `LCDC_INPUT_RGB565` or `LCDC_INPUT_RGB888`
2. OUTPUT format? → `RGB565` or `RGB888`
   - If 565 → also ask which colormap: `EDPI_PIXELFORMAT_RGB565_1` / `_2` / `_3`
   - If 888 → colormap `EDPI_PIXELFORMAT_RGB888`
3. Pin group, according to the target chip:
   - 8773G / 8773E → Group0 / Group1
   - 8762G → fixed RGB port with `LCDC_GroupSel=1`; do not ask the user to choose Group0/Group1
4. RST pin number?
5. BL pin needed? → pin number or not used
6. Max PCLK from AC Timing? (required); typical (optional)
7. **6 timing values**: HSA / HBP / HFP + VSA / VBP / VFP (from RGB/DPI Timing table)
8. Does the panel require an initialization command sequence? If yes, ask whether it uses a separate SPI interface, then collect the exact sequence and user-specified SPI peripheral/pins.
9. Select reference driver → skill recommends closest-resolution, **wait for confirmation**

#### 8080 / DBIB Branch

1. INPUT (framebuffer) bytes per pixel? → `INPUT_PIXEL_BYTES = 2/3/4`
2. OUTPUT (on-wire) bytes per pixel? → `OUTPUT_PIXEL_BYTES = 2/3`
3. Data bus width and target-chip pin group? Only offer combinations documented in `references/pins-<chip>.md`.
4. TE needed? → pin number or `TE_VALID=0`
5. DCX/RS, RST, and optional RD pin assignments?
6. BL or PWR_EN pin needed? → pin number or not used
7. Max write clock/cycle from AC Timing? (required); typical (optional)
8. Does the panel require an initialization command sequence? If yes, obtain the exact sequence.
9. Select reference driver → skill recommends closest-resolution, **wait for confirmation**

#### SPI Branch

1. INPUT (framebuffer) bytes per pixel? → `INPUT_PIXEL_BYTES = 2/3/4`
2. OUTPUT (on-wire) bytes per pixel? → `OUTPUT_PIXEL_BYTES = 2/3`
3. SPI mode and wire format (for example 3-wire/9-bit or 4-wire with DCX)?
4. SPI peripheral and CLK/CS/data/DCX pin assignments?
5. TE, RST, and BL/PWR_EN pin assignments or not used?
6. Max SCLK from AC Timing? (required); typical (optional)
7. Does the panel require an initialization command sequence? If yes, obtain the exact sequence.
8. Select reference driver → skill recommends closest-resolution, **wait for confirmation**

#### RLSPI Branch

1. Confirm the panel has no GRAM and requires continuous full-frame streaming.
2. INPUT and on-wire pixel formats/bytes?
3. QSPI data width and group, restricted by target-chip pin documentation.
4. TE/VSYNC/HSYNC command requirements and exact command bytes?
5. RST and optional BL/PWR_EN pins?
6. Max SCLK from AC Timing? (required); typical (optional)
7. VSA / VBP / VFP and any additional timing values required by `references/ramless-qspi.md`?
8. Does the panel require an initialization command sequence? If yes, obtain the exact sequence and write opcode.
9. Select reference driver → skill recommends closest-resolution, **wait for confirmation**

Use `references/interface-map.md` and the target chip's pin document to validate every offered option.

## Main Flow (6 Steps)

1. **Collect Parameters**: Follow the protocol above. Keep all answers in one task-level state record when cross-session persistence is needed. Do NOT proceed until all required parameters are collected.
2. **Select Template Source**: Read `references/interface-map.md`, check target chip dir for existing `.c/.h` of same interface.
   - **Reference exists** → pick closest-resolution driver; note its init style (table-driven or inline).
   - **No reference** → read `references/code-skeletons.md`, take the interface+chip block, replace template variables.
3. **Convert Init Sequence**: Run `scripts/seq_convert.py`:
   ```
   python .claude/skills/lcd-driver-gen/scripts/seq_convert.py \
       --in <file or paste> --style <qspi-table|inline> --prefix <PANEL_MACRO>
   ```
   Review the exact command order, parameter counts, and every required delay against the panel specification. Do not insert, remove, or rewrite commands based on another panel's sequence.
4. **Rewrite the Driver**: Based on template/skeleton, replace:
   - Filename, header guard, includes, macros per `references/hal-contract.md` §1
   - Pin macros → `references/pins-<chip>.md`
   - `set_window` 0x2A/0x2B/0x2C (check x/y offset)
   - RGB timing → `references/rgb-timing.md`
   - Clock → `clock_calc.py --chip <chip> --iface <iface> --max <mhz> [--typical <mhz>]`; source per `references/clock-<chip>.md`
   - Reset → `references/reset-sequence.md`
   - Init sequence from step 3
   - HAL functions per `references/hal-contract.md` (§5 for RGB data path)
   - Register 3 places per `references/registration.md`
5. **Register in 3 Places**: Modify Kconfig, CMakeLists.txt, SConscript per `references/registration.md`. **Cross-check CONFIG_ symbol is byte-identical in all three** — historically bug-prone.
6. **Output the Checklist Below**: Present each item for manual confirmation.

## Self-Check Checklist (output after every generation)

- [ ] CONFIG symbol matches across Kconfig/CMakeLists/SConscript byte-identically
- [ ] HAL surface + `T_LCDC_TE_TYPE` enum = `references/hal-contract.md` spec (verbatim)
- [ ] `power_on/off` return `bool` — **do NOT copy template** (legacy mixes `bool`/`uint32_t`)
- [ ] `init` retains required INPUT/OUTPUT format configuration; `clear_screen` packs per INPUT. Panel commands come only from the confirmed panel-specific initialization sequence.
- [ ] Header macros: `<MODEL>_<W>_<H>_LCD_WIDTH/_LCD_HEIGHT` (infix mandatory); `_DRV_PIXEL_BITS` infix per interface; `INPUT/OUTPUT_PIXEL_BYTES`/`TE_VALID` only for GRAM, **not in RGB headers**
- [ ] `INPUT=3` → `LCD_WIDTH` 4-pixel-aligned or DMA misaligns. Don't copy deprecated `#error`
- [ ] QSPI: commands in `CMD_DESC.index`, `instruction=0x02`; SPI/8080/RGB send bytes directly
- [ ] Color depth changes only `OUTPUT_PIXEL_BYTES`; DMA `DataSize=Word` fixed; `Msize+DmaThreshold=128`
- [ ] Pins per target chip doc: 8773E (`pins-8773e.md`), 8773G QSPI (`pins-8773g-qspi.md`), 8773G RGB (`pins-8773g-rgb.md`), 8773G DBIB (`pins-8773g-dbib.md`), 8762G (`pins-8762g.md`)
- [ ] 8773G RGB: `INPUT`/`OUTPUT`/`eDPI_ColorMap` are independent; mixed formats valid (e.g. EK9716 INPUT=565/OUTPUT=888/colormap=888)
- [ ] RLSPI: no-GRAM confirmed; `RamlessEn=ENABLE`; struct/spec per `references/ramless-qspi.md`
- [ ] Clock ≤ panel spec max; source/selection per `references/clock-<chip>.md`
- [ ] eDPI: `eDPI_ClockDiv` = raw divisor (no ±1), min 2, bare integer
- [ ] RGB data path: `set_window/clear_screen/start_transfer` are empty `return;`; `transfer_done` clears `LCDC_CLR_WAVEFORM_FINISH` then returns; data = `<panel>_dma_init()` (LLI: block=1 row, sar_offset=2 rows) + `update_framebuffer` (first→init, later→`Infinite_Buf_Update`)
- [ ] RGB extra SPI: user-configured, skill leaves call sites only, **no hardcoded periph/pin**
- [ ] Reset: HIGH→LOW→HIGH, t3 ≥120ms before commands, power stable first
- [ ] `set_window` x/y offset needed? (some panels GRAM not at 0,0)
- [ ] Init seq ends with terminator (QSPI: `SEQ_FINISH_CODE`)
- [ ] Every command, parameter, and delay matches the confirmed panel-specific initialization sequence; no sequence was copied or inferred from another panel

## Reference Docs

- `references/hal-contract.md` — `rtk_lcd_hal_*` unified contract: functions, enum, macros, pixel format; §5 = RGB data path differences
- `references/code-skeletons.md` — Full code skeletons (3 interfaces × 3 platforms) for no-reference case
- `references/interface-map.md` — Interface → header → style → recommended drivers
- `references/registration.md` — Kconfig/CMakeLists/SConscript templates
- `references/pins-8773e.md` — 8773E pins: RGB/QSPI 2 groups each, 8080 Group0, CM=ADC3
- `references/pins-8773g-qspi.md` — 8773G QSPI 2 groups + GroupSel mapping
- `references/pins-8773g-rgb.md` — 8773G RGB 2 groups + colormap → D-line mapping
- `references/pins-8773g-dbib.md` — 8773G DBIB: Group0, 8-bit, RD# conflict
- `references/pins-8762g.md` — 8762G: RGB single port, QSPI 3 groups, 8080 3 groups, no HighSpeed*
- `references/clock-8773g.md` — 8773G clock: src 200M/280M, formulas
- `references/clock-8773e.md` — 8773E clock: src PLL1/PLL2/40M, unified RCC API
- `references/clock-8762g.md` — 8762G clock: src PLL1/PLL2/40M, BITS_324 bitfield
- `references/rgb-timing.md` — Timing params: names, acquisition, register formulas, bit-width limits
- `references/ramless-qspi.md` — RLSPI: no-GRAM, register mapping, init table (0xDE)
- `references/reset-sequence.md` — Reset waveform, safe defaults, edge cases
- `scripts/seq_convert.py` — Datasheet → normalized sequence → target style
- `scripts/clock_calc.py` — `--chip 8773g|8773e|8762g --iface qspi|rgb|dbib`: clock → register divider

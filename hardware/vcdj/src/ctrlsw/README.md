# VeriSilicon VC9000DJ (DecoderJ) Control Software — vendor drop (partial import)

Every file below this directory is a **byte-for-byte copy of a third-party vendor release**. It is
not Realtek code. Do not edit, reformat, refactor, re-indent or re-license anything here.

This is a **selective import**, not a copy of the whole release archive: only the files the firmware
build actually reads are imported. See section 2.

## 1. Source of truth

| Item | Value |
|---|---|
| Release archive | `Hantro_SW_DecoderJ_CtrlSW_Redistributable_0x2070_v4.1.84_CL1411136.tgz` |
| Archive SHA256 | `b3d08199df8464b07b6d58e82820e2503d34363fb2ca099dc512ca78770a9d12` |
| Software version | 4.1.84 |
| Hardware build ID | `0x2070` |
| Changelist | CL1411136 |
| Vendor | VeriSilicon Holdings Co., Ltd. (Hantro) |
| IP | VC9000DJ DecoderJ — JPEG and PNG hardware decoding only |

The archive above is the **only** authoritative baseline, and it is the recovery path for anything
not imported here. Any local git repository used to analyse the package is an analysis aid, not an
upstream dependency.

## 2. What is imported, and what is not

The archive was extracted with the leading `VC9000DJ_CtrlSW_V4.1.84/` path component stripped, then
reduced to the files the firmware build reads. No other transformation was applied to the files that
remain: no line ending conversion, no encoding conversion, no formatting, no header edits.

**Imported — 67 files, 42,281 lines:**

```text
software/source/inc/      12 files    7,728 lines
software/source/config/    1 file       342 lines   (deccfg.h)
software/source/common/   35 files   22,642 lines
software/source/jpeg/     10 files    8,281 lines
software/source/png/       9 files    3,288 lines
```

That is the 24 compiled `.c` files plus every header the compiler opens while building them, as
recorded by the build system's own dependency data (`ninja -t deps`), plus two headers kept
deliberately — see the note at the end of this section.

**Not imported — 108 files, 51,284 lines:**

| Dropped | Files | Lines | Why |
|---|---:|---:|---|
| `software/linux/dwl/` | 25 | 12,705 | Linux / PC-C-model DWL reference implementation. This SDK has no Linux target. Read it in the vendor archive when porting `src/common/vcdj_dwl.c`. |
| `software/linux/subsys_driver/`, `software/linux/memalloc/` | 2 | 238 | Linux kernel-module load scripts. Zero build or reference value, and the only files in the release carrying an explicit GPL-2.0 notice. |
| `software/test/` | 54 | 24,843 | PC command-line testbench (depends on SDL, libav, POSIX `getopt`). Not portable and not a reference for embedded integration. |
| `software/source/common/` non-compiled `.c` | 9 | 9,763 | `vcdecapi.c` multi-codec dispatcher, `film_grain.c`, `bqueue.c`, `workaround.c`, `tiledref.c`, `parselogmsg.c`, `stream_corrupt.c`, `sw_activity_trace.c`, `sw_performance.c` |
| Headers no compiled `.c` opens | 13 | 1,891 | e.g. `commonvp9.h`, `vcdecapi.h`, `ppcfg.h`, `film_grain.h`, and the other-OS OSAL headers `osal_linux.h` / `osal_win32.h` / `osal_freertos.h` |
| Vendor `Makefile` / `common.mk` | 5 | 1,844 | Vendor GNU-make build system. This tree is built by `CMakeLists.txt`. The macro values `common.mk` used to document are recorded in the build-contract document instead. |

Two headers are kept even though the compiler never opens them in this configuration, because a
compiled `.c` includes them behind a build macro and their absence would be a trap for anyone
enabling that macro:

| Header | Included by | Guard |
|---|---|---|
| `software/source/inc/error.h` | `error.c` | `ERROR_BUILD_SUPPORT` |
| `software/source/inc/vsi_string.h` | `input_queue.c` | `_USE_VSI_STRING_` |

Three Realtek-owned files live at the root of this directory and are **not** part of the vendor
release:

- `README.md` (this file)
- `SHA256SUMS` — per-file digests of the imported vendor files
- `CMakeLists.txt` — Zephyr/CMake build glue that selects the sources to compile

## 3. Verifying the copy has not been modified

`SHA256SUMS` lists all 67 imported vendor files. Verify with:

```sh
cd hardware/vcdj/src/ctrlsw
sha256sum -c SHA256SUMS
```

This proves every imported file is byte-identical to the release. It does **not** prove the import
is complete — completeness is section 2's table plus the build itself, which fails to compile or
link if anything needed is missing.

`SHA256SUMS` intentionally does not cover itself, `README.md` or `CMakeLists.txt`.

## 4. Upgrade procedure

1. Obtain a new official VeriSilicon release archive; record its file name and SHA256.
2. Delete the vendor tree (`software/`) — keep `README.md`, `SHA256SUMS`, `CMakeLists.txt`.
3. Extract the **whole** new archive with `--strip-components=1` into a scratch directory, not here.
4. Re-check the explicit source list in `CMakeLists.txt` against the new vendor
   `software/source/{common,jpeg,png}/Makefile`; added or renamed files will not be picked up
   automatically because the list is explicit, not globbed.
5. Copy the compiled `.c` files and build with the include path pointed at the scratch directory, so
   the compiler resolves headers from the full tree.
6. Take the build system's dependency data as the import list, for example
   `ninja -C <build> -t deps | grep -oE '<scratch>/software/[^ ]*\.(c|h)$' | sort -u`,
   then add back any macro-guarded headers per section 2.
7. Copy that file set here, regenerate `SHA256SUMS`:
   `find . -type f | LC_ALL=C sort | sed 's|^\./||' | grep -v -e '^README\.md$' -e '^SHA256SUMS$' -e '^CMakeLists\.txt$' | xargs sha256sum > SHA256SUMS`
8. Rebuild from scratch. Zero warnings and zero undefined references is the acceptance bar.
9. Update sections 1 and 2 of this file.

Never cherry-pick individual files from a different release. Replace the imported set as a whole.

## 5. What is actually compiled

All 24 `.c` files in this tree are compiled (22,232 lines); the remaining 43 files are headers.
The list is explicit in `CMakeLists.txt`, which also fails the CMake configure step if any listed
file is missing.

Note that `error.c` compiles to an empty object: its entire body is inside
`#ifdef ERROR_BUILD_SUPPORT`, which this build does not define. It is kept in the compile list
because the vendor `Makefile` lists it in `COMMON_SRCS`, so dropping it would be a deviation to
re-litigate on every upgrade, and it costs nothing.

The vendor subdirectories deliberately contain no `CMakeLists.txt`, so the recursive
`add_subdirectory` template used elsewhere in this repository skips them.

## 6. Licensing

Vendor headers were preserved byte for byte. Two license families are present in the imported set:

| Family | Files | Notice |
|---|---|---|
| VeriSilicon restricted redistribution | 12 files, all under `software/source/inc/` | `Copyright 2014-2025 Verisilicon Inc.` — redistribution permitted to SoC vendors that licensed the related VeriSilicon/Hantro core IP, limited to users who purchased the SoC product containing that IP |
| BSD-3-Clause style | 55 files | `Copyright (c) 2015 VeriSilicon Inc.`, `Copyright (c) 2011-2014 Google Inc.`, `Copyright (c) 2007-2010 Hantro OY` — 3-clause redistribution terms |

The release also contains GPL-2.0 / BSD-3-Clause dual-licensed Linux kernel-module load scripts.
**Those are not imported** (section 2), so no GPL-licensed file exists under this directory.

Consequences:

- This code is **not** covered by the `Apache-2.0` notice used by the rest of the `display`
  repository. Do not add, rewrite or normalise SPDX identifiers here.
- Do not delete or alter a single line of any vendor copyright or license block.
- The package is classified **VeriSilicon LEVEL B: CONFIDENTIAL — DISTRIBUTION RESTRICTED**.
  It must not be pushed to github, gitee or any other external hosting, and its contents must not
  be sent to external services.
- Import into the internal Gerrit `display` repository, internal verification use, and shipping as
  part of the Realtek SDK were confirmed as in scope by the `display` repository administrator on
  2026-08-27 (VC9000DJ IP purchased).

## 7. CI

The whole `hardware/vcdj/src/ctrlsw/` path is excluded from astyle and cppcheck/static analysis in
`.jenkins/ci-check-config.py`; the vendor code does not follow Realtek coding style and must not be
reformatted to fit it. `License-Scan` is left enabled — if it cannot classify the VeriSilicon
license, the fix belongs in the Jenkins-side configuration, not in these files.

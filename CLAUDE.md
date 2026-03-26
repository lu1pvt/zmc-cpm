# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ZMC (Z80 Management Commander) — a dual-panel Norton Commander-style file manager for CP/M systems on Z80 processors. Supports CP/M 2.2 and CP/M 3 (CP/M Plus). Written in C, compiled with z88dk.

## Build

Requires **z88dk** (ZCC compiler) installed and on PATH.

```sh
make            # builds zmc.com (requires z88dk on PATH)
./make.sh       # alternative build script with status feedback

# Docker build (no local z88dk needed):
docker run --rm -v $(pwd):/src -w /src z88dk/z88dk make
# On Apple Silicon, add: --platform linux/amd64
```

Output: `zmc.com` (CP/M executable). No test suite or linter exists.

CI: GitHub Actions workflow (`.github/workflows/build.yml`) builds on push/PR to main using the z88dk Docker image and uploads `zmc.com` as an artifact.

Compiler flags: `+cpm -O3 -vn -DAMALLOC -pragma-define:CRT_STACK_SIZE=1024 -Wall -create-app`

## Architecture

Four source files, one header (~1,400 lines total):

- **zmc.h** — All data structures (`FileEntry`, `Panel`, `AppState`), macros (screen dimensions, attribute flags, ASCII codes), CP/M directory format docs, and function declarations. Single source of truth for constants.
- **main.c** — Application loop, keyboard input dispatch, command-line parsing (TYPE, COPY, DEL, etc.), navigation, help display.
- **panel.c** — Panel rendering: file lists with box frames, file info lines, scroll-aware single-line redraws.
- **operations.c** — All file I/O via CP/M BDOS calls: directory loading/sorting, file copy/delete (single and batch), text viewer, hex dump viewer, FCB preparation, CP/M 3 timestamp conversion.
- **globals.c** — Global state, ANSI terminal escape sequences, BIOS keyboard input (inline Z80 assembly), screen utilities.

Data flow: `AppState` holds left/right `Panel` structs, each containing an array of `FileEntry`. The active panel pointer determines which panel receives input. BDOS calls go through FCB-based file operations in `operations.c`.

## Coding Conventions

- Use macros from `zmc.h` — no hardcoded screen dimensions, attribute values, or control characters.
- CP/M file operations use BDOS system calls (functions 14–22) via FCBs, not standard C file I/O.
- BIOS CONIO is used for keyboard input to bypass XON/XOFF handling.
- CP/M 3 features (timestamps, SCB screen detection) are conditionally used when available.
- UI rendering uses ANSI/VT100 escape sequences defined in `globals.c`.
- Improvements should be additive and respect previous logic. Prefer BDOS alternatives over hardware-specific calls for compatibility.
- Test on real hardware when possible.

## Workflow (from AGENT.md)

- Enter plan mode for non-trivial tasks (3+ steps or architectural decisions). Re-plan if things go sideways.
- Offload research and exploration to subagents to keep main context clean.
- Never mark a task complete without proving it works.
- For non-trivial changes, pause and ask "is there a more elegant way?" Skip for simple fixes.
- When given a bug: just fix it autonomously — point at logs/errors, then resolve.
- Simplicity first: make every change as simple as possible, minimal code impact, find root causes.

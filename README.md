======================================================================
         Z80 MANAGEMENT COMMANDER (ZMC) - Version 1.2
           "The Crystallized & Global Release"
              by Volney Torres & Martin H.R.
======================================================================

1. DESCRIPTION
--------------
ZMC is a dual-panel file manager inspired by Norton Commander,
specifically designed for CP/M systems based on Z80 processors.
Version 1.2 consolidates the code structure for superior speed and
global compatibility [cite: 2026-02-15].

2. WHAT'S NEW IN V1.2 (The Ho-Ro Update)
----------------------------------------
- CP/M 3 SUPPORT: Native display of file date and time stamps.
- ALPHABETIC SORTING: Fast QSort implementation for quick navigation.
- ATTRIBUTE MANAGEMENT: View Read-Only, System, and Backup flags.
- OPTIMIZED ENGINE: Drastically improved file size calculation for CP/M 2.2.
- MODULAR CORE: Separated into globals.c and operations.c for stability.
- TECHNICAL TRANSLATION: Standardized English UI and code comments.

3. KEYMAP (Updated)
-------------------
- [Arrows Up/Down] : Also Navigate the file list.
- [TAB]            : Switch active panel (A <-> B).
- [Space]          : Tag file for batch operations (*).
- [F1]             : Quick HELP and version credits.
- [F3 / F4]        : VIEW and DUMP modes with MORE/ESC support.
- [F5 / F8]        : Batch COPY and DELETE operations.
- [F10 / Ctrl+X]   : EXIT to system prompt.

The key handling suports the standard VT100 cursor and function keys as well as
the wordstar key bindings (^E = UP, ^X = DOWN, ^R = PAGEUP, ^C = PAGEDOWN).
ZMC allows also to enter commands in the prompt line "A> " followed by [CR],
i.e. HELP, VIEW (or TYPE or CAT), DUMP (or HEX), COPY (or CP), DEL (or ERA or RM),
EXIT (or QUIT or [ESC][ESC]).

4. TECHNICAL SPECIFICATIONS
---------------------------
- Compiler: z88dk (ZCC) with -O3 optimization [cite: 2026-02-10].
- Terminal: ANSI/VT100 (Full support for real hardware and emulators).
- Memory: Dynamic Heap management to support large directories.

5. INSPIRATION & CREDITS
------------------------
ZMC is a tribute to the legendary Norton Commander and Peter Norton.

CONTRIBUTORS:
- Volney Torres (lu1pvt): Original creator, UI design, and panel logic.
- Martin Homuth-Rosemann (Ho-Ro): Global refactoring, CP/M Plus support,
  and core algorithm optimization.

(c) 2025-2026 - Open Project for the CP/M Community and VCFed.
======================================================================

# ARMv7-M / Cortex-M7 Debug Lab — Review & Verification Report

**Status**: Manual/source review complete for documented architecture claims; generic source syntax was tested for an Arm-target compiler using a *compile-only CMSIS stub*; CRC calculation was exercised independently on the host. **NOT VERIFIED ON USER HARDWARE; NOT TESTED IN KEIL/ARMCC5; ETM DEMO IS CONDITIONAL.** User supplied no exact chip model, board, probe version or trace wiring. These gaps cannot honestly be declared passing.

## Iteration history

- **R0: re-check earlier explanation against the actual Cortex-M7 TRM**. Architecture C1.11 says the generic FPB may patch Flash, but Cortex-M7 DDI 0489F §9.3 explicitly says **no Flash patch / FP_REMAP RAZ-WI**. Corrected in tutorial Demo 4 and warning at start. M7 §13.2.3 says ETM data is discarded in SWO formatter-bypass mode; stopped equating SWO ITM printf with full ETM instruction trace. M7 §1.1.3 clarifies optional DWT/ITM/ETM and variable comparator counts. [M7 TRM, Arm official](https://documentation-service.arm.com/static/5e906dc68259fe2368e2abbe).
- **R1: audit register behaviors & fault diagnostics**. DDI 0403E.e §C1.4–C1.8 distinguishes halting debug, DebugMonitor, vector catch, W1C status, 32-bit CYCCNT and optional 8-bit performance counters; §B1.5 distinguishes precise from imprecise bus faults. Rewrote demos to avoid hand-writing DHCSR while CPU runs, guessing stacked PC with FPU/stack faults, or claiming DWT watchpoint sees DMA writes. [Armv7-M ARM, official](https://documentation-service.arm.com/static/606dc36485368c4c2b1bf62f); [M7 fault analysis with ABFSR](https://interrupt.memfault.com/blog/cortex-m-hardfault-debug).
- **R2: distinguish source-backed demo vs original teaching stimulus**. Demo 1 guided by Keil actual watchpoint article and BA command; Demo 2 by Arm/SEGGER/Memfault actual Fault tutorials; Demo 3 by official CYCCNT spec/SEGGER performance guide, with CRC payload designed specifically for the user's scenario; Demo 5 by SEGGER Reset Strategies; Demo 6 by Arm CMSIS ITM and SEGGER SWO; Demo 8 uses SEGGER's downloadable S32K3 M7 trace project rather than unverified invented ETM register initialization. Custom `debug_lab.c` is **new exercise code informed by these resources**, not a verbatim official example. Every detailed operating step has relevant authoritative reading beside it.
- **R3: build/test review**. Eight mutually exclusive modes were compiled with clang `--target=arm-none-eabi -mcpu=cortex-m7 -mthumb -std=c11 -O2 -Wall -Wextra -Werror` against a minimal *syntax-only* mock of the CMSIS identifiers (8/8 compile). The real CMSIS headers could not be downloaded by the sandbox's container (`Could not resolve host: raw.githubusercontent.com`), so this does **not** prove compatibility with user's installed SDK or Arm Compiler 5/6. The real CRC function was extracted from `debug_lab.c`, compiled with host GCC and checked against a standard vector and Python zlib: `"123456789" → 0xCBF43926`; generated 30 KiB pattern → `0x83159AF6`. These are **algorithm checks, not M7 timings**.

## 9-Demo acceptance matrix (user to verify on target)

| Demo | Manual or project basis | Static review | Live board result / required proof |
|---|---|---|---|
| 1. DWT Write Watchpoint | DDI0403E.e §C1.8; Keil apnt_236, BA WRITE | Source mode 1 compiled with stub | **Pending**: stop at CPU write; PC/disassembly screenshot |
| 2. Fault + Vector Catch | DDI0403E.e §B1.5, §C1.4–C1.6; Arm fault guide; Memfault fault guide | Mode 2 compiled with stub; expected UNDEFINSTR/FORCED checked in manual | **Pending**: CFSR, HFSR, stacked frame, exact PC |
| 3. 30KiB CRC cycles | DDI0403E.e §C1.8; M7 TRM §11; SEGGER performance guide | Mode 3 compiled with stub; CRC host tests pass | **Pending**: 5×cycles, actual clock/cache/memory |
| 4. FPB/step | M7 TRM §9.3; DDI0403E.e §C1.6, §C1.11 | Modes 4 and 8 compiled with stub | **Pending**: halt, disassembly, FPB count, core register view |
| 5. Reset Catch | DDI0403E.e §C1.4; SEGGER reset strategies | Mode 5 compiled with stub | **Pending**: stop at startup on selected device/reset mode |
| 6. ITM/SWO | DDI0403E.e §C1.7, §C1.10; CMSIS ITM; SEGGER SWO | Mode 6 compiled with stub | **Pending/conditional**: trace pin, probe, clock, T/R/E; distinguish accepted vs received |
| 7. DFSR | DDI0403E.e §C1.5/§C1.6; SEGGER fault guide | Mode 7 compiled with stub | **Pending**: fresh DFSR before debugger clears flags |
| 8. ETM | M7 TRM §13; ETM-M7 DDI0494D; SEGGER S32K3 example | Reference project inspected at source description level; hardware requirements explicit | **Conditional/not run**: supported M7 MCU+probe+route+trace export |
| 9. DAP/ROM IDs | M7 TRM §9.1, Appendix D1 / old Appendix A | Addresses and indicative ROM entries checked in M7 TRM | **Pending**: 32-bit AP/Memory reads, PID/CID, actual device topology |

## Coverage checklist — all titles from the screenshot

- [x] ARMv7-M Debug (parent)
- [x] C1.1 Introduction to ARMv7-M debug
- [x] C1.2 The Debug Access Port
- [x] C1.3 Overview of ARMv7-M debug features
- [x] C1.4 Debug and reset
- [x] C1.5 Debug event behavior
- [x] C1.6 Debug register support in the SCS
- [x] C1.7 The Instrumentation Trace Macrocell
- [x] C1.8 The Data Watchpoint and Trace unit
- [x] C1.9 Embedded Trace Macrocell support
- [x] C1.10 Trace Port Interface Unit
- [x] C1.11 Flash Patch and Breakpoint unit
- [x] Appendices (navigation heading)
- [x] ARMv7-M CoreSight Infrastructure IDs (appendix heading)
- [x] A.1 CoreSight infrastructure IDs for an ARMv7-M implementation (DDI0403E.e renumbered D1.1)

## Reproduce static checks

The `.verify/` directory is for author-side checks and **not included in the distribution ZIP**; its CMSIS stub must never be copied into a real firmware tree. Build checklist:

```sh
# Read actual device CMSIS headers from your Keil project, not from the stub.
# Rebuild eight modes separately with -O0 for debugging and -O2 for timing.
# Check the link map, debug symbols, HardFault_Handler symbol and device datasheet.
# Verify CPU features in J-Link's Log / Keil memory window.
# Record hardware screenshots and 5 CRC samples in the matrix above.
```

## Known limitations to be closed by user's board tests

1. Exact vendor chip/board was not provided; SoC trace pin mux, actual DWT/ITM feature presence, memory map and device protection were not verified.
2. `debug_lab.c` is a CMSIS drop-in source, not a standalone downloadable Keil `.uvprojx`. It needs existing startup/system-clock/linker/flash algorithms; the old 32-KiB-limited Keil linker may reject large static CRC test images, so use an appropriately licensed edition / supported toolchain.
3. Modes 2 and 5 deliberately cause fault/reset; run separately and restore firmware afterward. Compiler 5 inline `UDF` syntax was reviewed as a separate branch but not actually compiled with Arm Compiler 5.
4. The samples are **teaching stimuli** grounded in official debugging procedures, not claimed copies of ARM's tested reference projects. The SEGGER ETM example has its own chip/probe requirements and is not runnable on arbitrary M7 hardware.
5. This lab does not substantiate a measured time for CRC, RTOS starvation percentage, or SWO loss rate until target measurements are provided.

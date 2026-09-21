# ARMv7-M Cortex-M7 Debug Practical Lab

## Start here / clickable index

- [Main tutorial and 9 failure-driven demos](./armv7m_debug_lab.md)
  - [Preparation and requirements](./armv7m_debug_lab.md#setup)
  - [Demo 1: CPU data write watchpoint](./armv7m_debug_lab.md#demo-1)
  - [Demo 2: HardFault/stack diagnosis](./armv7m_debug_lab.md#demo-2)
  - [Demo 3: 30-KiB CRC performance](./armv7m_debug_lab.md#demo-3)
  - [Demo 4: FPB hardware breakpoint / single-step](./armv7m_debug_lab.md#demo-4)
  - [Demo 5: Reset Catch](./armv7m_debug_lab.md#demo-5)
  - [Demo 6: ITM/TPIU/SWO trace](./armv7m_debug_lab.md#demo-6)
  - [Demo 7: DFSR debug-event diagnosis](./armv7m_debug_lab.md#demo-7)
  - [Demo 8: ETM instruction history (requires compatible hardware)](./armv7m_debug_lab.md#demo-8)
  - [Demo 9: DAP / ROM tables / CoreSight IDs](./armv7m_debug_lab.md#demo-9)
  - [All eleven C1 topics and Appendix mapping](./armv7m_debug_lab.md#topic-coverage)
  - [References / Manuals](./armv7m_debug_lab.md#references)
- [Per-demo official manuals and verified practice source index](./source_index.md)
- [C source for eight selectable firmware modes](./debug_lab.c)
- [Source review, syntax-test results, gaps, acceptance matrix](./review_report.md)

**No pre-built or bench-tested firmware is included.** Add the source to an existing CMSIS M7 project; use a real device-specific reference manual and debugger configuration. To check a particular exercise, set `DEBUG_LAB_DEMO=1..8`, then rebuild. Modes 1-7 correspond to tutorial demos 1-7; mode 8 supplements demo 4. The ETM and DAP demos are debugger/project-guided, not build modes.

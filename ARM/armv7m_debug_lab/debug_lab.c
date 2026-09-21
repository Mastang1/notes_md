/* Cortex-M7 debug lab: drop this file into an EXISTING CMSIS Cortex-M7 project.
 * Set DEBUG_LAB_DEMO=1..8 in project compiler defines. Do not ship in production.
 * The containing project provides startup, linker, SystemInit and SystemCoreClock.
 * HardFault_Handler below REPLACES the startup's weak/default handler for demo 2.
 */
#include <stdint.h>
#include "core_cm7.h"

#ifndef DEBUG_LAB_DEMO
#define DEBUG_LAB_DEMO 1
#endif

extern uint32_t SystemCoreClock;
volatile uint32_t g_lab_stage;
volatile uint32_t g_lab_result;

#if DEBUG_LAB_DEMO == 1
/* Watchpoint: configure WRITE access to g_rx_index after first stopping at stage 1. */
volatile uint32_t g_rx_index;
void DebugLab_BadWriter(void) { g_rx_index = 0xFFFFFFFFu; }
void DebugLab_Run(void)
{
    g_rx_index = 0u;
    g_lab_stage = 1u;      /* Set a source breakpoint at the next __NOP(). */
    __NOP();
    DebugLab_BadWriter(); /* When resumed, DWT write watchpoint catches this. */
    g_lab_stage = 2u;
}

#elif DEBUG_LAB_DEMO == 2
/* Intentional fault: UDF -> UsageFault disabled -> escalates to HardFault. */
volatile uint32_t g_fault_cfsr, g_fault_hfsr, g_fault_dfsr, g_fault_abfsr;
void HardFault_Handler(void)
{
    g_fault_cfsr  = SCB->CFSR;
    g_fault_hfsr  = SCB->HFSR;
    g_fault_dfsr  = SCB->DFSR;
    g_fault_abfsr = SCB->ABFSR;  /* Cortex-M7-specific auxiliary status. */
    while (1) { __NOP(); }
}
static void DebugLab_UndefinedInstruction(void)
{
#if defined(__CC_ARM) && !defined(__clang__)
    __asm { UDF #0 }
#else
    __asm volatile ("udf #0");
#endif
}
void DebugLab_Run(void)
{
    SCB->SHCSR &= ~SCB_SHCSR_USGFAULTENA_Msk;
    __DSB();
    __ISB();
    g_lab_stage = 1u;
    DebugLab_UndefinedInstruction();
    g_lab_stage = 2u; /* Must never be reached. */
}

#elif DEBUG_LAB_DEMO == 3
/* CRC32/ISO-HDLC (reflected polynomial); DWT counter is 32-bit and wraps. */
#define LAB_CRC_LEN (30u * 1024u)
static uint8_t g_crc_data[LAB_CRC_LEN];
volatile uint32_t g_crc_cycles[5];
volatile uint32_t g_crc_value, g_crc_known_value, g_core_clock_hz;
static uint32_t DebugLab_Crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i, j;
    for (i = 0u; i < len; ++i) {
        crc ^= (uint32_t)data[i];
        for (j = 0u; j < 8u; ++j) {
            if ((crc & 1u) != 0u) crc = (crc >> 1) ^ 0xEDB88320u;
            else                   crc = crc >> 1;
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
void DebugLab_Run(void)
{
    uint32_t i, k, t0;
    static const uint8_t known[9] = { '1','2','3','4','5','6','7','8','9' };
    g_crc_known_value = DebugLab_Crc32(known, 9u); /* Must be 0xCBF43926. */
    if (g_crc_known_value != 0xCBF43926u) { g_lab_result = 1u; return; }
    for (i = 0u; i < LAB_CRC_LEN; ++i) g_crc_data[i] = (uint8_t)i;
    g_core_clock_hz = SystemCoreClock;  /* Ensure SystemCoreClockUpdate() ran. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) != 0u) {
        g_lab_result = 2u; return;      /* This implementation has no CYCCNT. */
    }
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    for (k = 0u; k < 5u; ++k) {
        __DSB(); __ISB();
        t0 = DWT->CYCCNT;
        g_crc_value = DebugLab_Crc32(g_crc_data, LAB_CRC_LEN);
        __DSB(); __ISB();
        g_crc_cycles[k] = (uint32_t)(DWT->CYCCNT - t0);
    }
    g_lab_result = 0u;
    g_lab_stage = 2u;  /* View five raw cycle counts in debugger. */
}

#elif DEBUG_LAB_DEMO == 4
/* FPB: set a source breakpoint inside DebugLab_TxSend(), do not program FPB by hand. */
volatile uint32_t g_tx_payload, g_tx_count;
void DebugLab_TxSend(uint32_t value)
{
    g_tx_payload = value;
    g_tx_count++;
}
void DebugLab_Run(void)
{
    g_tx_count = 0u;
    g_lab_stage = 1u;
    DebugLab_TxSend(0x12345678u);
    g_lab_stage = 2u;
}

#elif DEBUG_LAB_DEMO == 5
/* Reset catch: configure debugger reset-and-halt or VC_CORERESET first. */
void DebugLab_Run(void)
{
    g_lab_stage = 1u;
    NVIC_SystemReset();
    while (1) { __NOP(); }
}

#elif DEBUG_LAB_DEMO == 6
/* Non-blocking, lossy ITM port 0 event markers; debugger configures trace. */
uint32_t DebugLab_TryTraceChar(uint8_t byte)
{
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0u) return 0u;
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0u) return 0u;
    if ((ITM->TER & 1u) == 0u) return 0u;
    if (ITM->PORT[0].u32 == 0u) return 0u;
    ITM->PORT[0].u8 = byte;
    return 1u;
}
void DebugLab_Run(void)
{
    g_lab_result  = DebugLab_TryTraceChar((uint8_t)'T'); /* TX start */
    g_lab_result += DebugLab_TryTraceChar((uint8_t)'R'); /* RX IRQ */
    g_lab_result += DebugLab_TryTraceChar((uint8_t)'E'); /* TX end */
    g_lab_stage = 2u; /* g_lab_result counts accepted, not received bytes. */
}

#elif DEBUG_LAB_DEMO == 7
/* Debug event: inspect DFSR *while halted* at the BKPT; debugger may clear it. */
void DebugLab_Run(void)
{
    g_lab_stage = 1u;
    __BKPT(0);
    g_lab_stage = 2u;
}

#elif DEBUG_LAB_DEMO == 8
/* Inspect DHCSR / DCRSR+DCRDR indirectly via the IDE register window. */
volatile uint32_t g_lab_register_demo;
void DebugLab_Run(void)
{
    g_lab_register_demo = 0x11223344u;
    g_lab_stage = 1u;
    __NOP();            /* Halt here; inspect R0..R12, SP, LR, PC, xPSR. */
    g_lab_register_demo ^= 0xFFFFFFFFu;
    g_lab_stage = 2u;
}
#else
#error "DEBUG_LAB_DEMO must be an integer in [1, 8]"
#endif

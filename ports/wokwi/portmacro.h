/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Standard FreeRTOS type and task macros retained for the simulator port.
 * The interrupt and yield implementation uses PRIMASK and Thread-mode calls.
 */
#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <stdint.h>
#include "stm32f1xx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define portCHAR char
#define portFLOAT float
#define portDOUBLE double
#define portLONG long
#define portSHORT short
#define portSTACK_TYPE uint32_t
#define portBASE_TYPE long
typedef uint32_t StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if configUSE_16_BIT_TICKS == 1
typedef uint16_t TickType_t;
#define portMAX_DELAY ((TickType_t)0xffffU)
#else
typedef uint32_t TickType_t;
#define portMAX_DELAY ((TickType_t)0xffffffffUL)
#define portTICK_TYPE_IS_ATOMIC 1
#endif

#define portSTACK_GROWTH (-1)
#define portBYTE_ALIGNMENT 8
#define portTICK_PERIOD_MS ((TickType_t)1000U / configTICK_RATE_HZ)

void vPortYield(void);
void vPortEnterCritical(void);
void vPortExitCritical(void);
void vPortValidateInterruptPriority(void);

#define portYIELD() vPortYield()
#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL() vPortExitCritical()
#define portDISABLE_INTERRUPTS() __disable_irq()
#define portENABLE_INTERRUPTS() __enable_irq()

static inline uint32_t ulPortSaveInterruptMask(void) {
    const uint32_t previous_mask = __get_PRIMASK();
    __disable_irq();
    __DMB();
    return previous_mask;
}

static inline void vPortRestoreInterruptMask(uint32_t previous_mask) {
    __DMB();
    __set_PRIMASK(previous_mask);
    __ISB();
}

#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortSaveInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(mask) vPortRestoreInterruptMask(mask)
#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID() vPortValidateInterruptPriority()

/* The next Thread-mode yield observes any tasks readied by an ISR. */
#define portEND_SWITCHING_ISR(switch_required) do { \
    if ((switch_required) != 0) { vPortValidateInterruptPriority(); } \
} while (0)
#define portYIELD_FROM_ISR(switch_required) portEND_SWITCHING_ISR(switch_required)

#define portTASK_FUNCTION_PROTO(function, parameter) void function(void *parameter)
#define portTASK_FUNCTION(function, parameter) void function(void *parameter)
#define portNOP() __NOP()
#define portINLINE __inline
#define portFORCE_INLINE inline __attribute__((always_inline))
#define portMEMORY_BARRIER() __asm volatile("" ::: "memory")
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

static inline BaseType_t xPortIsInsideInterrupt(void) {
    return __get_IPSR() != 0U;
}

#ifdef __cplusplus
}
#endif
#endif

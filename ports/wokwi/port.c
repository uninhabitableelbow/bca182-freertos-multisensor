/*
 * Wokwi STM32F103 Thread-mode port for the unchanged FreeRTOS V10.3.1 kernel.
 *
 * Task switches are ordinary calls on PSP; interrupts continue to use MSP.
 * TIM3 advances the kernel at 100 Hz, without switching inside an exception.
 * This implements cooperative scheduling, not interrupt-driven preemption.
 *
 * Wokwi's missing NVIC-priority/SVC/PendSV behavior was independently reported
 * at github.com/monxx-ie/BCA182-freetos-multisensor. This port's context frame
 * and interrupt-state handling are implemented locally for this project.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"

#if configUSE_PREEMPTION != 0
#error "The Wokwi Thread-mode port requires cooperative scheduling"
#endif
#if configTICK_RATE_HZ != 100
#error "The Wokwi TIM3 timebase requires a 100 Hz RTOS tick"
#endif
#if configUSE_TICKLESS_IDLE != 0
#error "Tickless idle is not implemented by the Wokwi Thread-mode port"
#endif

extern void *volatile pxCurrentTCB;
extern __IO uint32_t uwTick;

static volatile UBaseType_t critical_nesting __attribute__((used));
static volatile uint32_t saved_outer_mask __attribute__((used));
static TIM_HandleTypeDef tick_timer;

static void prvSwitchContext(void) __attribute__((naked, noinline, used));
static void prvStartFirstTask(void) __attribute__((naked, noinline, used, noreturn));
static void prvTaskBootstrap(void) __attribute__((naked, noinline, used, noreturn));
static void prvTaskReturned(void) __attribute__((noinline, used, noreturn));

/*
 * A 48-byte, 8-byte-aligned frame:
 * [0..7] r4-r11; [8] LR; [9] critical nesting; [10] PRIMASK;
 * [11] the interrupt mask saved by the outermost critical-section entry.
 * Ordinary-call ABI rules allow r0-r3, r12, and flags to change across yield.
 */
StackType_t *pxPortInitialiseStack(StackType_t *top, TaskFunction_t entry,
                                 void *parameter) {
    top -= 12;
    for (unsigned int index = 0; index < 12; ++index) {
        top[index] = 0U;
    }
    top[0] = (StackType_t)entry;
    top[1] = (StackType_t)parameter;
    top[8] = (StackType_t)prvTaskBootstrap | 1U;
    return top;
}

static void prvTaskReturned(void) {
    configASSERT(!"A FreeRTOS task returned from its entry function");
    __disable_irq();
    for (;;) { __NOP(); }
}

static void prvTaskBootstrap(void) {
    __asm volatile(
        "mov r0, r5\n"
        "blx r4\n"
        "bl prvTaskReturned\n"
        "b .\n"
    );
}

static void prvSwitchContext(void) {
    __asm volatile(
        "mrs r12, primask\n"
        "cpsid i\n"
        "mrs r0, psp\n"
        "sub r0, r0, #48\n"
        "stmia r0, {r4-r11}\n"
        "str lr, [r0, #32]\n"
        "ldr r1, =critical_nesting\n"
        "ldr r2, [r1]\n"
        "str r2, [r0, #36]\n"
        "str r12, [r0, #40]\n"
        "ldr r1, =saved_outer_mask\n"
        "ldr r2, [r1]\n"
        "str r2, [r0, #44]\n"
        "msr psp, r0\n"
        "ldr r1, =pxCurrentTCB\n"
        "ldr r2, [r1]\n"
        "str r0, [r2]\n"
        "bl vTaskSwitchContext\n"
        "ldr r1, =pxCurrentTCB\n"
        "ldr r2, [r1]\n"
        "ldr r0, [r2]\n"
        "ldr r1, =critical_nesting\n"
        "ldr r2, [r0, #36]\n"
        "str r2, [r1]\n"
        "ldr r1, =saved_outer_mask\n"
        "ldr r2, [r0, #44]\n"
        "str r2, [r1]\n"
        "ldr r12, [r0, #40]\n"
        "ldr lr, [r0, #32]\n"
        "ldmia r0, {r4-r11}\n"
        "add r0, r0, #48\n"
        "msr psp, r0\n"
        "dsb\n"
        "isb\n"
        "msr primask, r12\n"
        "bx lr\n"
    );
}

static void prvStartFirstTask(void) {
    __asm volatile(
        "cpsid i\n"
        "ldr r1, =pxCurrentTCB\n"
        "ldr r2, [r1]\n"
        "ldr r0, [r2]\n"
        "ldr r1, =critical_nesting\n"
        "ldr r2, [r0, #36]\n"
        "str r2, [r1]\n"
        "ldr r1, =saved_outer_mask\n"
        "ldr r2, [r0, #44]\n"
        "str r2, [r1]\n"
        "ldr r12, [r0, #40]\n"
        "ldr lr, [r0, #32]\n"
        "ldmia r0, {r4-r11}\n"
        "add r0, r0, #48\n"
        "msr psp, r0\n"
        "movs r0, #2\n"
        "msr control, r0\n"
        "dsb\n"
        "isb\n"
        "msr primask, r12\n"
        "bx lr\n"
    );
}

void vPortYield(void) {
    configASSERT(__get_IPSR() == 0U);
    configASSERT((__get_CONTROL() & CONTROL_SPSEL_Msk) != 0U);
    prvSwitchContext();
}

void vPortEnterCritical(void) {
    configASSERT(__get_IPSR() == 0U);
    const uint32_t previous_mask = ulPortSaveInterruptMask();
    if (critical_nesting == 0U) {
        saved_outer_mask = previous_mask;
    }
    configASSERT(critical_nesting != (UBaseType_t)~0UL);
    ++critical_nesting;
}

void vPortExitCritical(void) {
    configASSERT(__get_IPSR() == 0U);
    configASSERT(critical_nesting > 0U);
    --critical_nesting;
    if (critical_nesting == 0U) {
        vPortRestoreInterruptMask(saved_outer_mask);
    }
}

void vPortValidateInterruptPriority(void) {
    /* PRIMASK excludes every IRQ; no simulated priority-width probe is used. */
    configASSERT(__get_IPSR() != 0U);
}

static void prvSetupTickTimer(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) {
        timer_clock *= 2U;
    }
    const uint32_t counter_frequency = 1000000U;
    configASSERT(timer_clock >= counter_frequency);
    configASSERT(timer_clock % counter_frequency == 0U);
    const uint32_t divider = timer_clock / counter_frequency;
    configASSERT(divider <= 65536U);

    tick_timer.Instance = TIM3;
    tick_timer.Init.Prescaler = divider - 1U;
    tick_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    tick_timer.Init.Period = counter_frequency / configTICK_RATE_HZ - 1U;
    tick_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tick_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    configASSERT(HAL_TIM_Base_Init(&tick_timer) == HAL_OK);
    __HAL_TIM_CLEAR_FLAG(&tick_timer, TIM_FLAG_UPDATE);
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(TIM3_IRQn);
    configASSERT(HAL_TIM_Base_Start_IT(&tick_timer) == HAL_OK);
}

void TIM3_IRQHandler(void) {
    HAL_TIM_IRQHandler(&tick_timer);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer) {
    if (timer->Instance == TIM3) {
        const uint32_t previous_mask = ulPortSaveInterruptMask();
        uwTick += 1000U / configTICK_RATE_HZ;
        (void)xTaskIncrementTick();
        vPortRestoreInterruptMask(previous_mask);
    }
}

BaseType_t xPortStartScheduler(void) {
    __disable_irq();
    critical_nesting = 0U;
    saved_outer_mask = 0U;
    SysTick->CTRL = 0U;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    prvSetupTickTimer();
    prvStartFirstTask();
}

void vPortEndScheduler(void) {
    configASSERT(!"The Wokwi scheduler cannot return to main");
}

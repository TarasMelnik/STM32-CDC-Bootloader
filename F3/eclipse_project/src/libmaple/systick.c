/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2010, 2011 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file libmaple/systick.c
 * @brief System timer (SysTick).
 */

#include "systick.h"
#include "delay.h"

volatile uint32 systick_uptime_millis;
static void (*systick_user_callback)(void);

/**
 * @brief Initialize and enable SysTick.
 *
 * Clocks the system timer with the core clock, turns it on, and
 * enables interrupts.
 *
 * @param reload_val Appropriate reload counter to tick every 1 ms.
 */
void systick_init() {
    SYSTICK_REGS->RVR = SYSTICK_RELOAD_VAL;
    systick_enable();
}

/**
 * Clock the system timer with the core clock, but don't turn it
 * on or enable interrupt.
 */
void systick_disable() {
    SYSTICK_REGS->CSR = SYSTICK_CSR_CLKSOURCE_CORE;
}

/**
 * Clock the system timer with the core clock and turn it on;
 * interrupt every 1 ms, for systick_timer_millis.
 */
void systick_enable() {
    /* re-enables init registers without changing reload val */
    SYSTICK_REGS->CSR = (SYSTICK_CSR_CLKSOURCE_CORE   |
                         SYSTICK_CSR_ENABLE           |
                         SYSTICK_CSR_TICKINT_PEND);
}

/**
 * @brief Attach a callback to be called from the SysTick exception handler.
 *
 * To detach a callback, call this function again with a null argument.
 */
void systick_attach_callback(voidFuncPtr callback) {
    systick_user_callback = callback;
}

/*
 * SysTick ISR
 */

__weak void __exc_systick(void) {
    systick_uptime_millis++;
    if (systick_user_callback) {
        systick_user_callback();
    }
}
//-----------------------------------------------------------------------------
__weak void yield() {}
//-----------------------------------------------------------------------------
void delay(uint32 ms)
{
    uint32 start = micros();
    while (ms > 0)
    {
        yield();
        while ( (ms > 0) && ((micros() - start) >= 1000) )
        {
            ms--;
            start += 1000;
        }
    }
}

//-----------------------------------------------------------------------------
void delayMicroseconds(uint32 us) {
    delay_us(us);
}

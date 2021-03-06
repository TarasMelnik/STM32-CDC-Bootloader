/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2012 LeafLabs, LLC.
 * Copyright (c) 2010 Perry Hung.
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
 * @file libmaple/stm32f1/usart.c
 * @author Marti Bolivar <mbolivar@leaflabs.com>,
 *         Perry Hung <perry@leaflabs.com>
 * @brief STM32F1 USART.
 */

#include "libmaple_types.h"
#include "usart.h"
#include "gpio.h"
#include "usart_private.h"

/*
 * Devices
 */

static RING_BUFFER(usart1_rb, USART_RX_BUF_SIZE);
static RING_BUFFER(usart1_wb, USART_TX_BUF_SIZE);
/** USART1 device */
const usart_dev usart1 = {
    .regs     = USART1_REGS,
    .rb       = &usart1_rb,
    .wb       = &usart1_wb,
    .clk_id   = RCC_USART1,
    .irq_num  = NVIC_USART1,
};
#if 0
static RING_BUFFER(usart2_rb, SERIAL_RX_BUFFER_SIZE);
static RING_BUFFER(usart2_wb, SERIAL_TX_BUFFER_SIZE);
/** USART2 device */
const usart_dev usart2 = {
    .regs     = USART2_BASE,
    .rb       = &usart2_rb,
    .wb       = &usart2_wb,
    .clk_id   = RCC_USART2,
    .irq_num  = NVIC_USART2,
};

static RING_BUFFER(usart3_rb, SERIAL_RX_BUFFER_SIZE);
static RING_BUFFER(usart3_wb, SERIAL_TX_BUFFER_SIZE);
/** USART3 device */
const usart_dev usart3 = {
    .regs     = USART3_BASE,
    .rb       = &usart3_rb,
    .wb       = &usart3_wb,
    .clk_id   = RCC_USART3,
    .irq_num  = NVIC_USART3,
};
#endif
#if defined(STM32_HIGH_DENSITY) || defined(STM32_XL_DENSITY)
static ring_buffer uart4_rb;
static ring_buffer uart4_wb;
static usart_dev uart4 = {
    .regs     = UART4_BASE,
    .rb       = &uart4_rb,
    .wb       = &uart4_wb,
    .max_baud = 2250000UL,
    .clk_id   = RCC_UART4,
    .irq_num  = NVIC_UART4,
};
/** UART4 device */
usart_dev *UART4 = &uart4;

static ring_buffer uart5_rb;
static ring_buffer uart5_wb;
static usart_dev uart5 = {
    .regs     = UART5_BASE,
    .rb       = &uart5_rb,
    .wb       = &uart5_wb,
    .max_baud = 2250000UL,
    .clk_id   = RCC_UART5,
    .irq_num  = NVIC_UART5,
};
/** UART5 device */
usart_dev *UART5 = &uart5;
#endif

/*
 * Routines
 */


void usart_config(const usart_dev *udev, uint8 rx, uint8 tx, uint8 flags)
{
/*
CR1 bit 12 Word length 0=8  1=9 
CR1 bit 11 wake (default value is 0) we can safely set this value to 0 (zero) each time 
CR1 bit 10 parity enable (1 = enabled)
CR1 bit 9  Parity selection 0 = Even  1 = Odd
CR2 bits 13 and 12 stop bits: 00 = 1 01 = 0.5 10 = 2 11 = 1.5
Not all USARTs support 1.5 or 0.5 bits so its best to avoid them.
When parity enabled the word length must be increased (CR1 bit 12 set).
Word length of 9 bit with parity is not supported.
*/
	udev->regs->CR1  = (udev->regs->CR1 & 0B1110000111111111) | ((uint32_t)(flags&0x0F)<<9);
	udev->regs->CR2  = (udev->regs->CR2 & 0B1100111111111111) | ((uint32_t)(flags&0x30)<<8);
	gpio_set_mode(rx, GPIO_AF_INPUT);
	gpio_set_mode(tx, GPIO_AF_OUTPUT);
	gpio_set_af(rx, GPIO_AF_USART1_2_3);
	gpio_set_af(tx, GPIO_AF_USART1_2_3);
}

void usart_set_baud_rate(const usart_dev *dev, uint32 baud)
{
    uint32 clock_speed = _usart_clock_freq(dev);

    /* Convert desired baud rate to baud rate register setting. */
    uint32 integer_part = (25 * clock_speed) / (4 * baud);
    uint32 tmp = (integer_part / 100) << 4;
    uint32 fractional_part = integer_part - (100 * (tmp >> 4));
    tmp |= (((fractional_part * 16) + 50) / 100) & ((uint8)0x0F);

    dev->regs->BRR = (uint16)tmp;
}

/**
 * @brief Call a function on each USART.
 * @param fn Function to call.
 */
void usart_foreach(void (*fn)(const usart_dev*)) {
    fn(USART1);
#if 0
    fn(USART2);
    fn(USART3);
#endif
#ifdef STM32_HIGH_DENSITY
    fn(UART4);
    fn(UART5);
#endif
}

/*
 * Interrupt handlers.
 */

__weak void __irq_usart1(void) {
    usart_irq(USART1);
}

__weak void __irq_usart2(void) {
    usart_irq(USART2);
}

__weak void __irq_usart3(void) {
    usart_irq(USART3);
}

#ifdef STM32_HIGH_DENSITY
__weak void __irq_uart4(void) {
    usart_irq(UART4);
}

__weak void __irq_uart5(void) {
    usart_irq(UART5);
}
#endif

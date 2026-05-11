/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2025, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * saturn_uart16550.h - 16550 UART Driver for Saturn NetLink Modem
 *
 * The Saturn NetLink modem (MK-80118) exposes a 16550-compatible UART on
 * the A-bus (cartridge slot). The modem contains a Rockwell L39 controller
 * (6502-based) connected to an RC288DPi V.34 data pump. The L39 presents
 * standard Hayes AT commands over the UART interface — no proprietary
 * protocol is needed.
 *
 * IMPORTANT: Two Saturn-specific quirks are required for correct operation:
 *
 * 1. SMPC command 0x0A (NEON) must be sent to power on the modem before
 *    any register access. Without this, the hardware is unpowered.
 *    Call saturn_netlink_smpc_enable() before using any UART functions.
 *
 * 2. After each register read or write, 0xFF must be written to address
 *    0x2582503D. The purpose is undocumented but required on real hardware.
 *    This is handled automatically by saturn_uart_reg_read/write().
 *
 * Register base: 0x25895001 (stride 4, odd-byte addresses)
 * Interrupt: SCU External Interrupt 12 (vector 0x5C)
 *
 * Sources:
 *   - Yabause/Kronos netlink.c: register map and AT command handling
 *     https://github.com/Yabause/yabause/blob/master/yabause/src/netlink.c
 *   - CyberWarriorX (Theo Berkau) on SegaXtreme: SMPC enable + quirk addr
 *     https://segaxtreme.net/threads/yabause-netlink-code.24153/
 *   - SegaXtreme NetLink ROM dumps thread: L39 and hardware details
 *     https://segaxtreme.net/threads/netlink-rom-dumps.24942/
 *   - Yabause Wiki SMPC page: command 0x0A = NEON, 0x0B = NEOFF
 *     http://wiki.yabause.org/index.php5?title=SMPC
 */

/** @file uart16550.h
 *  @author Farkus, port by Slinga
 *
 *  @brief Dial-up Modem UART
 *  @bug
 */

#ifndef __JO_UART16550_H__
#define __JO_UART16550_H__

#ifdef JO_COMPILE_WITH_MODEM_SUPPORT

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * UART Instance — configurable base address and stride
 * ========================================================================= */

typedef struct {
    uint32_t base;    /* Register base address (e.g. 0x25895001) */
    uint32_t stride;  /* Bytes between registers (e.g. 4) */
} saturn_uart16550_t;

/* Register indices */
#define SATURN_UART_RBR  0   /* Receive Buffer Register (read)           */
#define SATURN_UART_THR  0   /* Transmit Holding Register (write)        */
#define SATURN_UART_DLL  0   /* Divisor Latch Low (when DLAB=1)          */
#define SATURN_UART_IER  1   /* Interrupt Enable (or DLM when DLAB=1)    */
#define SATURN_UART_DLM  1   /* Divisor Latch High (when DLAB=1)         */
#define SATURN_UART_IIR  2   /* Interrupt Identification (read)          */
#define SATURN_UART_FCR  2   /* FIFO Control Register (write)            */
#define SATURN_UART_LCR  3   /* Line Control Register                    */
#define SATURN_UART_MCR  4   /* Modem Control Register                   */
#define SATURN_UART_LSR  5   /* Line Status Register                     */
#define SATURN_UART_MSR  6   /* Modem Status Register                    */
#define SATURN_UART_SCR  7   /* Scratch Register                         */

void saturn_netlink_smpc_enable(void);
bool saturn_uart_detect(const saturn_uart16550_t* uart);
void saturn_uart_init(const saturn_uart16550_t* uart, uint16_t divisor);
void saturn_uart_flush_rx(const saturn_uart16550_t* uart);
bool saturn_uart_puts(const saturn_uart16550_t* uart, const char* str);
bool saturn_uart_rx_ready(const saturn_uart16550_t* uart);
bool saturn_uart_tx_ready(const saturn_uart16550_t* uart);
uint8_t saturn_uart_reg_read(const saturn_uart16550_t* uart, int reg);
bool saturn_uart_putc(const saturn_uart16550_t* uart, uint8_t c);
uint8_t saturn_uart_getc(const saturn_uart16550_t* uart);
int saturn_uart_getc_timeout(const saturn_uart16550_t* uart, uint32_t timeout);

#endif /* !JO_COMPILE_WITH_MODEM_SUPPORT */

#endif /* !__JO_UART16550_H__ */

/*
** END OF FILE
*/


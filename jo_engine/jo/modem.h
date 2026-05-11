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
/** @file modem.h
 *  @author Farkus, port by Slinga
 *
 *  @brief WIP dial-up modem support
 *  @bug Only supports detecting card, no support for playing videos
 */

#ifndef __JO_MODEM_H__
#define __JO_MODEM_H__

#ifdef JO_COMPILE_WITH_MODEM_SUPPORT

#include <jo/uart16550.h>

#define MODEM_TIMEOUT       2000000   /* Standard command timeout */
#define MODEM_TIMEOUT_LONG  5000000   /* Extended timeout for reset */
#define MODEM_LINE_MAX      128
#define MODEM_GUARD_TIME    200000    /* Guard time for +++ escape */

#define MODEM_BAUD_9600       12        /* Divisor for 9600 baud */
#define MODEM_BAUD_14400       8        /* Divisor for 14,400 baud */
#define MODEM_BAUD_28800       4        /* Divisor for 28,800 baud (US modem) */
#define MODEM_SETTLE_CYCLES   2000000   /* L39 post-init settle (~700ms) */
#define MODEM_PROBE_TIMEOUT   3000000   /* AT probe timeout (~1s) */

typedef enum {
    MODEM_OK = 0,
    MODEM_ERROR,
    MODEM_TIMEOUT_ERR,
    MODEM_CONNECT,
    MODEM_NO_CARRIER,
    MODEM_BUSY,
    MODEM_NO_DIALTONE,
    MODEM_NO_ANSWER,
    MODEM_RING,
    MODEM_UNKNOWN
} modem_result_t;

bool modem_is_present(void);
bool modem_get_uart(saturn_uart16550_t* uart);
modem_result_t modem_init(const saturn_uart16550_t* uart);
modem_result_t modem_probe(const saturn_uart16550_t* uart);
modem_result_t modem_dial(const saturn_uart16550_t* uart,
                          const char* number,
                          uint32_t timeout);
void modem_flush_input(const saturn_uart16550_t* uart);
modem_result_t modem_hangup(const saturn_uart16550_t* uart);

bool modem_recv_ready(const saturn_uart16550_t* uart);
uint8_t modem_recv_byte(const saturn_uart16550_t* uart);
int modem_send_bytes(const saturn_uart16550_t* uart, const uint8_t* data, int len);

#endif /* !JO_COMPILE_WITH_MODEM_SUPPORT */

#endif /* !__JO_MODEM_H__ */

/*
** END OF FILE
*/


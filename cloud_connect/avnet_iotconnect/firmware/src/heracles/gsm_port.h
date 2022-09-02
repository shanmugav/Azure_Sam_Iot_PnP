/**
 * Copyright 2019-2021 EBV Elektronik. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of EBV Elektronik may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EBV "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL EBV BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __GSM_PORT_H__
#define __GSM_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** GSM hardware abstraction layer functions. */

/** GSM critical section functions. */
uint32_t gsm_enter_critical_section(void);
void gsm_exit_critical_section(unsigned int status);

/** GSM delay functions. */
uint32_t gsm_port_get_tick(void);
void gsm_port_delay(uint32_t delay);

/** GSM power sequence functions. */
int gsm_port_power_on(int timeout);
int gsm_port_power_off(void);
int gsm_port_get_status(void);

/** GSM UART functions. */
void gsm_port_io_init(void);
void gsm_port_io_deinit(void);
void gsm_port_uart_open(void);
void gsm_port_uart_close(void);
void gsm_port_uart_putc(uint8_t c);
void gsm_port_uart_isr(void);

#ifdef __cplusplus
}
#endif

#endif /* __GSM_PORT_H__ */

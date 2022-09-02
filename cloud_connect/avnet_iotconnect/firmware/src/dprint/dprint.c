/**
 * Copyright 2019-2020 EBV Elektronik. All rights reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/** Debug includes. */
#include "dprint/dprint.h"
#include "dprint/port/dprint_port.h"

/**
 * \brief Debug function to print log similar to printf.
 */
void dprint_printf(const char *format, ...)
{
	int len = 0;
	va_list args;
	va_start(args, format);
	char tmp[512];

	len = vsnprintf(&tmp[len], sizeof(tmp), format, args);
	if (len == sizeof(tmp)) {
		dprint_printf("Warning input dprint_printf string too long... now truncated!\r\n");
	}
	for (int i = 0; i < len; ++i) {
		dprint_port_printc(tmp[i]);
	}
}

/*
static void dprint_print_unbuffered(char *str, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i) {
		while (!LL_USART_IsActiveFlag_TXE(UART_DBG)) {
		}
		LL_USART_ClearFlag_TC(UART_DBG);
		LL_USART_TransmitData8(UART_DBG, str[i]);
	}
}

static char logger[30000] = {0};
static int logger_size = 0;
void dprint_logger(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	logger_size += _vsnprintf(&logger[logger_size], sizeof(logger) - logger_size, format, args);
}

void dprint_logger_dump(void)
{
	char *msg1 = "\r\n\r\n\r\n########### LOGGER DUMP ###########\r\n";
	char *msg2 = "########### ------ DUMP ###########\r\n";

	dprint_print_unbuffered(msg1, strlen(msg1));
	dprint_print_unbuffered(logger, logger_size);
	dprint_print_unbuffered(msg2, strlen(msg2));
	memset(logger, 0, sizeof(logger));
	dprint_printf(" -> %d bytes\r\n\r\n", logger_size);
	logger_size = 0;
}*/

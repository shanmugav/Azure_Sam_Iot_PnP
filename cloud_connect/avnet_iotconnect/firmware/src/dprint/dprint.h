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

#ifndef __DPRINT_H__
#define __DPRINT_H__

#include <stdint.h>
#include <stdarg.h>

#include "dprint/port/dprint_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DPRINT_COLOR

#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_FORMAT(letter, format)  LOG_COLOR_##letter "(%u) [%s]: " format LOG_RESET_COLOR "\r\n"
#define LOG_RAW(letter, format)     LOG_COLOR_##letter format LOG_RESET_COLOR

#ifdef DPRINT_COLOR
#define DPRINT_D(tag, format, ...)  dprint_printf(LOG_FORMAT(D, format), (unsigned int)ms_ticks, tag, ##__VA_ARGS__)
#define DPRINT_I(tag, format, ...)  dprint_printf(LOG_FORMAT(I, format), (unsigned int)ms_ticks, tag, ##__VA_ARGS__)
#define DPRINT_W(tag, format, ...)  dprint_printf(LOG_FORMAT(W, format), (unsigned int)ms_ticks, tag, ##__VA_ARGS__)
#define DPRINT_E(tag, format, ...)  dprint_printf(LOG_FORMAT(E, format), (unsigned int)ms_ticks, tag, ##__VA_ARGS__)
#define DPRINT_RD(format, ...)      dprint_printf(LOG_RAW(D, format), ##__VA_ARGS__)
#define DPRINT_RI(format, ...)      dprint_printf(LOG_RAW(I, format), ##__VA_ARGS__)
#define DPRINT_RW(format, ...)      dprint_printf(LOG_RAW(W, format), ##__VA_ARGS__)
#define DPRINT_RE(format, ...)      dprint_printf(LOG_RAW(E, format), ##__VA_ARGS__)
#else
#define DPRINT_D(tag, format, ...)  dprint_printf(format "\r\n", ##__VA_ARGS__, tag)
#define DPRINT_I(tag, format, ...)  dprint_printf(format "\r\n", ##__VA_ARGS__, tag)
#define DPRINT_W(tag, format, ...)  dprint_printf(format "\r\n", ##__VA_ARGS__, tag)
#define DPRINT_E(tag, format, ...)  dprint_printf(format "\r\n", ##__VA_ARGS__, tag)
#define DPRINT_RD(format, ...)      dprint_printf(format "\r\n", ##__VA_ARGS__)
#define DPRINT_RI(format, ...)      dprint_printf(format "\r\n", ##__VA_ARGS__)
#define DPRINT_RW(format, ...)      dprint_printf(format "\r\n", ##__VA_ARGS__)
#define DPRINT_RE(format, ...)      dprint_printf(format "\r\n", ##__VA_ARGS__)
#endif

void dprint_open(void);
void dprint_close(void);

void dprint_io_init(void);
void dprint_io_deinit(void);

void dprint_printf(const char *format, ...);

/*
void dprint_print_unbuffered(char *str, uint32_t len);
void dprint_logger(const char *format, ...);
void dprint_logger_dump(void);
*/

#ifdef __cplusplus
}
#endif

#endif /* __DPRINT_H__ */

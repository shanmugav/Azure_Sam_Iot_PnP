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

#ifndef __GSM_SOCKET_H__
#define __GSM_SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

/** GSM IP status. */
enum {
    IP_INITIAL      = 0,
    IP_START        = 1,
    IP_CONFIG       = 2,
    IP_GPRSACT      = 3,
    IP_STATUS       = 4,
    IP_CONNECTING   = 5,
    IP_CONNECTED    = 6,
    IP_CLOSING      = 7,
    IP_CLOSED       = 8,
    IP_ERROR        = 9,
    IP_PDP_DEACT    = 10,
    IP_STATE_QTY,
    IP_UNKNOWN      = -1,
};

/** Return codes used by socket functions. */
typedef enum {
    GSM_SOCKET_OK            = 0,
    GSM_SOCKET_WAITING       = -1,
    GSM_SOCKET_CLOSED        = -2,
    GSM_SOCKET_TIMEOUT       = -3,
    GSM_SOCKET_NO_DATA       = -4,
	GSM_SOCKET_OVERFLOW      = -5,
} gsm_resp_socket_t;

int gsm_tcp_open(const char* addr, const char* port, uint8_t tls);
int gsm_tcp_read(char *buf, size_t len);
int gsm_tcp_write(char* data, uint16_t length, uint16_t timeout);
int gsm_tcp_close(void);

#ifdef __cplusplus
}
#endif

#endif /* __GSM_SOCKET_H__ */

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

#ifndef __GSM_SERIAL_H__
#define __GSM_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Type of serial command. */
typedef enum
{
    CMD_NULL = 0,
    CMD_AT,
    CMD_SMS_HEADER,
    CMD_SMS_BODY,
    CMD_HTTP_DATA,
    CMD_IP_HEADER,
    CMD_IP_DATA,
    CMD_FS_HEADER,
    CMD_FS_DATA

} gsm_serial_cmd_t;

/** GSM driver serial events. */
typedef enum
{
    GSM_SEVT_AT_OK = 1,
    GSM_SEVT_IPSEND_OK = GSM_SEVT_AT_OK,
    GSM_SEVT_IPSHUT_OK,
    GSM_SEVT_AT_ECHO,
    GSM_SEVT_AT_TIMEOUT,
    GSM_SEVT_AT_ERROR,
    GSM_SEVT_AT_CME_ERROR,
    GSM_SEVT_AT_CMS_ERROR,
    GSM_SEVT_AT_MSG,
    GSM_SEVT_AT_URC,
    GSM_SEVT_AT_PROMPT,
    GSM_SEVT_AT_POWER_DOWN,
    GSM_SEVT_BREAK,
    GSM_SEVT_OVERFLOW,
	GSM_SEVT_NOISE_ERROR,
    GSM_SEVT_DOWNLOAD,
    GSM_SEVT_BIN_MSG,
    GSM_SEVT_UNLOCK,

} gsm_sevents_t;

/**
 * \brief Callback to be registered at gsm_serial_open. Will be called on serial events.
 *
 * \param[out] evt Serial event amongst gsm_sevents_t.
 * \param[out] data Actual serial data, first byte codes the data length (i.e. data_length = data[0]).
 */
typedef void (gsm_serial_callback_t) (uint8_t evt, uint8_t* data);

void gsm_serial_open(gsm_serial_callback_t* callback, uint8_t ate);
void gsm_serial_close(void);
void gsm_serial_send(const char* data, uint8_t cmd_type);
void gsm_serial_sendbin(uint8_t* data, uint16_t len, uint8_t cmd_type);
uint8_t *gsm_serial_get_cmee(void);

/** GSM serial functions to process UART interrupt. */
void gsm_serial_isr_data(uint8_t in);
void gsm_serial_isr_overflow(void);
void gsm_serial_isr_framing(void);
void gsm_serial_isr_noise(void);

#ifdef __cplusplus
}
#endif

#endif /* __GSM_SERIAL_H__ */

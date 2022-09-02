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

#ifndef __GSM_H__
#define __GSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GSM_MAX_WAKEUP_RETRY (5)

/** GSM command context state machine. */
typedef struct {
	uint8_t msg[128];
	uint8_t urc[128];
	uint8_t resp;
	uint8_t serr;
	uint8_t prompt;
	uint8_t msg_avail;
	uint8_t urc_avail;
	uint8_t bin_avail;

	struct {
		volatile int state;
		volatile uint8_t *data_buff;
		volatile uint32_t data_len;
		volatile uint32_t data_pos;
		volatile uint32_t data_avail;
#define IP_ADDR_LEN (4 * 3 + 3)
		char ip_addr[IP_ADDR_LEN];
		uint8_t tls;
	} tcp;

} gsm_context_t;

/** Return codes used for all functions. */
typedef enum {
    // OK response, function ended on success
    GSM_RESP_OK            = 0,
    // Still waiting for terminal response
    GSM_RESP_WAITING       = -1,
    // ERROR response, function ended on error
    GSM_RESP_ERROR         = -2,
    // Timeout, no response on due time
    GSM_RESP_TIMEOUT       = -3,
    // Response did not match expected
    GSM_RESP_NOMATCH       = -4,
    // No response
    GSM_RESP_NOHRESP       = -5,
    // Unknown RSSI returned value
    GSM_RESP_UNKNOWN_RSSI  = -6,
    // CREG searching operator
    GSM_RESP_SEARCHING     = -7,
    // CREG registration denied
	GSM_RESP_DENIED        = -8,
} gsm_resp_t;

/** SIM card interface. */
typedef enum {
    // Use external SIM card
	GSM_SIM_EXTERNAL       = 1,
    // Use internal Orange prepaid SIM card
    GSM_SIM_INTERNAL       = 2,
} gsm_sim_t;

/**
 * \brief Callback to be registered for URC or AT command processing.
 *
 * \param[in] times Number of attempts at parsing the request.
 * \param[in] resp String to be processed.
 * \return Response code.
 */
typedef int (gsm_resp_handler_t) (int times, char* resp);

/** GSM command processor related internal functions. */
int gsm_check_response(const char *match);
int gsm_check_urc(const char *match);
int gsm_wait_urc(const char *match, uint16_t timeout);
int gsm_wait_urc_with_handler(gsm_resp_handler_t *randler, uint16_t timeout);
int gsm_at_command(const char *cmd, const char *match, uint32_t timeout);
int gsm_at_command_with_handler(const char *cmd, gsm_resp_handler_t *randler, uint16_t timeout);

/** GSM module functions. */
int gsm_init(gsm_sim_t sim, const char *cpin);
int gsm_get_csq_status(int *in_rssi, uint32_t timeout);
int gsm_get_reg_status(uint32_t timeout);
int gsm_get_gatt_status(uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __GSM_SERIAL_H__ */

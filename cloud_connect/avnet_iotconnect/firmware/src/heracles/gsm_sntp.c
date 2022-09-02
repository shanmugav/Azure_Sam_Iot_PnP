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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

/** Heracles includes. */
#include "heracles/gsm.h"
#include "heracles/gsm_sntp.h"
#include "heracles/gsm_config.h"

/** Debug includes. */
#include "dprint/dprint.h"

/** Debug tag prefix definition. */
static const char *TAG = "heracles";
static char time[25];

/**
 * \brief Handler to parse SIMCom module CCLK message.
 *
 * \param[in] times Number of attempts at reading CCLK value.
 * \param[in] resp String containing the CCLK message from SIMCom.
 * \return Response code.
 */
static int sntp_handler(int times, char *resp)
{
	int idx1 = -1;
	int idx2 = -1;
	uint32_t len = 0;

	/* Find SNTP URC. */
	if (strstr(resp, "+CCLK:") != 0) {

		/* Extract date and convert format. */
		len = strlen(resp);
		for (uint32_t i = 0; i < len; ++i) {
			if (resp[i] == '"') {
				idx1 = i;
				break;
			}
		}
		for (uint32_t i = idx1 + 1; i < len; ++i) {
			if (resp[i] == '"') {
				idx2 = i;
				break;
			}
		}
		if (idx1 > 0 && idx2 > 0) {
			time[0] = '2';
			time[1] = '0';
			time[2] = resp[idx1 + 1];
			time[3] = resp[idx1 + 2];
			time[4] = '-';
			time[5] = resp[idx1 + 4];
			time[6] = resp[idx1 + 5];
			time[7] = '-';
			time[8] = resp[idx1 + 7];
			time[9] = resp[idx1 + 8];
			time[10] = 'T';
			memcpy(time + 11, resp + idx1 + 10, 8);
			memcpy(time + 19, ".000Z", 5);
		}

		return GSM_RESP_OK;
	}

	return GSM_RESP_WAITING;
}

/**
 * \brief Initialize SIMCom SNTP module and synchronize clock.
 *
 * \param[in] timezone Timezone to read local time.
 * \return Response code.
 */
int gsm_sntp_init(uint32_t timezone)
{
	int ret;
	char scratch[128];

	/* Set NTP to use bearer profile 0. */
	gsm_at_command("AT+CNTPCID=0\r", NULL, 1000);

	/* Set SNTP remote server and configure timezone. */
	sprintf(scratch, "AT+CNTP=\"pool.ntp.org\",%lu,0,2\r", timezone);
	ret = gsm_at_command(scratch, NULL, 5000);
	if (ret != GSM_RESP_OK) {
		DPRINT_E(TAG, "gsm_sntp_init: SNTP initialization error %d", ret);
		return ret;
	}

	/* Start synchronize network time. */
	ret = gsm_at_command("AT+CNTP\r", NULL, 1000);
	if (ret != GSM_RESP_OK) {
		DPRINT_E(TAG, "gsm_sntp_init: SNTP start synchronize failed %d", ret);
		return ret;
	}
	ret = gsm_wait_urc("+CNTP: 1", 15000);
	if (ret != GSM_RESP_OK) {
		DPRINT_E(TAG, "gsm_sntp_get_time: SNTP request timeout %d", ret);
		return ret;
	}

	return GSM_RESP_OK;
}

/**
 * \brief Initialize SIMCom SNTP module and synchronize clock.
 *
 * \param[out] str Buffer to store current time value.
 * \return Response code.
 */
int gsm_sntp_get_time(char *str)
{
	memset(time, 0, 25);

	/* Query local time. */
	gsm_at_command_with_handler("AT+CCLK?\r", sntp_handler, 1000);
	memcpy(str, time, 25);

	return GSM_RESP_OK;
}

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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

/** Heracles includes. */
#include "heracles/gsm.h"
#include "heracles/gsm_serial.h"
#include "heracles/gsm_port.h"
#include "heracles/gsm_config.h"

/** Debug includes. */
#include "dprint/dprint.h"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

/** GSM command state machine variable. */
gsm_context_t g_gsm = {
	.resp = 0,
	.serr = 0,
	.msg_avail = false,
	.urc_avail = false,
	.bin_avail = 0,
	.prompt = 0,
	.tcp.state = -1,
};

/** GSM modem specific. */
static int rssi = 0;

/** Debug tag prefix definition. */
static const char *TAG = "heracles";

#if GSM_CONFIG_DEBUG_MODE
int gsm_print_process(void);
int gsm_print_process(void)
{
	uint8_t gsm_msg_avail, gsm_urc_avail, gsm_bin_avail;
	uint8_t gsm_resp;

	/* Disable interrupt to avoid concurrent access to AT input buffer. */
	uint32_t cs = gsm_enter_critical_section();
	gsm_msg_avail = g_gsm.msg_avail;
	gsm_urc_avail = g_gsm.urc_avail;
	gsm_bin_avail = g_gsm.bin_avail;
	gsm_resp = g_gsm.resp;
	g_gsm.msg_avail = 0;
	g_gsm.urc_avail = 0;
	g_gsm.bin_avail = 0;
	g_gsm.resp = 0;
	gsm_exit_critical_section(cs);

	if (gsm_msg_avail) {
		DPRINT_E(TAG, "%s", (char *)g_gsm.msg);
	}

	if (gsm_resp) {
		switch (gsm_resp) {
		case GSM_SEVT_AT_OK:
			DPRINT_E(TAG, ">OK"); break;

		case GSM_SEVT_AT_ERROR:
			DPRINT_E(TAG, ">ERROR"); break;

		case GSM_SEVT_AT_TIMEOUT:
			DPRINT_E(TAG, ">TIMEOUT"); break;

		case GSM_SEVT_OVERFLOW:
			DPRINT_E(TAG, ">RXOVFL"); break;

		case GSM_SEVT_NOISE_ERROR:
			DPRINT_E(TAG, ">NOISE"); break;

		case GSM_SEVT_AT_CME_ERROR:
			DPRINT_E(TAG, ">CME ERROR:%s", gsm_serial_get_cmee()); break;

		case GSM_SEVT_AT_CMS_ERROR:
			DPRINT_E(TAG, ">CMS ERROR:%s", gsm_serial_get_cmee()); break;

		default:
			break;
		}
	}

	if (gsm_urc_avail) {
		DPRINT_E(TAG, "%s", (char *)g_gsm.urc);
	}

	if (gsm_bin_avail) {
		DPRINT_E(TAG, "BIN(%d):%X %X %X %X", gsm_bin_avail,
				g_gsm.urc[0],
				g_gsm.urc[1],
				g_gsm.urc[2],
				g_gsm.urc[3]);
	}

	return 1;
}

static void gsm_print_error(uint8_t resp)
{
	switch (resp) {
	case GSM_SEVT_AT_ERROR:
		DPRINT_E(TAG, ">ERROR"); break;

	case GSM_SEVT_AT_TIMEOUT:
		DPRINT_E(TAG, ">TIMEOUT"); break;

	case GSM_SEVT_OVERFLOW:
		DPRINT_E(TAG, ">RXOVFL"); break;

	case GSM_SEVT_BREAK:
		DPRINT_E(TAG, ">BREAK"); break;

	case GSM_SEVT_NOISE_ERROR:
		DPRINT_E(TAG, ">NOISE"); break;

	case GSM_SEVT_AT_CME_ERROR:
		DPRINT_E(TAG, ">CME ERROR:%s", gsm_serial_get_cmee()); break;

	case GSM_SEVT_AT_CMS_ERROR:
		DPRINT_E(TAG, ">CMS ERROR:%s", gsm_serial_get_cmee()); break;

	default:
		DPRINT_E(TAG, "e!%x", resp); break;
	}
}
#endif /* GSM_CONFIG_DEBUG_MODE */

/**
 * \brief Callback to handle command events from SIMCom module from interrupt context.
 * This function takes care of updating GSM command state machine.
 *
 * \param[in] evt Event type.
 * \param[in] data Data corresponding to the event (if any).
 */
static void gsm_serial_callback(uint8_t evt, uint8_t *data)
{
	switch (evt) {
	case GSM_SEVT_AT_OK:
	case GSM_SEVT_AT_ERROR:
	case GSM_SEVT_AT_TIMEOUT:
	case GSM_SEVT_AT_CME_ERROR:
	case GSM_SEVT_AT_CMS_ERROR:
		g_gsm.resp = evt;
		break;

	case GSM_SEVT_OVERFLOW:
	case GSM_SEVT_BREAK:
	case GSM_SEVT_NOISE_ERROR:
		g_gsm.serr = evt;
		break;

	case GSM_SEVT_AT_URC:
		memcpy(g_gsm.urc, &data[1], data[0]);
		g_gsm.urc[data[0]] = '\n';
		g_gsm.urc[data[0] + 1] = 0;
		g_gsm.urc_avail = true;
		break;

	case GSM_SEVT_AT_MSG:
		memcpy(g_gsm.msg, &data[1], data[0]);
		g_gsm.msg[data[0]] = '\n';
		g_gsm.msg[data[0] + 1] = 0;
		g_gsm.msg_avail = true;
		break;

	case GSM_SEVT_AT_PROMPT:
		g_gsm.prompt = 1;
		break;

	case GSM_SEVT_BIN_MSG:
		memcpy(g_gsm.urc, &data[1], data[0]);
		g_gsm.bin_avail = data[0];
		break;

	default:
		break;
	}
}

/**
 * \brief Handler to parse SIMCom module FW version.
 *
 * \param[in] times Number of attempts at reading GMR status.
 * \param[in] resp String containing the GMR message from SIMCom.
 * \return Response code.
 */
static int gmr_state_handler(int times, char *resp)
{
	/* Filter other URCs */
	if (strstr(resp, "Revision:") == 0) {
		return GSM_RESP_ERROR;
	}
	else {
		resp[strlen(resp) - 1] = 0;
		DPRINT_I(TAG, "gsm_init: FW version %s", resp + 10);
		return GSM_RESP_OK;
	}
}

/**
 * \brief Handler to parse SIMCom module CSQ message (RSSI).
 *
 * \param[in] times Number of attempts at reading CSQ status.
 * \param[in] resp String containing the CSQ message from SIMCom.
 * \return GSM_RESP_OK if successful with global rssi indicating RSSI
 * current level or GSM_RESP_UNKNOWN_RSSI if no signal can be found.
 */
static int csq_state_handler(int times, char *resp)
{
	int ptr = 6;
	int num = 0;
	int mul = 1;

	/* Filter other URCs */
	if (strstr(resp, "+CSQ: ") != 0) {
		while (resp[ptr] != ',') {
			num = num * mul + (resp[ptr++] - 48);
			mul *= 10;
			if (mul > 100) {
				/* Something went wrong max value is 99. */
				return GSM_RESP_ERROR;
			}
		}

		if (num == 0) {
			rssi = 115;
		}
		else if (num == 1) {
			rssi = 111;
		}
		else if (num < 31) {
			rssi = 110 - 2 * (num - 2);
		}
		else if (num == 31) {
			rssi = 52;
		}
		else {
			return GSM_RESP_UNKNOWN_RSSI;
		}
	}

	return GSM_RESP_OK;
}

/**
 * \brief Handler to parse SIMCom module CREG message.
 *
 * \param[in] times Number of attempts at reading CREG status.
 * \param[in] resp String containing the CREG message from SIMCom.
 * \return Response code.
 */
static int creg_state_handler(int times, char *resp)
{
	/* Filter other URCs */
	if (strstr(resp, "+CREG:") == 0) {
#if GSM_CONFIG_DEBUG_MODE
		DPRINT_D(TAG, "creg_state_handler: %s -> Filtered", resp);
#endif /* GSM_CONFIG_DEBUG_MODE */
		return GSM_RESP_WAITING;
	}

	/* 0 not registered, MT is not currently searching a new operator to register to */
	/* 1 registered, home network */
	/* 2 not registered, but MT is currently searching a new operator to register to */
	/* 3 registration denied */
	/* 4 unknown (e.g. out of GERAN/UTRAN/E-UTRAN coverage) */
	/* 5 registered, roaming */
	/* following are for E-UTRAN only: */
	/* 6 registered for "SMS only", home network */
	/* 7 registered for "SMS only", roaming */
	/* 8 attached for emergency bearer services only */
	/* 9 registered for "CSFB not preferred", home network */
	/* 10 registered for "CSFB not preferred", roaming */
	if ((strstr(resp, "+CREG: 0,1" ) != 0) ||
			(strstr(resp, "+CREG: 0,5" ) != 0)) {
		return GSM_RESP_OK;
	} else if ((strstr(resp, "+CREG: 0,0" ) != 0) ||
			(strstr(resp, "+CREG: 0,2" ) != 0)) {
		DPRINT_E(TAG, "creg_state_handler: device not registered!");
		return GSM_RESP_SEARCHING;
	} else if (strstr(resp, "+CREG: 0,3" ) != 0) {
		DPRINT_E(TAG, "creg_state_handler: registration denied error!");
		return GSM_RESP_DENIED;
	}

	return GSM_RESP_ERROR;
}

/**
 * \brief Read command context state machine and check for command response.
 *
 * \param[in] match String to match against a response message from SIMCom module.
 * \return Response code.
 *
 * \note This function shortly disable interrupts to read and update the command
 * context state machine.
 */
int gsm_check_response(const char *match)
{
	uint8_t gsm_msg_avail;
	uint8_t gsm_resp;

	if (g_gsm.serr) {
#if GSM_CONFIG_DEBUG_MODE
		gsm_print_error(g_gsm.serr);
#endif /* GSM_CONFIG_DEBUG_MODE */
		g_gsm.serr = 0;
		return GSM_RESP_ERROR;
	}

	if (g_gsm.resp) {
		/* Disable interrupt to avoid concurrent access to AT input buffer. */
		uint32_t cs = gsm_enter_critical_section();
		gsm_msg_avail = g_gsm.msg_avail;
		gsm_resp = g_gsm.resp;
		g_gsm.msg_avail = 0;
		g_gsm.resp = 0;
		gsm_exit_critical_section(cs);

		if (gsm_resp != GSM_SEVT_AT_OK) {
#if GSM_CONFIG_DEBUG_MODE
			gsm_print_error(gsm_resp);
#endif /* GSM_CONFIG_DEBUG_MODE */
			return GSM_RESP_ERROR;
		} else if (match) {
#if GSM_CONFIG_DEBUG_MODE
			DPRINT_D(TAG, "gsm_check_response: recv <- %s", (const char *)g_gsm.msg);
#endif /* GSM_CONFIG_DEBUG_MODE */
			if (gsm_msg_avail && (strstr((const char *)g_gsm.msg, match) != NULL)) {
				return GSM_RESP_OK;
			} else {
				return GSM_RESP_NOMATCH;
			}
		} else {
			return GSM_RESP_OK;
		}
	}

	return GSM_RESP_WAITING;
}

/**
 * \brief Read command context state machine and check for received Unsolicited
 * Response Code (URC).
 *
 * \param[in] match String to match against a URC message from SIMCom module.
 * \return Response code.
 *
 * \note This function shortly disable interrupts to read and update the command
 * context state machine.
 */
int gsm_check_urc(const char *match)
{
	uint8_t gsm_urc_avail;

	/* Disable interrupt to avoid concurrent access to AT input buffer. */
	uint32_t cs = gsm_enter_critical_section();
	gsm_urc_avail = g_gsm.urc_avail;
	g_gsm.urc_avail = 0;
	gsm_exit_critical_section(cs);

	if (gsm_urc_avail) {
		if (match) {
#if GSM_CONFIG_DEBUG_MODE
			DPRINT_D(TAG, "gsm_check_urc: recv URC <- %s", (const char *)g_gsm.urc);
#endif /* GSM_CONFIG_DEBUG_MODE */
			if ((strstr((const char *)g_gsm.urc, match) != NULL)) {
				return GSM_RESP_OK;
			} else {
				/* The next URC is not always the one we wait for, */
				/* so wait until we get it or timeout. */
				return GSM_RESP_WAITING;
			}
		} else {
			return GSM_RESP_OK;
		}
	}

	return GSM_RESP_WAITING;
}

/**
 * \brief Wait for the desired URC to be read from SIMCom module for the specified
 * amount of time.
 *
 * \param[in] match String to match against a URC message from SIMCom module.
 * \param[in] timeout Maximum delay to wait for a response.
 * \return Response code.
 */
int gsm_wait_urc(const char *match, uint16_t timeout)
{
	int status = GSM_RESP_WAITING;
	uint32_t tickstart = gsm_port_get_tick();
	while (status == GSM_RESP_WAITING) {
		status = gsm_check_urc(match);
		if (timeout && (gsm_port_get_tick() > (tickstart + timeout))) {
			status = GSM_RESP_TIMEOUT;
		}
	}

#if GSM_CONFIG_DEBUG_MODE
	if (status == GSM_RESP_OK) {
		DPRINT_D(TAG, "gsm_wait_urc: %s -> OK", match);
	} else {
		DPRINT_E(TAG, "gsm_wait_urc: %s -> ERR:%d", match, status);
	}
#endif /* GSM_CONFIG_DEBUG_MODE */

	return status;
}

/**
 * \brief Wait for the desired URC to be read from SIMCom module for the specified
 * amount of time and process data with the specified handler.
 *
 * \param[in] randler Function pointer to the specified handler.
 * \param[in] timeout Maximum delay to wait for a response.
 * \return Response code.
 *
 * \note This function shortly disable interrupts to read and update the command
 * context state machine.
 */
int gsm_wait_urc_with_handler( gsm_resp_handler_t *randler, uint16_t timeout)
{
	int status = GSM_RESP_WAITING;
	uint32_t tickstart = gsm_port_get_tick();
	int times = 0;
	while (status == GSM_RESP_WAITING) {
		uint8_t gsm_urc_avail;
		uint8_t serr;

		/* Disable interrupt to avoid concurrent access to AT input buffer. */
		uint32_t cs = gsm_enter_critical_section();
		gsm_urc_avail = g_gsm.urc_avail;
		serr = g_gsm.serr;
		g_gsm.serr = 0;
		g_gsm.urc_avail = 0;
		gsm_exit_critical_section(cs);

		if (serr) {
#if GSM_CONFIG_DEBUG_MODE
			gsm_print_error(serr);
#endif /* GSM_CONFIG_DEBUG_MODE */
		}

		if (gsm_urc_avail) {
			status = randler(times++, (char *)g_gsm.urc);
		}

		if (timeout && (gsm_port_get_tick() > (tickstart + timeout))) {
			status = GSM_RESP_TIMEOUT;
		}
	}

#if GSM_CONFIG_DEBUG_MODE
	if (status == GSM_RESP_OK) {
		DPRINT_D(TAG, "gsm_wait_urc_with_handler: URC -> OK");
	} else if (status == GSM_RESP_TIMEOUT) {
		DPRINT_E(TAG, "gsm_wait_urc_with_handler: URC -> ERR:%d", status);
	} else {
		DPRINT_E(TAG, "gsm_wait_urc_with_handler: recv <- %s", g_gsm.urc);
		DPRINT_E(TAG, "gsm_wait_urc_with_handler: URC -> ERR:%d", status);
	}
#endif /* GSM_CONFIG_DEBUG_MODE */

	return status;
}

/**
 * \brief Send AT command to SIMCom module and wait for a specific response format.
 *
 * \param[in] cmd AT command to be sent.
 * \param[in] match String to match against a command response message from SIMCom module.
 * \param[in] timeout Maximum delay to wait for a response.
 * \return Response code.
 */
int gsm_at_command(const char *cmd, const char *match, uint32_t timeout)
{
	int status = GSM_RESP_WAITING;
#if GSM_CONFIG_DEBUG_MODE
	DPRINT_D(TAG, "gsm_at_command: send -> %s", cmd);
#endif /* GSM_CONFIG_DEBUG_MODE */
	gsm_serial_send(cmd, CMD_AT);
	uint32_t tickstart = gsm_port_get_tick();
	while (status == GSM_RESP_WAITING) {
		status = gsm_check_response(match);
		if (timeout && (gsm_port_get_tick() > (tickstart + timeout))) {
			status = GSM_RESP_TIMEOUT;
		}
	}

#if GSM_CONFIG_DEBUG_MODE
	if (status == GSM_RESP_OK) {
		DPRINT_D(TAG, "gsm_at_command: response OK");
	} else {
		DPRINT_E(TAG, "gsm_at_command: response ERR:%d", status);
	}
#endif /* GSM_CONFIG_DEBUG_MODE */

	return status;
}

/**
 * \brief Send AT command to SIMCom module and wait for a response that will be processed
 * in the specified handler.
 *
 * \param[in] cmd AT command to be sent.
 * \param[in] randler Function pointer to a handler that can process the response.
 * \param[in] timeout Maximum delay to wait for a response.
 * \return Response code.
 *
 * \note This function shortly disable interrupts to read and update the command
 * context state machine.
 */
int gsm_at_command_with_handler(const char *cmd, gsm_resp_handler_t *rhandler, uint16_t timeout)
{
	int status = GSM_RESP_WAITING;
	int hstatus = GSM_RESP_NOHRESP;
#if GSM_CONFIG_DEBUG_MODE
	DPRINT_D(TAG, "gsm_at_command_with_handler: send -> %s", cmd);
#endif /* GSM_CONFIG_DEBUG_MODE */
	gsm_serial_send(cmd, CMD_AT);
	uint32_t tickstart = gsm_port_get_tick();
	while (status == GSM_RESP_WAITING) {
		uint8_t gsm_msg_avail = 0;
		uint8_t gsm_resp = 0;
		uint8_t gsm_serr = 0;
		uint8_t times = 0;

		if (g_gsm.msg_avail || g_gsm.resp || g_gsm.serr) {
			/* Disable interrupt to avoid concurrent access to AT input buffer. */
			uint32_t cs = gsm_enter_critical_section();
			gsm_msg_avail   = g_gsm.msg_avail;
			gsm_resp        = g_gsm.resp;
			gsm_serr        = g_gsm.serr;
			g_gsm.msg_avail = 0;
			g_gsm.resp      = 0;
			g_gsm.serr      = 0;
			gsm_exit_critical_section(cs);
		}

		if (gsm_msg_avail) {
			gsm_msg_avail   = 0;
			hstatus         = rhandler(times++, (char *)g_gsm.msg);
		}

		if (gsm_serr) {
#if GSM_CONFIG_DEBUG_MODE
			gsm_print_error(gsm_serr);
#endif /* GSM_CONFIG_DEBUG_MODE */
			status = GSM_RESP_ERROR;
		}

		if (gsm_resp) {
			if (gsm_resp != GSM_SEVT_AT_OK) {
#if GSM_CONFIG_DEBUG_MODE
				gsm_print_error(gsm_resp);
#endif /* GSM_CONFIG_DEBUG_MODE */
				status = GSM_RESP_ERROR;
			} else {
				status = hstatus;
			}
		}

		if (timeout && (gsm_port_get_tick() > (tickstart + timeout))) {
			status = GSM_RESP_TIMEOUT;
		}
	}

#if GSM_CONFIG_DEBUG_MODE
	if (status == GSM_RESP_OK) {
		DPRINT_D(TAG, "gsm_at_command_with_handler: response OK");
	} else {
		DPRINT_E(TAG, "gsm_at_command_with_handler: response ERR:%d", status);
	}
#endif /* GSM_CONFIG_DEBUG_MODE */

	return status;
}

/**
 * \brief Initialize SIMCom module.
 *
 * \param[in] sim SIM card interface to use.
 * \param[in] cpin PIN number required to activate SIM card (if any).
 * \return Response code.
 */
int gsm_init(gsm_sim_t sim, const char *cpin)
{
	int ret;
	int retry = 0;
	char scratch[128];

	g_gsm.resp = 0;
	g_gsm.serr = 0;
	g_gsm.msg_avail = false;
	g_gsm.urc_avail = false;
	g_gsm.bin_avail = 0;
	g_gsm.prompt = 0;
	g_gsm.tcp.state = -1;

	/* Perform IO initialization sequence. */
	DPRINT_I(TAG, "gsm_init: power up modem...");
	ret = gsm_port_power_on(5000);
	if (ret != GSM_RESP_OK) {
		DPRINT_E(TAG, "gsm_init: modem timeout error (status pin not ready)!");
		return ret;
	}

	/* Open USART to Heracles. */
	gsm_serial_open(&gsm_serial_callback, 1);

	/* Test Heracles ready. Send AT. */
	DPRINT_I(TAG, "gsm_init: checking modem status...");
	do {
		ret = gsm_at_command("AT\r", NULL, 1000);
		if ((ret != GSM_RESP_OK) && (retry++ > GSM_MAX_WAKEUP_RETRY)) {
			return ret;
		}
	} while (ret != GSM_RESP_OK);

	/* Print Heracles FW version. */
	gsm_at_command_with_handler("AT+GMR\r", gmr_state_handler, 1000);

	/* Set preferred mode to LTE CAT-M (ignore for Heracles 1). */
	gsm_at_command("AT+CMNB=1\r", NULL, 1000);

	/* Enable AT verbose mode. */
	DPRINT_I(TAG, "gsm_init: setting verbose level...");
	ret = gsm_at_command("AT+CMEE=2\r", NULL, 2000);
	if (ret != GSM_RESP_OK) {
		DPRINT_E(TAG, "gsm_init: CMEE error!");
	}

	/* Select SIM card interface. */
	sprintf(scratch, "AT+CSIMSW=%d\r", sim);
	ret = gsm_at_command(scratch, NULL, 2000);

	/* Test SIM ready. Send AT+CPIN. */
	DPRINT_I(TAG, "gsm_init: waiting SIM ready...");
	/* Test may fail as CPIN may not be ready at startup and return error value immediately (no timeout). */
	ret = gsm_at_command("AT+CPIN?\r", "READY", 5000);
	if (ret != GSM_RESP_OK) {
		/* CPIN ready message may be deferred, so wait URC CPIN status message. */
		ret = gsm_wait_urc("+CPIN: READY", 5000);
		if (ret != GSM_RESP_OK) {
			/* Try enter PIN code and wait ready. Send AT+CPIN. */
			DPRINT_I(TAG, "gsm_init: configuring SIM (%s)...", cpin && (strlen(cpin) == 0) ? "no PIN" : "PIN");
			if (strlen(cpin)) {
				sprintf(scratch, "AT+CPIN=%s\r", cpin);
				ret = gsm_at_command(scratch, NULL, 2000);
				if (ret != GSM_RESP_OK) {
					DPRINT_E(TAG, "gsm_init: CPIN error!");
					return ret;
				}
				ret = gsm_wait_urc("+CPIN: READY", 5000);
				if (ret != GSM_RESP_OK) {
					DPRINT_E(TAG, "gsm_init: CPIN error!");
					return ret;
				}
			} else {
				DPRINT_E(TAG, "gsm_init: CPIN not ready error!");
				return ret;
			}
		}
	}

	/* Deactivate the PDP context and close all connections. */
	gsm_at_command("AT+CACLOSE=0\r", 0, 1);
	gsm_at_command("AT+CNACT=0,0\r", 0, 1);

	DPRINT_I(TAG, "gsm_init: OK.");
	return ret;
}

/**
 * \brief Get SIMCom RSSI level.
 *
 * \param[out] in_rssi Measured RSSI level.
 * \param[in] timeout Maximum delay to wait for a response.
 * \return 0 if success, an error code otherwise.
 */
int gsm_get_csq_status(int *in_rssi, uint32_t timeout)
{
	int ret;
	rssi = 0;

	ret = gsm_at_command_with_handler("AT+CSQ\r", csq_state_handler, timeout);
	*in_rssi = rssi;
	return ret;
}

/**
 * \brief Get SIMCom GSM registration status.
 *
 * \param[in] timeout Maximum delay to wait for a response.
 * \return GSM_RESP_OK if modem is registered, an error code otherwise.
 */
int gsm_get_reg_status(uint32_t timeout)
{
	int ret;

	ret = gsm_at_command_with_handler("AT+CREG?\r", creg_state_handler, timeout);
	return ret;
}

/**
 * \brief Get SIMCom GPRS status.
 *
 * \param[in] timeout Maximum delay to wait for a response.
 * \return GSM_RESP_OK if GPRS is attached, an error code otherwise.
 */
int gsm_get_gatt_status(uint32_t timeout)
{
	int ret;

	ret = gsm_at_command("AT+CGATT?\r", "+CGATT: 1", timeout);
	return ret;
}

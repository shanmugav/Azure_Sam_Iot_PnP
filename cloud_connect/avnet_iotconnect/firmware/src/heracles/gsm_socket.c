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
#include "heracles/gsm_socket.h"

/** Debug includes. */
#include "dprint/dprint.h"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

/** GSM context state machine. */
extern gsm_context_t g_gsm;

/** Debug tag prefix definition. */
static const char *TAG = "heracles";

/** SIMCom IP state notification values. */
/*static const char *k_ip_state_str[IP_STATE_QTY] = {
	"IP INITIAL",
	"IP START",
	"IP CONFIG",
	"IP GPRSACT",
	"IP STATUS",
	"TCP CONNECTING",
	"CONNECT OK",
	"IP CLOSING",
	"TCP CLOSED",
	"TCP ERROR",
	"PDP DEACT"
};*/

/** Pointer to server root CA in PEM format (only necessary when using SIMCom internal TLS stack). */
#if GSM_CONFIG_USE_TLS_AT_COMMAND
static char *k_tls_certificate = TLS_CERT_DATA;
#endif /* GSM_CONFIG_USE_TLS_AT_COMMAND */

// Forward declaration only used for TLS AT command based.
int gsm_fs_write(char *filename, char *data, uint32_t length);

/**
 * \brief Handler to parse SIMCom module PDP status message.
 *
 * \param[in] times Number of attempts at reading PDP status.
 * \param[in] resp String containing the PDP status.
 * \return Response code.
 */
static int gsm_pdp_state_handler(int times, char *resp)
{
	/* Check for PDP activated then parse IP address. */
	if (strstr(resp, "+CNACT: 0,1")) {
		for (uint32_t ptr = 13; ptr < strlen(resp); ++ptr) {
			if (resp[ptr] == '\"') {
				memset(g_gsm.tcp.ip_addr, 0, IP_ADDR_LEN);
				memcpy(g_gsm.tcp.ip_addr, resp + 13, ptr - 13);
				g_gsm.tcp.state = IP_GPRSACT;
				break;
			}
		}
	}

	return GSM_RESP_OK;
}

/**
 * \brief Handler to parse SIMCom module TCP status message.
 *
 * \param[in] times Number of attempts at reading TCP status.
 * \param[in] resp String containing the TCP status from SIMCom.
 * \return Response code.
 */
static int gsm_socket_state_handler(int times, char *resp)
{
	/* Check for PDP activated then parse IP address. */
	if (strstr(resp, "+CAOPEN: 0,")) {
		if (resp[11] == '0') {
			g_gsm.tcp.state = IP_CONNECTED;
		}
		else {
			g_gsm.tcp.state = IP_ERROR;			
		}
	}

	return GSM_RESP_OK;
}

/**
 * \brief Write file to SIMCom file system.
 *
 * \param[in] filename Name of the file to write in the file system.
 * \param[in] data Buffer representing the file content.
 * \param[in] len Length of the file.
 * \return Response code.
 */
#define FS_WR_CHUNK_SIZE     64
#define FS_WR_DELAY          100
#define FS_PROMPT_TIMEOUT    500
int gsm_fs_write(char *filename, char *data, uint32_t length)
{
	int status = GSM_RESP_WAITING;
	char scratch[128];
	sprintf(scratch, "AT+FSWRITE=%s,0,%d,60\r", filename, (int)length);
	g_gsm.prompt = 0;
	gsm_serial_send(scratch, CMD_FS_HEADER);
	uint32_t tickstart = gsm_port_get_tick();

	while (!g_gsm.prompt) {
		if ((gsm_port_get_tick() > (tickstart + FS_PROMPT_TIMEOUT))) {
			status = GSM_RESP_TIMEOUT;
			return status;
		}
	}

	while (status == GSM_RESP_WAITING) {
		uint8_t _len = (length > FS_WR_CHUNK_SIZE) ? FS_WR_CHUNK_SIZE : length;
		if (length) {
			gsm_serial_sendbin((uint8_t *)data, _len, CMD_FS_DATA);
			data    += _len;
			length  -= _len;
			gsm_port_delay(FS_WR_DELAY);
		}

		status = gsm_check_response(NULL);
	}
	return status;
}

/**
 * \brief Perform GPRS Attach then open TCP socket and connect to remote server.
 *
 * \param[in] addr URL or IP address of the remote server.
 * \param[in] port Port number of the remote server.
 * \param[in] tls Boolean to enable/disable TLS encryption using SIMCom module internal
 * TLS stack. If set to false, connection is clear TCP.
 * \return Response code.
 */
int gsm_tcp_open(const char *addr, const char *port, uint8_t tls)
{
	int ret;
	int retry = 0;
	uint32_t tickstart;
	char scratch[128];

	/* Connection sequence. */
	DPRINT_I(TAG, "gsm_tcp_open: enabling PDP context...");
	memset(g_gsm.tcp.ip_addr, 0, IP_ADDR_LEN);
	g_gsm.tcp.state = IP_UNKNOWN;
	g_gsm.tcp.tls = tls;

	/* Configure PDP Context 0 and Access Point Name (APN). */
	sprintf(scratch, "AT+CNCFG=0,1,%s\r", GSM_CONFIG_APN);
	ret = gsm_at_command(scratch, NULL, 2000);
	if (ret != GSM_RESP_OK) {
		return ret;
	}

	do {
		/* Force deactivate PDP Context 0. */
		gsm_at_command("AT+CNACT=0,0\r", NULL, 2000);
		gsm_port_delay(100);

		/* Activate PDP Context 0. */
		sprintf(scratch, "AT+CNACT=0,1\r");
		ret = gsm_at_command(scratch, NULL, 2000);
	}
	while (ret != GSM_RESP_OK && ++retry < 5);
	if (ret != GSM_RESP_OK) {
		return ret;
	}
	
	/* PDP Context status (get local IP). */
	tickstart = gsm_port_get_tick();
	sprintf(scratch, "AT+CNACT?\r");
	do {
		gsm_at_command_with_handler(scratch, gsm_pdp_state_handler, 5000);
		if (gsm_port_get_tick() - tickstart > 20000) {
			DPRINT_I(TAG, "gsm_tcp_open: status GPRS TIMEOUT");
			return GSM_RESP_ERROR;
		}
	} while (g_gsm.tcp.state != IP_GPRSACT);
	DPRINT_I(TAG, "gsm_tcp_open: assigned IP address: %s", g_gsm.tcp.ip_addr);

	/* Close any previous TCP socket. */
	gsm_at_command("AT+CACLOSE=0\r", 0, 1000);

	/* Open TCP socket. */
	sprintf(scratch, "AT+CAOPEN=0,0,\"TCP\",\"%s\",\"%s\"\r", addr, port);
	gsm_at_command_with_handler(scratch, gsm_socket_state_handler, 20000);
	if (g_gsm.tcp.state == IP_CONNECTED) {
		DPRINT_I(TAG, "gsm_tcp_open: status TCP CONNECTED");
		return GSM_RESP_OK;
	}
	else {
		DPRINT_I(TAG, "gsm_tcp_open: status TCP FAILED");
		return GSM_RESP_ERROR;
	}
}

/**
 * \brief Read data from TCP socket.
 *
 * \param[out] buf Buffer to store data read from TCP socket.
 * \param[in] len Length of provided input buffer.
 * \return Number of bytes read or socket status code.
 */
int gsm_tcp_read(char *buf, size_t len)
{	
	int ret;
	char scratch[128];

	/* Check that socket is connected. */
	if (g_gsm.tcp.state != IP_CONNECTED) {
		return GSM_SOCKET_CLOSED;
	}

	/* If data is available and there is no reception pending, read data. */
	if (g_gsm.tcp.data_avail) {

		/* Receive data in synchronous mode. */
		g_gsm.tcp.data_buff = (uint8_t *)buf;
		sprintf(scratch, "AT+CARECV=0,%d\r", (len > 1400) ? 1400 : len);
		ret = gsm_at_command(scratch, NULL, 5000);
		if (ret != GSM_RESP_OK) {
			gsm_tcp_close();
			return GSM_SOCKET_CLOSED;
		}

		/* Variables data_len and data_buff are updated from interrupt context. */
		if (g_gsm.tcp.data_len) {
			return g_gsm.tcp.data_len;
		}
		else {
			return GSM_SOCKET_NO_DATA;
		}
	}
	else {
		return GSM_SOCKET_NO_DATA;
	}
}

/**
 * \brief Write data to TCP socket.
 *
 * \param[in] data AT command to be sent.
 * \param[in] length Function pointer to a handler that can process the response.
 * \param[in] timeout Maximum delay to wait SIMCom and remote server data acknowledge.
 * 0 to wait forever.
 * \return Socket status code.
 */
int gsm_tcp_write(char *data, uint16_t length, uint16_t timeout)
{
	int status = GSM_SOCKET_WAITING;
	char scratch[128];

	/* Check that socket is connected. */
	if (g_gsm.tcp.state != IP_CONNECTED) {
		return GSM_SOCKET_CLOSED;
	}

	/* Send AT command header and wait for AT prompt. */
	sprintf(scratch, "AT+CASEND=0,%d\r", length);
	g_gsm.prompt = 0;
	gsm_serial_send(scratch, CMD_IP_HEADER);
	uint32_t tickstart = gsm_port_get_tick();
	while (!g_gsm.prompt) {
		if (g_gsm.tcp.state != IP_CONNECTED) {
			return GSM_SOCKET_CLOSED;
		}
		if (gsm_check_response(NULL) == GSM_RESP_ERROR) {
			return GSM_SOCKET_CLOSED;
		}
		if (timeout && (gsm_port_get_tick() > (tickstart + timeout))) {
			status = GSM_SOCKET_TIMEOUT;
			return status;
		}
	}

	/* Send AT command body packet data and wait for ACK. */
	gsm_serial_sendbin((uint8_t *)data, length, CMD_IP_HEADER);
	while (status == GSM_SOCKET_WAITING) {
		if (g_gsm.tcp.state != IP_CONNECTED) {
			return GSM_SOCKET_CLOSED;
		}
		status = gsm_check_response(NULL);
		if (status == GSM_RESP_ERROR) {
			return GSM_SOCKET_CLOSED;
		}
		if (timeout && (gsm_port_get_tick() > (tickstart + timeout))) {
			status = GSM_SOCKET_TIMEOUT;
		}
	}

	return status;
}

/**
 * \brief Close TCP socket and disconnect from remote server.
 *
 * \return Response code.
 */
int gsm_tcp_close(void)
{
	int ret;

	/* Deactivate the PDP context and close all connections. */
	ret = gsm_at_command("AT+CACLOSE=0\r", 0, 1000);
	ret = gsm_at_command("AT+CNACT=0,0\r", 0, 5000);

	return ret;
}

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
#include "heracles/gsm_serial.h"
#include "heracles/gsm_port.h"
#include "heracles/gsm_socket.h"
#include "heracles/gsm.h"

/** GSM special char definitions. */
#define SERIAL_CHAR_BS                           '\b'
#define SERIAL_CHAR_CR                           '\r'
#define SERIAL_CHAR_LF                           '\n'
#define SERIAL_CHAR_EOF                          (26)
#define SERIAL_CHAR_ZERO                         '\0'
#define SERIAL_CHAR_GT                           '>'
#define SERIAL_CHAR_COL                          ':'

/** GSM serial command buffer definitions. */
#define GSM_SERIAL_RX_BUFFER_BYTE_LEN            (256)
#define GSM_SERIAL_CMEE_BUFFER_BYTE_LEN          (32)

/** GSM modem specific. */
#define GSM_SIM800_ATOK_RSP                      "OK"
#define GSM_SIM800_ATOK_RSP_LEN                  (2)
#define GSM_SIM800_ATERR_RSP                     "ERROR"
#define GSM_SIM800_ATERR_RSP_LEN                 (5)
#define GSM_SIM800_ATERR_CME_RSP                 "+CME ERROR: "
#define GSM_SIM800_ATERR_CME_RSP_LEN             (12)
#define GSM_SIM800_ATERR_CMS_RSP                 "+CMS ERROR: "
#define GSM_SIM800_ATERR_CMS_RSP_LEN             (12)
#define GSM_SIM800_POWER_DOWN_RSP                "NORMAL POWER DOWN"
#define GSM_SIM800_POWER_DOWN_RSP_LEN            (17)
#define GSM_SIM800_DOWNLOAD_RSP                  "DOWNLOAD"
#define GSM_SIM800_DOWNLOAD_RSP_LEN              (8)
#define GSM_SIM800_IPSEND_OK                     "SEND OK"
#define GSM_SIM800_IPSEND_OK_LEN                 (7)
#define GSM_SIM800_IPSHUT_OK                     "SHUT OK"
#define GSM_SIM800_IPSHUT_OK_LEN                 (7)
#define GSM_SIM800_IPDATA_URC                    "+IPD,"
#define GSM_SIM800_IPDATA_URC_LEN                (5)
#define GSM_SIM800_FSTIMEOUT_RSP                 "TimeOut"
#define GSM_SIM800_FSTIMEOUT_RSP_LEN             (7)
#define GSM_SIM800_SOCKET_CLOSED                 "CLOSED"
#define GSM_SIM800_SOCKET_CLOSED_LEN             (6)
#define GSM_SIM800_SOCKET_PDP                    "+PDP"
#define GSM_SIM800_SOCKET_PDP_LEN                (4)

#define GSM_SIM7070_IPDATA_URC                   "+CARECV: "
#define GSM_SIM7070_IPDATA_URC_LEN               (9)
#define GSM_SIM7070_SOCKET_CLOSED                "+CASTATE: 0,0"
#define GSM_SIM7070_SOCKET_CLOSED_LEN            (13)
#define GSM_SIM7070_SOCKET_DATA_AVAIL            "+CADATAIND:"
#define GSM_SIM7070_SOCKET_DATA_AVAIL_LEN        (11)
#define GSM_SIM7070_SOCKET_NO_DATA_AVAIL         "+CARECV: 0"
#define GSM_SIM7070_SOCKET_NO_DATA_AVAIL_LEN     (10)

/** Enumerator of serial states. */
typedef enum {
	/* / Off */
	GSM_SERIAL_STATE_OFF,
	/* / Idle, nothing to do */
	GSM_SERIAL_STATE_IDLE,
	/* / Send message */
	GSM_SERIAL_STATE_TX,
} gsm_serial_state_t;

/** Structure of the serial global state. */
typedef struct {
	/* User Callback */
	gsm_serial_callback_t *cb;
	/* TX state */
	gsm_serial_state_t tx_state;
	/* RX data pending */
	uint8_t rx_data_pen;
	/* RX ping-pong command buffer */
	uint8_t rx_buf[2][GSM_SERIAL_RX_BUFFER_BYTE_LEN];
	/* RX buffer pointer */
	uint8_t *rx;
	/* RX buffer size */
	uint8_t rxi;
	/* RX state */
	uint8_t rx_state;
	/* AT Echo status */
	uint8_t ate;
	/* AT Echo expected */
	uint8_t echo_pending;
	/* AT Cmd Pending */
	uint8_t cmd_pending;
	/* Bin mode flag */
	uint8_t bin_mode;
	/* Maximum bytes to be read in binary mode */
	uint16_t bin_size;
	/* Binary mode timeout */
	uint16_t bin_timeout;
	/* Extended error buffer */
	uint8_t mee[GSM_SERIAL_CMEE_BUFFER_BYTE_LEN];
} gsm_serial_t;

/** Macro definition for command buffers. */
#define SERIAL_INIT_BUFFERS()       do { \
                                        g_gsm_serial.rx = g_gsm_serial.rx_buf[0]; \
                                        g_gsm_serial.rxi = 1; \
                                    } while (0)
#define SERIAL_PUSH_BUFFER(c)       do {\
                                        /*dprint_logger("%c %d %c\r\n", g_gsm_serial.bin_mode ? 'b' : 'D', g_gsm_serial.rxi, c);*/ \
                                        g_gsm_serial.rx[g_gsm_serial.rxi++] = (c); \
                                    } while (0)
#define SERIAL_SWAP_BUFFERS()       do { \
                                        g_gsm_serial.rxi = 1; \
                                        g_gsm_serial.rx = (g_gsm_serial.rx == g_gsm_serial.rx_buf[1]) ? \
                                                          g_gsm_serial.rx_buf[0]: \
                                                          g_gsm_serial.rx_buf[1]; \
                                    } while (0)
#define SERIAL_RECYCLE_BUFFER()     g_gsm_serial.rxi = 1
#define SERIAL_FINALIZE_BUFFER()    do { \
                                        g_gsm_serial.rx[g_gsm_serial.rxi] = SERIAL_CHAR_ZERO; \
                                        g_gsm_serial.rx[0] = g_gsm_serial.rxi-1; \
                                    } while (0)
#define SERIAL_BUFFER_SIZE()        (g_gsm_serial.rxi - 1)

/** Serial status. */
static gsm_serial_t g_gsm_serial;

/** GSM context state machine. */
extern gsm_context_t g_gsm;

/**
 * \brief Parse final terminal response and return corresponding event.
 *
 * \param[in] rx Input buffer.
 * \return Terminal event (OK, ERROR, ...)
 */
static int gsm_response_is_terminal(uint8_t *rx)
{
	uint8_t len = *rx++;
	int ret = 0;
	/* Reset CMEE */
	g_gsm_serial.mee[0] = '\0';
	if (!strncmp((const char *)rx, GSM_SIM800_ATOK_RSP, GSM_SIM800_ATOK_RSP_LEN)) {
		ret = GSM_SEVT_AT_OK;
	} else if ((!strncmp((const char *)rx, GSM_SIM800_ATERR_RSP, GSM_SIM800_ATERR_RSP_LEN)) ||
			(!strncmp((const char *)rx, GSM_SIM800_FSTIMEOUT_RSP, GSM_SIM800_FSTIMEOUT_RSP_LEN))) {
		ret = GSM_SEVT_AT_ERROR;
	} else if (!strncmp((const char *)rx, GSM_SIM800_DOWNLOAD_RSP, GSM_SIM800_DOWNLOAD_RSP_LEN)) {
		ret = GSM_SEVT_DOWNLOAD;
	} else if (!strncmp((const char *)rx, GSM_SIM800_IPSEND_OK, GSM_SIM800_IPSEND_OK_LEN)) {
		ret = GSM_SEVT_IPSEND_OK;
	} else if (!strncmp((const char *)rx, GSM_SIM800_IPSHUT_OK, GSM_SIM800_IPSHUT_OK_LEN)) {
		ret = GSM_SEVT_AT_OK; /* Accept SHUT OK as a regular AT command successful acknowledgment */
	} else if (!strncmp((const char *)rx, GSM_SIM800_ATERR_CME_RSP, GSM_SIM800_ATERR_CME_RSP_LEN)) {
		ret = GSM_SEVT_AT_CME_ERROR;
	} else if (!strncmp((const char *)rx, GSM_SIM800_ATERR_CMS_RSP, GSM_SIM800_ATERR_CMS_RSP_LEN)) {
		ret = GSM_SEVT_AT_CMS_ERROR;
	} else if (!strncmp((const char *)rx, GSM_SIM800_POWER_DOWN_RSP, GSM_SIM800_POWER_DOWN_RSP_LEN)) {
		ret = GSM_SEVT_AT_POWER_DOWN;
	}

	/* Save CMEE */
	if ((ret == GSM_SEVT_AT_CME_ERROR) || (ret == GSM_SEVT_AT_CMS_ERROR)) {
		/* We want to add a \n to ease printout */
		len = SERIAL_BUFFER_SIZE() - GSM_SIM800_ATERR_CME_RSP_LEN + 1;
		if (len > GSM_SERIAL_CMEE_BUFFER_BYTE_LEN - 1) {
			strncpy((char *)&g_gsm_serial.mee[0], (const char *)&rx[GSM_SIM800_ATERR_CME_RSP_LEN], GSM_SERIAL_CMEE_BUFFER_BYTE_LEN - 3);
			g_gsm_serial.mee[GSM_SERIAL_CMEE_BUFFER_BYTE_LEN - 2] = '\n';
			g_gsm_serial.mee[GSM_SERIAL_CMEE_BUFFER_BYTE_LEN - 1] = '\0';
		} else {
			strcpy((char *)&g_gsm_serial.mee[0], (const char *)&rx[GSM_SIM800_ATERR_CME_RSP_LEN]);
			g_gsm_serial.mee[len - 1] = '\n';
			g_gsm_serial.mee[len] = '\0';
		}
	}

	return ret;
}

/**
 * \brief Parse URC and check if socket has data available.
 *
 * \param[in] rx Input buffer.
 * \return 1 if data is available, 0 otherwise.
 */
static int gsm_response_is_socket_data_available(uint8_t *rx)
{
	rx++;
	if (!strncmp((const char *)rx, GSM_SIM7070_SOCKET_DATA_AVAIL, GSM_SIM7070_SOCKET_DATA_AVAIL_LEN)) {
		return 1;
	}

	return 0;
}

/**
 * \brief Parse URC and check if socket has no data available.
 *
 * \param[in] rx Input buffer.
 * \return 1 if no data is available, 0 otherwise.
 */
static int gsm_response_is_socket_no_data_available(uint8_t *rx)
{
	rx++;
	if (!strncmp((const char *)rx, GSM_SIM7070_SOCKET_NO_DATA_AVAIL, GSM_SIM7070_SOCKET_NO_DATA_AVAIL_LEN)) {
		return 1;
	}

	return 0;
}

/**
 * \brief Parse URC and check if this is a socket closed event.
 *
 * \param[in] rx Input buffer.
 * \return 1 if this is a closed event, 0 otherwise.
 */
static int gsm_response_is_socket_closed(uint8_t *rx)
{
	uint8_t len = *rx++;

	if ((len == GSM_SIM800_SOCKET_CLOSED_LEN) && (!strncmp((const char *)rx, GSM_SIM800_SOCKET_CLOSED, GSM_SIM800_SOCKET_CLOSED_LEN))) {
		return 1;
	}
	else if ((len == GSM_SIM800_SOCKET_PDP_LEN) && (!strncmp((const char *)rx, GSM_SIM800_SOCKET_PDP, GSM_SIM800_SOCKET_PDP_LEN))) {
		return 1;
	}
	else if ((len == GSM_SIM7070_SOCKET_CLOSED_LEN) && (!strncmp((const char *)rx, GSM_SIM7070_SOCKET_CLOSED, GSM_SIM7070_SOCKET_CLOSED_LEN))) {
		return 1;
	}

	return 0;
}

/**
 * \brief Parse URC for incoming IP Data binary size.
 *
 * \param[in] rx Input buffer.
 * \return Binary size of the URC. -1 if not an IP Data URC.
 */
static int gsm_urc_is_ipdata(uint8_t *rx)
{
	int ret = -1;
	rx++;
	if (!strncmp((const char *)rx, GSM_SIM7070_IPDATA_URC, GSM_SIM7070_IPDATA_URC_LEN)) {
		for (uint32_t i = 0; rx[i]; ++i) {
			if (rx[i] == ',') {
				rx[i] = 0;
				break;
			}
		}
		rx += GSM_SIM7070_IPDATA_URC_LEN;
		ret = atoi((const char *)rx);
	}

	return ret;
}

/**
 * \brief Send data buffer over serial to SIMCom module.
 *
 * \param[out] data Buffer to send.
 * \param[out] len Size of the buffer to send.
 * \param[out] cmd_type Command type.
 */
static void gsm_serial_n_send(uint8_t *data, uint16_t len, uint8_t cmd_type)
{
	if (!len) {
		return;
	}
	g_gsm_serial.cmd_pending = cmd_type;

	/* URC collision catching: */
	/* Flag that we expect echo from our command after we send the 1st char. */
	gsm_port_uart_putc(data[0]);
	g_gsm_serial.echo_pending = ((cmd_type != CMD_HTTP_DATA) && (cmd_type != CMD_FS_DATA)) ? g_gsm_serial.ate : false;
	for (uint32_t i = 1; i < len; i++) {
		gsm_port_uart_putc(data[i]);
	}
}

/**
 * \brief Open serial port to SIMCom module.
 *
 * \param[in] callback Function pointer to manage serial events from SIMCom module.
 * \param[in] at Set to 1 to enable AT ECHO (recommended to avoid URC collision), 0 otherwise.
 */
void gsm_serial_open(gsm_serial_callback_t *callback, uint8_t ate)
{
	/* Initialize GSM serial state machine. */
	g_gsm_serial.cb = callback;
	g_gsm_serial.tx_state = GSM_SERIAL_STATE_IDLE;
	g_gsm_serial.cmd_pending = CMD_NULL;
	g_gsm_serial.echo_pending = false;
	g_gsm_serial.rx_data_pen = false;
	g_gsm_serial.ate = ate;
	g_gsm_serial.bin_size = 0;
	g_gsm_serial.bin_mode = false;
	g_gsm_serial.bin_timeout = 0;
	SERIAL_INIT_BUFFERS();

	/* Initialize UART module. */
	gsm_port_uart_open();
}

/**
 * \brief Close serial port to SIMCom module.
 */
void gsm_serial_close(void)
{
	g_gsm_serial.tx_state = GSM_SERIAL_STATE_OFF;

	/* Disable UART module. */
	gsm_port_uart_close();
}

/**
 * \brief Send AT command over serial to SIMCom module.
 *
 * \param[in] data AT command buffer to send to SIMCom module.
 * \param[in] cmd_type Command type amongst gsm_serial_cmd_t.
 */
void gsm_serial_send(const char *data, uint8_t cmd_type)
{
	uint16_t len = strlen((const char *)data);

	gsm_serial_n_send((uint8_t *)data, len, cmd_type);
}

/**
 * \brief Send binary data over serial to SIMCom module.
 *
 * \param[in] data Binary buffer to send to SIMCom module.
 * \param[in] len Length of the binary buffer.
 * \param[in] cmd_type Command type amongst gsm_serial_cmd_t.
 */
void gsm_serial_sendbin(uint8_t *data, uint16_t len, uint8_t cmd_type)
{
	if (cmd_type == CMD_IP_DATA && g_gsm_serial.ate) {
		/* SIM800 is actually echo'ing binary, prepended with SPACE (0x20). */
		/* Use bin size/mode to handle echo. */
		g_gsm_serial.bin_size = len + 1;
		g_gsm_serial.bin_mode = true;
	}

	gsm_serial_n_send(data, len, cmd_type);
}

/**
 * \brief Get CMEE error code.
 *
 * \return Error code value.
 */
uint8_t *gsm_serial_get_cmee(void)
{
	return &g_gsm_serial.mee[0];
}

/**
 * \brief Interrupt routine to handle character data from SIMCom module.
 *
 * \param[in] in Input character received from interrupt.
 * \note This function is called within interrupt context.
 */
void gsm_serial_isr_data(uint8_t in)
{
	/* BINARY MODE. */
	if (g_gsm_serial.bin_mode == true) {

		/* Add incoming character to packet buffer. */
		g_gsm.tcp.data_buff[g_gsm.tcp.data_pos++] = in;

		/* Once transfer is complete reset state machine. */
		if (g_gsm.tcp.data_pos == g_gsm.tcp.data_len) {
			g_gsm_serial.rx_data_pen = false;
			g_gsm_serial.echo_pending = false;
			g_gsm_serial.bin_mode = false;
			SERIAL_RECYCLE_BUFFER();
		}

	}
	/* TEXT MODE. */
	else {
		/* Carriage return in buffer means we got either: */
		/*  - Complete Echo if echo_pending and starts with 'A' */
		/*  - Incomplete Echo if echo_pending cmd=SMS_BODY -> skip */
		/*  - (Nice) URC if no cmd_pending */
		/*  - (Nasty) URC if echo_pending and starts with !='A' */
		/*  - Response */
		/*  - Terminal response */
		if (in == SERIAL_CHAR_CR) {
			if (SERIAL_BUFFER_SIZE() > 0) {
				if (g_gsm_serial.echo_pending) {
					if (g_gsm_serial.rx[1] == 'A' && (g_gsm_serial.cmd_pending != CMD_SMS_BODY)) {
						/* ECHO. */
						//dprint_logger("ECHO\r\n");
						SERIAL_RECYCLE_BUFFER();
						g_gsm_serial.cb(GSM_SEVT_AT_ECHO, NULL);
						g_gsm_serial.echo_pending = false;
					} else {
						/* NASTY URC. */
						//dprint_logger("NASTY URC\r\n");
						SERIAL_FINALIZE_BUFFER();
						g_gsm_serial.cb(GSM_SEVT_AT_URC, g_gsm_serial.rx);
						SERIAL_SWAP_BUFFERS();
						g_gsm_serial.echo_pending = false;
					}
				} else {
					SERIAL_FINALIZE_BUFFER();
					if (g_gsm_serial.cmd_pending == CMD_AT ||
							g_gsm_serial.cmd_pending == CMD_SMS_HEADER ||
							g_gsm_serial.cmd_pending == CMD_SMS_BODY ||
							g_gsm_serial.cmd_pending == CMD_IP_HEADER ||
							g_gsm_serial.cmd_pending == CMD_IP_DATA ||
							g_gsm_serial.cmd_pending == CMD_FS_HEADER ||
							g_gsm_serial.cmd_pending == CMD_FS_DATA ||
							g_gsm_serial.cmd_pending == CMD_HTTP_DATA) {
						/* Check if this is a terminal response we parse here, in this case send the relevant */
						/* event but don't swap buffer just recycle it (this let the other buffer untouched even */
						/* if a new response comes right after, increasing the time allowed to process it). */
						//dprint_logger("Terminal response?\r\n");
						uint8_t resp;
						if ((resp = gsm_response_is_terminal(g_gsm_serial.rx)) != 0) {
							//dprint_logger("Terminal indeed\r\n");
							/* TERMINAL RESPONSE. */
							g_gsm_serial.cb(resp, NULL);
							SERIAL_RECYCLE_BUFFER();
							g_gsm_serial.cmd_pending = CMD_NULL;
						} else {
							//dprint_logger("Intermediate response\r\n");
							/* INTERMEDIATE RESPONSE. */
							if (g_gsm_serial.cmd_pending == CMD_AT ||
									g_gsm_serial.cmd_pending == CMD_SMS_BODY) {
								//dprint_logger("AT or SMS body\r\n");
								if (gsm_response_is_socket_no_data_available(g_gsm_serial.rx)) {
									g_gsm.tcp.data_avail = 0;
									g_gsm.tcp.data_len = 0;
								}
								g_gsm_serial.cb(GSM_SEVT_AT_MSG, g_gsm_serial.rx);
								SERIAL_SWAP_BUFFERS();
							}
							else {
								/* NICE URC : handle connection closed during above command operation. */
								//dprint_logger("Nice URC\r\n");
								if (gsm_response_is_socket_data_available(g_gsm_serial.rx)) {
									g_gsm.tcp.data_avail = 1;
									SERIAL_RECYCLE_BUFFER();
								}
								else if (gsm_response_is_socket_closed(g_gsm_serial.rx)) {
									g_gsm.tcp.state = IP_CLOSED;
									SERIAL_RECYCLE_BUFFER();
								}
								else {
									g_gsm_serial.cb(GSM_SEVT_AT_URC, g_gsm_serial.rx);
									SERIAL_SWAP_BUFFERS();
								}
							}
						}
					} else {
						/* NICE URC. */
						//dprint_logger("Nice URC\r\n");
						if (gsm_response_is_socket_data_available(g_gsm_serial.rx)) {
							g_gsm.tcp.data_avail = 1;
							SERIAL_RECYCLE_BUFFER();
						}
						else if (gsm_response_is_socket_closed(g_gsm_serial.rx)) {
							g_gsm.tcp.state = IP_CLOSED;
							SERIAL_RECYCLE_BUFFER();
						}
						else {
							g_gsm_serial.cb(GSM_SEVT_AT_URC, g_gsm_serial.rx);
							SERIAL_SWAP_BUFFERS();
						}
					}
				}
			}
		}
		/* '>' is a prompt to signal SMS/IP_DATA ready to send. */
		else if (in == SERIAL_CHAR_GT) {
			if (g_gsm_serial.cmd_pending == CMD_SMS_HEADER ||
					g_gsm_serial.cmd_pending == CMD_IP_HEADER  ||
					g_gsm_serial.cmd_pending == CMD_FS_HEADER) {
				g_gsm_serial.cb(GSM_SEVT_AT_PROMPT, NULL);
				SERIAL_RECYCLE_BUFFER();
				g_gsm_serial.cmd_pending = CMD_NULL;
			} else {
				SERIAL_PUSH_BUFFER(in);
			}
		}
		/* '<EOF>' is the end of an echoed SMS. */
		else if (in == SERIAL_CHAR_EOF) {
			if (g_gsm_serial.cmd_pending == CMD_SMS_BODY) {
				g_gsm_serial.cb(GSM_SEVT_AT_ECHO, (uint8_t *)1);
				SERIAL_RECYCLE_BUFFER();
				g_gsm_serial.echo_pending = false;
			} else {
				SERIAL_PUSH_BUFFER(in);
			}
		}
		/* Special handling of IP Data URC (incoming TCP/IP packets). */
		else if (in == ',') {
			int len;
			SERIAL_PUSH_BUFFER(in);
			if ((len = gsm_urc_is_ipdata(g_gsm_serial.rx)) >= 0) {
				g_gsm.tcp.data_len = len;
				g_gsm.tcp.data_pos = 0;
				if (len > 0) {
					g_gsm.tcp.data_avail = 1;
					if (g_gsm.tcp.data_buff) {
						g_gsm_serial.bin_mode = true;
						g_gsm_serial.bin_size = len;
					}
				}
				else {
					g_gsm.tcp.data_avail = 0;
				}
				SERIAL_RECYCLE_BUFFER();
			}
		}
		/* Other chars are just pushed into the AT command buffer (skipping <LF>). */
		else if (in != SERIAL_CHAR_LF) {
			SERIAL_PUSH_BUFFER(in);
		}
	}
}

/**
 * \brief Interrupt routine to handle overflow error.
 *
 * \note This function is called within interrupt context.
 */
void gsm_serial_isr_overflow(void)
{
	/* Ignore. */
}

/**
 * \brief Interrupt routine to handle framing error.
 *
 * \note This function is called within interrupt context.
 */
void gsm_serial_isr_framing(void)
{
	/* Ignore. */
}

/**
 * \brief Interrupt routine to handle noise error.
 *
 * \note This function is called within interrupt context.
 */
void gsm_serial_isr_noise(void)
{
	/* Ignore. */
}

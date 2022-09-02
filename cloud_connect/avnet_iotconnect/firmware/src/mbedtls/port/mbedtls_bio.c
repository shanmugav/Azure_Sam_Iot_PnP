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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/** ARM mbedTLS includes. */
#include "mbedtls/ssl.h"
#include "mbedtls/port/mbedtls_bio.h"

/** Heracles includes. */
#include "heracles/gsm_socket.h"

/** Debug includes. */
#include "dprint/dprint.h"

/** Debug tag prefix definition. */
static const char *TAG = "mbedtls";

/**
 * \brief Find the last right index of char in string.
 *
 * \param[in] str String where to find right index of char.
 * \param[in] c Char to find in string.
 * \return Char index or 0 if not found.
 */
static char *get_right_index(const char *str, char c)
{
	char *index = 0;

	do {
		if (*str == c) {
			index = (char *)str;
		}
	} while (*str++);

	return (index);
}

/**
 * \brief Open TCP socket and connect to remote server.
 *
 * \param[in] addr URL or IP address of the remote server.
 * \param[in] port Port number of the remote server.
 * \return 0 on success, an error code otherwise.
 */
int mbedtls_bio_connect(const char *addr, const char *port)
{
	return gsm_tcp_open(addr, port, 0);
}

/**
 * \brief Send data to TCP socket.
 *
 * \param[in] ctx Socket descriptor.
 * \param[in] data AT command to be sent.
 * \param[in] length Function pointer to a handler that can process the response.
 * \return Number of bytes sent or 0 if socket was closed.
 */
int mbedtls_bio_send(void *ctx, const unsigned char *buf, size_t len)
{
	// Send buffer to TCP socket.
	int ret = gsm_tcp_write((char *)buf, len, 6000);
	if (ret == GSM_SOCKET_TIMEOUT) {
		return 0;
	}
	else if (ret == GSM_SOCKET_OK) {
		return len;
	}
	else {
		// Socket closed by remote host.
		return 0;
	}
}

/**
 * \brief Receive data from TCP socket.
 *
 * \param[in] ctx Socket descriptor.
 * \param[out] buf Buffer to store data read from TCP socket.
 * \param[in] len Length of provided input buffer.
 * \return Number of bytes read or 0 if socket was closed.
 * In case of socket timeout or overflow return MBEDTLS_ERR_SSL_WANT_READ.
 */
int mbedtls_bio_recv(void *ctx, unsigned char *buf, size_t len)
{
	// Read buffer from TCP socket.
	int ret = gsm_tcp_read((char *)buf, len);
	if (ret > 0) {
		return ret;
	}
	else if (ret == GSM_SOCKET_OVERFLOW) {
		// Socket buffer memory too small to handle TCP traffic.
		DPRINT_E(TAG, "mbedtls_bio_recv: socket buffer overflow ERROR!");
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	else if (ret == GSM_SOCKET_CLOSED) {
		// Socket closed by remote host.
		return 0;
	}
	else {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
}

/**
 * \brief Close TCP socket and disconnect from remote server.
 *
 * \return Response code.
 */
int mbedtls_bio_close(void)
{
	return gsm_tcp_close();
}

/**
 * \brief Debug function for mbedTLS.
 *
 * \param[in] ctx Debug context.
 * \param[in] level Debug log level.
 * \param[in] file Filename which called this debug function.
 * \param[in] line Line number of the file which called this debug function.
 * \param[in] str String to display as debug.
 */
void mbedtls_bio_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    char *file_sep;

    // Shorten 'file' from the whole file path to just the filename.
    file_sep = get_right_index(file, '/');
    if (file_sep) {
        file = file_sep + 1;
    }

    switch(level) {
    case 1:
    	DPRINT_W(TAG, "%s:%d %s", file, line, str);
        break;
    case 2:
    	DPRINT_I(TAG, "%s:%d %s", file, line, str);
        break;
    case 3:
    	DPRINT_D(TAG, "%s:%d %s", file, line, str);
        break;
    case 4:
    	DPRINT_D(TAG, "%s:%d %s", file, line, str);
        break;
    default:
    	DPRINT_E(TAG, "Unexpected log level %d: %s", level, str);
        break;
    }
}

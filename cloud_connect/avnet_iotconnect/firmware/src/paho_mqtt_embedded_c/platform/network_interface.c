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

#include <string.h>
#include <stdlib.h>

/** ARM mbedTLS includes. */
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/debug.h"
#include "mbedtls/port/mbedtls_bio.h"

/** Cryptoauthlib includes. */
#include "cryptoauthlib.h"
#include "atcacert/atcacert_client.h"
#include "mbedtls/atca_mbedtls_wrap.h"

/** Static portion of certificate includes. */
#include "certs/cust_def_1_signer.h"
#include "certs/cust_def_2_device.h"

/** Paho MQTT includes. */
#include "MQTTReturnCodes.h"
#include "network_interface.h"
#include "timer_interface.h"

/** Debug includes. */
#include "dprint/dprint.h"

/** ATECC608A Device Certificate Information. */
#define MAX_TLS_CERT_LENGTH                 (1024)
#define SIGNER_CERT_MAX_LEN                 (g_cert_def_1_signer.cert_template_size + 8)  //! Need some space in case the cert changes size by a few bytes.
#define SIGNER_PUBLIC_KEY_MAX_LEN           (64)
#define DEVICE_CERT_MAX_LEN                 (g_cert_def_2_device.cert_template_size + 8)  //! Need some space in case the cert changes size by a few bytes.

#ifndef MAX
#define MAX(a,b) a > b ? a : b
#endif
#ifndef MIN
#define MIN(a,b) a < b ? a : b
#endif

/** Debug tag prefix definition. */
static const char *TAG = "tls_mqtt";

/** AWS server root CA (server identity verification). */
extern const char msazure_rootca[];

/** Variables for mbedTLS context. */
mbedtls_ssl_context ssl = {0};
static mbedtls_net_context socket_fd = {0};
static mbedtls_entropy_context entropy = {0};
static mbedtls_ctr_drbg_context ctr_drbg = {0};
static mbedtls_x509_crt cacert = {0};
static mbedtls_x509_crt clicert = {0};
static mbedtls_pk_context pkey = {0};
static mbedtls_ssl_config conf = {0};

/** ATECC608 device certificate related variables. */
uint32_t certs_buffer[MAX_TLS_CERT_LENGTH];
uint8_t *signer_cert = NULL;
size_t signer_cert_size;
uint8_t *device_cert = NULL;
size_t device_cert_size;

/**
 * \brief Free TLS socket data.
 */
static void tls_mqtt_socket_clean(void)
{
    mbedtls_x509_crt_free(&clicert);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_pk_free(&pkey);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
	//mbedtls_net_free(&socket_fd);
}

/**
 * \brief Initialize TLS socket with certificate data stored into ATECC608A.
 *
 * \param[out] devId Buffer to store the device ID (Subject Key Identifier).
 * \param[out] cpId Buffer to store the company ID (IoTConnect Identifier).
 * \return 0 on success, -1 on error.
 */
int tls_mqtt_socket_init_certificates(char *devId, char *cpId)
{
    uint8_t signer_public_key[SIGNER_PUBLIC_KEY_MAX_LEN];
	uint8_t serial_number[ATCA_SERIAL_NUM_SIZE];
	int atca_status = 0;
	uint32_t i = 0;

    // Prepare buffer to extract certificates from ATECC608.
    memset(certs_buffer, 0x00, sizeof(certs_buffer));
    signer_cert = ((uint8_t *)certs_buffer) + (sizeof(certs_buffer)) - SIGNER_CERT_MAX_LEN - 1;
    device_cert = signer_cert - DEVICE_CERT_MAX_LEN - 1;

    // Uncompress the signer certificate from the ATECC608 device.
    signer_cert_size = SIGNER_CERT_MAX_LEN;
    atca_status = atcacert_read_cert(&g_cert_def_1_signer, (uint8_t *)g_cert_ca_public_key_1_signer, signer_cert, &signer_cert_size);
    if (atca_status != ATCACERT_E_SUCCESS) {
    	DPRINT_E(TAG, "Failed to get signer certificate : %d", atca_status);
        return -1;
    }

    // Get the signer's subject public key from its certificate.
    atca_status = atcacert_get_subj_public_key(&g_cert_def_1_signer, signer_cert, signer_cert_size, signer_public_key);
    if (atca_status != ATCACERT_E_SUCCESS) {
    	DPRINT_E(TAG, "Failed to get signer public key : %d", atca_status);
        return -1;
    }

    // Uncompress the device certificate from the ATECC608 device.
    device_cert_size = DEVICE_CERT_MAX_LEN;
    atca_status = atcacert_read_cert(&g_cert_def_2_device, signer_public_key, device_cert, &device_cert_size);
    if (atca_status != ATCACERT_E_SUCCESS) {
    	DPRINT_E(TAG, "Failed to get device certificate : %d", atca_status);
        return -1;
    }

    // Extract MQTT client ID from ATECC608 device serial number.
    memset(&serial_number[0], 0, sizeof(serial_number));
	if (atcab_read_serial_number(serial_number) != ATCA_SUCCESS) {
		DPRINT_E(TAG, "Failed to get device serial number : %d", atca_status);
		return -1;
	}
    uint8_t hex_str[]= "0123456789ABCDEF";
	for (i = 0; i < ATCA_SERIAL_NUM_SIZE; i++)  {
        devId[i * 2 + 0] = hex_str[(serial_number[i] >> 4) & 0x0F];
        devId[i * 2 + 1] = hex_str[(serial_number[i]) & 0x0F];
	}
	devId[i * 2] = 0;

	// Extract Company ID from ATECC608 device certificate.
	char *sn_offset = strstr((char *)device_cert, devId);
	char *companyId = 0;
	if (sn_offset == 0) {
		DPRINT_E(TAG, "Failed to extract Company ID from device certificate");
		return -1;
	}
	while (sn_offset > (char *)device_cert) {
		if ((sn_offset[0] == 0x55) && (sn_offset[1] == 0x04) && (sn_offset[2] == 0x03)) {
			companyId = sn_offset + 5;
			break;
		}
		sn_offset -= 1;
	}
	if (companyId == 0) {
		DPRINT_E(TAG, "Failed to extract Company ID from device certificate");
		return -1;
	}
	for (i = 0; companyId[i] && (companyId[i] != '-'); ++i) {
		cpId[i] = companyId[i];
	}
	cpId[i] = 0;

	return 0;
}

/**
 * \brief Open MQTT socket and perform a TLS handshake with specified remote server.
 *
 * \param[in] serv_addr URL or IP address of the remote server.
 * \param[in] serv_port Port number of the remote server.
 * \return 0 on success, an error code otherwise.
 */
int tls_mqtt_socket_open(const char *serv_addr, const char *serv_port)
{
    int ret;

    // Initialize mbedTLS.
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_x509_crt_init(&clicert);
    mbedtls_pk_init(&pkey);
	//mbedtls_net_init(&socket_fd);

    // Initialize random number generator.
    DPRINT_I(TAG, "Seeding the random number generator...");
    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) TAG, strlen(TAG))) != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_ctr_drbg_seed error %d", ret);
        return ret;
    }

    // Parse server root CA.
    DPRINT_I(TAG, "Loading the CA root certificate...");
    ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)msazure_rootca, strlen(msazure_rootca) + 1);
    if (ret < 0) {
    	DPRINT_E(TAG, "Failed mbedtls_x509_crt_parse error %d", ret);
        return ret;
    }

    // Parse client certificate chain.
    // Extract the device certificate and convert to mbedtls certificate.
    DPRINT_I(TAG, "Loading the device certificate...");
    ret = mbedtls_x509_crt_parse_der(&clicert, device_cert, device_cert_size);
    if (ret != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_x509_crt_parse error %d", ret);
        return ret;
    }
    // Extract the signer certificate, convert and attach to the certificate chain.
    ret = mbedtls_x509_crt_parse_der(&clicert, signer_cert, signer_cert_size);
    if (ret != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_x509_crt_parse error %d", ret);
        return ret;
    }

    // Use private key from ATECC608A secure element.
    if (0 != atca_mbedtls_pk_init(&pkey, 0)) {
    	DPRINT_E(TAG, "Failed to set ATECC608A private key!");
        return -1;
    }

    DPRINT_I(TAG, "Setting up the SSL/TLS structure...");
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_ssl_config_defaults error %d", ret);
    	tls_mqtt_socket_clean();
        return ret;
    }
    // Configure debug (from 1 warn to 4 verbose).
    //mbedtls_debug_set_threshold(4);
    //mbedtls_ssl_conf_dbg(&conf, mbedtls_bio_debug, NULL);
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    if ((ret = mbedtls_ssl_conf_own_cert(&conf, &clicert, &pkey)) != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_ssl_conf_own_cert error %d", ret);
    	tls_mqtt_socket_clean();
        return ret;
    }
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_read_timeout(&conf, 5000);
    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_ssl_setup error %d", ret);
    	tls_mqtt_socket_clean();
        return ret;
    }

    // Hostname must match with CN from server certificate.
    if ((ret = mbedtls_ssl_set_hostname(&ssl, serv_addr)) != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_ssl_set_hostname error %d", ret);
    	tls_mqtt_socket_clean();
        return ret;
    }
    mbedtls_ssl_set_bio(&ssl, &socket_fd, mbedtls_bio_send, mbedtls_bio_recv, NULL);

    // Open the TCP socket.
    if ((ret = mbedtls_bio_connect(serv_addr, serv_port)) != 0) {
    	DPRINT_E(TAG, "Failed mbedtls_bio_connect error %d", ret);
        tls_mqtt_socket_clean();
        return ret;
    }

    // Perform TLS handshake.
    DPRINT_I(TAG, "Performing the SSL/TLS handshake...");
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        	DPRINT_E(TAG, "Failed mbedtls_ssl_handshake error %d", ret);
            if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
            	DPRINT_E(TAG, "-> Unable to verify the server's certificate.");
            }
        	// Send TLS alert.
        	mbedtls_ssl_close_notify(&ssl);
            tls_mqtt_socket_close();
            return ret;
        }
    }
    
    // Print TLS cipher information.
    DPRINT_I(TAG, "Protocol: %s Ciphersuite: %s", mbedtls_ssl_get_version(&ssl), mbedtls_ssl_get_ciphersuite(&ssl));
    /*if ((ret = mbedtls_ssl_get_record_expansion(&ssl)) >= 0) {
    	DPRINT_I(TAG, "Record expansion is %d", ret);
    } 
    else {
    	DPRINT_I(TAG, "Record expansion is unknown (compression)");
    }*/

    return 0;
}

/**
 * \brief Read data from the MQTT socket.
 *
 * \param[in] network Paho MQTT network context data.
 * \param[out] read_buffer Buffer to store data read from socket.
 * \param[in] length Length of read_buffer.
 * \param[in] timeout_ms Timeout in milliseconds to wait for data.
 * \return 0 on success, an error code otherwise.
 */
int tls_mqtt_socket_read(Network *network, unsigned char *read_buffer, size_t length, int timeout_ms)
{
    uint32_t read_timeout;
    size_t rxLen = 0;
    int ret;
    Timer timer;
    
	TimerCountdownMS(&timer, timeout_ms);
    read_timeout = conf.read_timeout;
    while (length > 0) {
        // Make sure we don't block indefinitely.
        mbedtls_ssl_conf_read_timeout(&conf, (MAX(1, (MIN(read_timeout, (uint32_t)TimerLeftMS(&timer))))));
        ret = mbedtls_ssl_read(&ssl, read_buffer + rxLen, length);
        // Restore the old timeout.
        mbedtls_ssl_conf_read_timeout(&conf, read_timeout);        
        if (ret > 0) {
            rxLen += ret;
            length -= ret;
        }
        else if (ret == 0 || (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_TIMEOUT)) {
        	DPRINT_E(TAG, "mbedtls_ssl_read returned error %d", ret);
        	network->evt = MQTT_DISCONNECTED_EVENT;
            return MQTT_NETWORK_DISCONNECTED_ERROR;
        }

        // Evaluate timeout after the read to make sure read is done at least once.
        if (TimerIsExpired(&timer)) {
            break;
        }
    }

    if (length == 0) {
        return rxLen;
    }

    if (rxLen == 0) {
        return 0;
    } else {
        return MQTT_FAILURE;
    }
}

/**
 * \brief Write data to the MQTT socket.
 *
 * \param[in] network Paho MQTT network context data.
 * \param[in] send_buffer Buffer to send to the socket.
 * \param[in] length Length of send_buffer.
 * \param[in] timeout_ms Timeout in milliseconds to wait for data sent.
 * \return Number of bytes sent, an error code otherwise.
 */
int tls_mqtt_socket_write(Network *network, unsigned char *send_buffer, size_t length, int timeout_ms)
{
    int ret = 0;
    size_t sent;
    Timer timer;
    
	TimerCountdownMS(&timer, timeout_ms);
    for (sent = 0; sent < length && !TimerIsExpired(&timer); sent += ret) {
        while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)send_buffer + sent, length - sent)) <= 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            	DPRINT_E(TAG, "mbedtls_ssl_write returned error %d", ret);
            	network->evt = MQTT_DISCONNECTED_EVENT;
                return MQTT_NETWORK_DISCONNECTED_ERROR;
            }
        }
    }

    if (TimerIsExpired(&timer) && sent != length) {
        return MQTT_FAILURE;
    }

    return sent;
}

/**
 * \brief Close MQTT socket and clean mbedTLS associated resources.
 *
 * \return Response code.
 */
int tls_mqtt_socket_close(void)
{
    int ret = 0;

    // Close TCP socket.
    ret = mbedtls_bio_close();

    // Clean up data structure.
    mbedtls_ssl_session_reset(&ssl);
    tls_mqtt_socket_clean();
    return ret;
}

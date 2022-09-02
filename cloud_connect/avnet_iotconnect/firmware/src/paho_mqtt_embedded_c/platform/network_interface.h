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

#ifndef __MQTT_NETWORK_INTERFACE_H__
#define __MQTT_NETWORK_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_DISCONNECTED_EVENT   (1)

typedef struct mqtt_network {
	int (*mqttread)(struct mqtt_network *network, unsigned char *read_buffer, size_t length, int timeout_ms);
	int (*mqttwrite)(struct mqtt_network *network, unsigned char *send_buffer, size_t length, int timeout_ms);
	uint32_t evt;
} Network;

int tls_mqtt_socket_init_certificates(char *devId, char *cpId);
int tls_mqtt_socket_open(const char *serv_addr, const char *serv_port);
int tls_mqtt_socket_close(void);
int tls_mqtt_socket_read(Network *network, unsigned char *read_buffer, size_t length, int timeout_ms);
int tls_mqtt_socket_write(Network *network, unsigned char *send_buffer, size_t length, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_NETWORK_INTERFACE_H__ */

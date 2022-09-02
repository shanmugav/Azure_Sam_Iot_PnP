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

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes

/** Debug includes. */
#include "dprint/dprint.h"

/** Heracles includes. */
#include "heracles/gsm.h"
#include "heracles/gsm_socket.h"
#include "heracles/gsm_sntp.h"
#include "heracles/gsm_config.h"

/** Cryptoauthlib includes. */
#include "cryptoauthlib.h"

/** Sensors includes. */
#include "sensors.h"

/** MQTT client includes. */
#include "MQTTClient.h"

#define STRING_EOL    "\r\n"
#define STRING_HEADER STRING_EOL "-- EBVchips Heracles TrustFLEX IOTConnect Demo --"STRING_EOL \
"-- SAMV71 Xplained Ultra --"STRING_EOL	\
"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL STRING_EOL

/** Microchip ATECC608A Provisioning Type. */
#define ATECC608A_TNGTLS                    (0x6A)
#define ATECC608A_TFLXTLS                   (0x6C)
#define ATECC608A_TCSM                      (0xC0)

/** IoTConnect Demo Firmware version. */
#define IOTCONNECT_DEMO_VERSION             "1.00"

/** IoTConnect Host Server (must be configured). */
#define IOTCONNECT_CONF_SERVER_ADDRESS      "poc-iotconnect-iothub-eu.azure-devices.net"

/** IoTConnect Device Template ID (must be configured). */
#define IOTCONNECT_CONF_DEVICE_DTG          "E962322D-EEA9-465A-87F0-643F7A91331A"

/** IoTConnect Definitions. */
#define IOTCONNECT_PROT_ENVIRONMENT         "AVNET"
#define IOTCONNECT_PROT_VERSION             "2.0"
#define IOTCONNECT_PROT_LANG                "M_C"
#define IOTCONNECT_PROT_DEVICEID_MAX_LEN    (19)
#define IOTCONNECT_PROT_HOST_MAX_LEN        (64)
#define IOTCONNECT_PROT_CLIENTID_MAX_LEN    (84)
#define IOTCONNECT_PROT_USERNAME_MAX_LEN    (160)
#define IOTCONNECT_PROT_PASSWORD_MAX_LEN    (256)
#define IOTCONNECT_PROT_PUBTOPIC_MAX_LEN    (128)
#define IOTCONNECT_PROT_SUBTOPIC_MAX_LEN    (128)
#define IOTCONNECT_DATA_CPID_MAX_LEN        (64)
#define IOTCONNECT_DATA_TG_MAX_LEN          (64)
#define IOTCONNECT_DATA_OTA_ACKID_MAX_LEN   (40)
#define IOTCONNECT_DATA_OTA_URL_MAX_LEN     (256)
#define IOTCONNECT_ERR_RETRY_TIMEOUT        (8000)

/** IoTConnect Status Code. */
#define IOTCONNECT_STATUS_OTA_SUCCESS       (7)
#define IOTCONNECT_STATUS_OTA_PENDING       (11)
#define IOTCONNECT_STATUS_OTA_FAILED        (4)
#define IOTCONNECT_STATUS_CMD_SUCCESS       (6)
#define IOTCONNECT_STATUS_CMD_PENDING       (5)
#define IOTCONNECT_STATUS_CMD_FAILED        (4)

/** MQTT Settings. */
#define MQTT_BUFFER_SIZE                    (1024)
#define MQTT_DEVICE_REPORT_MSEC             (20000)
#define MQTT_COMMAND_TIMEOUT_MSEC           (5000)
#define MQTT_YIELD_TIMEOUT_MSEC             (500)
#define MQTT_KEEP_ALIVE_INTERVAL_SEC        (900)

/** IoTConnect MQTTS types definition. */
typedef struct iot_prot {
	char host[IOTCONNECT_PROT_HOST_MAX_LEN];
	char clientid[IOTCONNECT_PROT_CLIENTID_MAX_LEN];
	char username[IOTCONNECT_PROT_USERNAME_MAX_LEN];
	char password[IOTCONNECT_PROT_PASSWORD_MAX_LEN];
	char pubtopic[IOTCONNECT_PROT_PUBTOPIC_MAX_LEN];
	char subtopic[IOTCONNECT_PROT_SUBTOPIC_MAX_LEN];
} iot_prot;
typedef struct iot_data {
	char devId[IOTCONNECT_PROT_DEVICEID_MAX_LEN];
	char cpId[IOTCONNECT_DATA_CPID_MAX_LEN];
	char templateguid[IOTCONNECT_DATA_TG_MAX_LEN];
	char otaAckId[IOTCONNECT_DATA_OTA_ACKID_MAX_LEN];
	char otaURL[IOTCONNECT_DATA_OTA_URL_MAX_LEN];
	int32_t rc;
	int32_t ee;
	int32_t connected;
	int32_t otaActive;
	int32_t otaStatus;
	iot_prot broker;
} iot_data;

/** IoTConnect MQTTS global variables. */
static iot_data g_iot_data = {0};
static uint16_t g_m_id = 0;

/** MQTT client definition. */
static MQTTClient g_mqtt_client;
static uint8_t g_mqtt_rx_buffer[MQTT_BUFFER_SIZE];
static uint8_t g_mqtt_tx_buffer[MQTT_BUFFER_SIZE];

/** Debug tag prefix definition. */
static const char *TAG = "appdemo";

/** SysTick counter to avoid busy wait delay. */
volatile uint32_t ms_ticks = 0;

/**
 * \brief Initialize the ATECC608A secure element. 
 */
static ATCA_STATUS initialize_atecc608a(void)
{
	int ret = ATCA_SUCCESS;
	bool lock = 0;
	uint8_t buf[ATCA_ECC_CONFIG_SIZE];

	// ATECC608A device is defined as global variable. Set target I2C address.
	DPRINT_I(TAG, "initialize_atecc608a: loading ATECC608A driver...");
	cfg_ateccx08a_i2c_default.iface_type             = ATCA_I2C_IFACE;
	cfg_ateccx08a_i2c_default.devtype                = ATECC608A;
	cfg_ateccx08a_i2c_default.atcai2c.slave_address  = ATECC608A_TFLXTLS;
	cfg_ateccx08a_i2c_default.atcai2c.bus            = 0;
	cfg_ateccx08a_i2c_default.atcai2c.baud           = 400000;
	cfg_ateccx08a_i2c_default.wake_delay             = 1500;
	cfg_ateccx08a_i2c_default.rx_retries             = 20;
	if (0 != (ret = atcab_init(&cfg_ateccx08a_i2c_default))) {
		DPRINT_E(TAG, "initialize_atecc608a: failed to start ATECC608A ERROR 0x%02x", ret);
		return ret;
	}

	if (0 != (ret = atcab_is_locked(LOCK_ZONE_DATA, &lock))) {
		DPRINT_E(TAG, "initialize_atecc608a: failed to check status ERROR 0x%02x", ret);
		return ret;
	}

	if (0 != (ret = atcab_info(buf))) {
		DPRINT_E(TAG, "initialize_atecc608a: failed to read device info ERROR 0x%02x", ret);
		return ret;
	}
	DPRINT_I(TAG, "initialize_atecc608a: read device info {%02x}{%02x}", buf[2], buf[3]);
	DPRINT_I(TAG, "initialize_atecc608a: data zone is %s", lock ? "LOCKED" : "UNLOCKED");
	return ret;
}

/**
 * \brief IoTConnect function to update twin property in the cloud. 
 */
static int32_t iotconnect_send_twin(const char *key, const char *value)
{
	char packet[MQTT_BUFFER_SIZE];

	/* Prepare report and read sensor data. */
	memset(packet, 0, MQTT_BUFFER_SIZE);

	/* Build update packet. */
	sprintf(packet, "{\"%s\":\"%s\"}", key, value);

	MQTTMessage message;
	message.qos      = QOS1;
	message.retained = 0;
	message.dup      = 0;
	message.id       = g_m_id++;
	message.payload = packet;
	message.payloadlen = strlen(packet);

	/* Send MQTT sensor report. */
	DPRINT_I(TAG, "Sending Twin update...");
	return MQTTPublish(&g_mqtt_client, "$iothub/twin/PATCH/properties/reported/?$rid=1", &message);
}

/**
 * \brief IoTConnect function to read incoming messages from the cloud. 
 */
static void iotconnect_mqtt_callback(MessageData *msg)
{
	/* Print received MQTT message. */
	char *str = msg->message->payload;
	str[msg->message->payloadlen] = 0;
	DPRINT_W(TAG, "iotconnect_mqtt_callback: RCV |%s|\r\n", str);
    
    /* Process led command. */
    if (strstr(str, "\"led on\"") || strstr(str, "\"led ON\"") || strstr(str, "\"led 1\"")) {
        BOARD_LED_Clear();
    }
    if (strstr(str, "\"led off\"") || strstr(str, "\"led OFF\"") || strstr(str, "\"led 0\"")) {
        BOARD_LED_Set();
    }
}

/**
 * \brief SysTick handler used to measure accurate delay. 
 */
void systick_count(unsigned int ctx)
{
	ms_ticks++;
}

int _gettimeofday(struct timeval *tv, void *tzvp);
int _gettimeofday(struct timeval *tv, void *tzvp)
{
	tv->tv_sec = ms_ticks / 1000;  // convert to seconds
	tv->tv_usec = ( ms_ticks % 1000 ) / 1000;  // get remaining microseconds
	return 0;  // return non-zero for error
}

int main ( void )
{
	Network g_mqtt_network;
	MQTTPacket_connectData mqtt_options = MQTTPacket_connectData_initializer;
	uint32_t sensor_enabled = 1;
	uint32_t tick_count_report = 0;
	uint32_t clock_sync = 0;
	char report[512];
	char curtime[25];
	int report_len = 0;
	int rc = 0;
	int rssi = 0;

    /* Initialize all modules */
    SYS_Initialize ( NULL );
    BOARD_LED_Set();
    
    /* Enable SysTick timer. */
    SYSTICK_TimerInitialize();
    SYSTICK_TimerCallbackSet(systick_count, 0);
    SYSTICK_TimerStart();
    
    /* Open debug port. */
	dprint_port_open();
	DPRINT_RI(STRING_HEADER);
	DPRINT_W(TAG, "EBV IoTConnect current Firmware v%s", IOTCONNECT_DEMO_VERSION);
	DPRINT_I(TAG, "EBV IoTConnect SDK v%s (%s)", IOTCONNECT_PROT_VERSION, IOTCONNECT_PROT_ENVIRONMENT);

    /* Initialize ATECC608A driver. */
	if (initialize_atecc608a() != ATCA_SUCCESS) {
		while (1);
	}

	/* Enable I2C MEMS sensors (I2C bus already initialized by ATECC608A). */
	if (Sensors_Init() != COMPONENT_OK) {
		DPRINT_W(TAG, "WARNING sensors acquisition disabled!");
		sensor_enabled = 0;
	}
	else {
		DPRINT_W(TAG, "WARNING remove R602 on the board to read HTS221 sensor value!");
	}
    
	/* Fetch certificates and ID from ATECC608 and display IoTConnect settings. */
	if (tls_mqtt_socket_init_certificates(g_iot_data.devId, g_iot_data.cpId) < 0) {
		while (1);
	}
	sprintf(g_iot_data.broker.clientid, "%s-%s", g_iot_data.cpId, g_iot_data.devId);
	sprintf(g_iot_data.broker.username, "%s/%s/?api-version=2018-06-30", IOTCONNECT_CONF_SERVER_ADDRESS, g_iot_data.broker.clientid);
	strcpy(g_iot_data.broker.host, IOTCONNECT_CONF_SERVER_ADDRESS);
	strcpy(g_iot_data.templateguid, IOTCONNECT_CONF_DEVICE_DTG);
	sprintf(g_iot_data.broker.pubtopic, "devices/%s/messages/events/", g_iot_data.broker.clientid);
	sprintf(g_iot_data.broker.subtopic, "devices/%s/messages/devicebound/#", g_iot_data.broker.clientid);
	DPRINT_RI("\r\n");
	DPRINT_I(TAG, ">> *** Reading IoTConnect device certificate information *** <<");
	DPRINT_I(TAG, "Company ID: {%s}", g_iot_data.cpId);
	DPRINT_I(TAG, "Device ID: {%s}\r\n", g_iot_data.devId);

	/* Initialize Heracles driver. */
	DPRINT_I(TAG, "Starting Heracles driver...");
	while (gsm_init(GSM_SIM_INTERNAL, "") != GSM_RESP_OK) {
		DPRINT_E(TAG, "Failed to start Heracles driver!");
		SYSTICK_DelayMs(1000);
	}
	DPRINT_I(TAG, "Heracles module started.\r\n");

    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );

		/* Socket is not connected. */
		if (g_iot_data.connected == 0) {

			/* Wait for GSM network access. */
			rc = gsm_get_csq_status(&rssi, 10000);
			if (rc == GSM_RESP_TIMEOUT) {
				DPRINT_W(TAG, "Heracles timeout, reseting...");
				while (gsm_init(GSM_SIM_INTERNAL, "") != GSM_RESP_OK) {
					SYSTICK_DelayMs(1000);
				}
				continue;
			}
			else if (rc != GSM_RESP_OK) {
				DPRINT_I(TAG, "Waiting for signal...");
				SYSTICK_DelayMs(2000);
				continue;
			}
			else {
				DPRINT_I(TAG, "Current RSSI level: -%d dBm", rssi);
			}
			DPRINT_I(TAG, "Checking for GSM network...");
			if (gsm_get_reg_status(2000) != GSM_RESP_OK) {
				DPRINT_W(TAG, "No GSM network found!\r\n");
				SYSTICK_DelayMs(2000);
				continue;
			}
			else {
				DPRINT_I(TAG, "GSM network available.");
			}

			/* Wait for GPRS service attached. */
			DPRINT_I(TAG, "Checking for GPRS service...");
			if (gsm_get_gatt_status(1000) != GSM_RESP_OK) {
				DPRINT_W(TAG, "GPRS service not available!\r\n");
				SYSTICK_DelayMs(2000);
				continue;
			}
			else {
				DPRINT_I(TAG, "GPRS service attached.\r\n");
			}

			/* Initialize MQTT client. */
			g_mqtt_network.mqttread  = &tls_mqtt_socket_read;
			g_mqtt_network.mqttwrite = &tls_mqtt_socket_write;
			g_mqtt_network.evt = 0;
			MQTTClientInit(&g_mqtt_client, &g_mqtt_network, MQTT_COMMAND_TIMEOUT_MSEC,
					g_mqtt_tx_buffer, sizeof(g_mqtt_tx_buffer),
					g_mqtt_rx_buffer, sizeof(g_mqtt_rx_buffer));

			/* Open TLS socket. */
			DPRINT_I(TAG, "Connecting to %s:8883...", g_iot_data.broker.host);
			if ((rc = tls_mqtt_socket_open(g_iot_data.broker.host, "8883")) != MQTT_SUCCESS) {
				DPRINT_E(TAG, "Failed to connect to MQTT server!\r\n");
				SYSTICK_DelayMs(3000);
				continue;
			}

			/* Register to MQTT server. */
			mqtt_options.keepAliveInterval = MQTT_KEEP_ALIVE_INTERVAL_SEC;
			mqtt_options.cleansession = 1;
			mqtt_options.clientID.cstring = g_iot_data.broker.clientid;
			mqtt_options.username.cstring = g_iot_data.broker.username;
			if ((rc = MQTTConnect(&g_mqtt_client, &mqtt_options)) != MQTT_SUCCESS) {
				DPRINT_E(TAG, "Failed to login to MQTT server!\r\n");
				tls_mqtt_socket_close();
				SYSTICK_DelayMs(3000);
				continue;
			}

			/* Subscribe to IOTConnect device update topic. */
			rc = MQTTSubscribe(&g_mqtt_client, g_iot_data.broker.subtopic, QOS0, &iotconnect_mqtt_callback);
			if (rc != MQTT_SUCCESS) {
				DPRINT_E(TAG, "Failed to subscribe to MQTT topic!\r\n");
				tls_mqtt_socket_close();
				SYSTICK_DelayMs(3000);
				continue;
			}

			g_iot_data.connected = 1;
			DPRINT_I(TAG, "Welcome to IoTConnect!\r\n");

			/* Send firmware version to cloud. */
			iotconnect_send_twin("fw", IOTCONNECT_DEMO_VERSION);

			/* Initialize and synchronize clock using NTP (need to be done only once). */
			if (clock_sync == 0) {
				gsm_sntp_init(0);
				clock_sync = 1;
			}
		}

		/* Socket is connected to IOTConnect. */
		if (g_iot_data.connected) {

			/* Read for incoming messages. */
			if ((rc = MQTTYield(&g_mqtt_client, MQTT_YIELD_TIMEOUT_MSEC)) != MQTT_SUCCESS) {
				DPRINT_E(TAG, "Failed to read MQTT socket!");
				DPRINT_E(TAG, "Connection lost.\r\n");
				tls_mqtt_socket_close();
				g_iot_data.connected = 0;
				SYSTICK_DelayMs(3000);
				continue;
			}

			/* Send sensor data to the IOTConnect cloud. */
			if ((ms_ticks - tick_count_report) > (MQTT_DEVICE_REPORT_MSEC)) {

				/* Read sensor values if ST MEMS extension is connected. */
				if (sensor_enabled) {
					Sensors_Accelero_Handler();
					Sensors_Gyro_Handler();
					Sensors_Magneto_Handler();
					Sensors_Pressure_Handler();
					Sensors_Humidity_Handler();
					Sensors_Temperature_Handler();
				}

				/* Build a JSON report that contains sensor values. */
				gsm_sntp_get_time(curtime);
				report_len = sprintf(report, "{\"cpId\":\"%s\",\"dtg\":\"%s\",\"t\":\"%s\",\"mt\":0,\"sdk\":{\"l\":\""IOTCONNECT_PROT_LANG"\",\
						\"v\":\""IOTCONNECT_PROT_VERSION"\",\"e\":\""IOTCONNECT_PROT_ENVIRONMENT"\"},\"d\":[{\"id\":\"%s\",\"tg\":\"\",\
						\"dt\":\"%s\",\"d\":[{\"acc\":{\"X\":%d,\"Y\":%d,\"Z\":%d},\"gyro\":{\"X\":%d,\"Y\":%d,\"Z\":%d},\
						\"mag\":{\"X\":%d,\"Y\":%d,\"Z\":%d},\"press\":\"%d\", \"hum\":\"%d\", \"temp\":\"%d\"}]}]}",
						g_iot_data.cpId, g_iot_data.templateguid, curtime, g_iot_data.devId, curtime,
						(int)ACCELERO_Data.AXIS_X, (int)ACCELERO_Data.AXIS_Y, (int)ACCELERO_Data.AXIS_Z,
						(int)GYRO_Data.AXIS_X, (int)GYRO_Data.AXIS_Y, (int)GYRO_Data.AXIS_Z,
						(int)MAGNETO_Data.AXIS_X, (int)MAGNETO_Data.AXIS_Y, (int)MAGNETO_Data.AXIS_Z,
						(int)PRESSURE_Data,
						(int)HUMIDITY_Data,
						(int)TEMPERATURE_Data);
				MQTTMessage message;
				message.qos        = QOS0;
				message.retained   = 0;
				message.dup        = 0;
				message.id         = g_m_id++;
				message.payload    = report;
				message.payloadlen = report_len;

				/* Publish the sensor report. */
				DPRINT_I(TAG, "Publishing MQTT report...");
				if ((rc = MQTTPublish(&g_mqtt_client, g_iot_data.broker.pubtopic, &message)) != MQTT_SUCCESS) {
					DPRINT_E(TAG, "Failed to write MQTT socket!");
					DPRINT_E(TAG, "Connection lost.\r\n");
					tls_mqtt_socket_close();
					g_iot_data.connected = 0;
					SYSTICK_DelayMs(3000);
					continue;
				}

				/* Print sensor report values. */
				DPRINT_I(TAG, "---> Accelero:     %8d %8d %8d mg", ACCELERO_Data.AXIS_X, ACCELERO_Data.AXIS_Y, ACCELERO_Data.AXIS_Z);
				DPRINT_I(TAG, "---> Gyro:         %8d %8d %8d mdps", GYRO_Data.AXIS_X, GYRO_Data.AXIS_Y, GYRO_Data.AXIS_Z);
				DPRINT_I(TAG, "---> Magneto:      %8d %8d %8d mgauss", MAGNETO_Data.AXIS_X, MAGNETO_Data.AXIS_Y, MAGNETO_Data.AXIS_Z);
				DPRINT_I(TAG, "---> Pressure:     %8d hPa", (uint16_t)PRESSURE_Data);
				DPRINT_I(TAG, "---> Humidity:     %8d %%", (uint16_t)HUMIDITY_Data);
				DPRINT_I(TAG, "---> Temperature:  %8d °C", (uint16_t)TEMPERATURE_Data);

				tick_count_report = ms_ticks;
				DPRINT_I(TAG, "Done.\r\n");
			}
		}
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/


/**
  ******************************************************************************
  * @file    sensors.c
  * @brief   Sensors implementation.
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT(c) 2018 STMicroelectronics - EBV Elektronik
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "dprint/dprint.h"

#include "sensors.h"
#include "twihs_asf.h"
#include "LSM6DSL_ACC_GYRO_driver_HL.h"
#include "LIS2MDL_MAG_driver_HL.h"
#include "LPS22HB_Driver_HL.h"
#include "HTS221_Driver_HL.h"

/** Debug tag prefix definition. */
static const char *TAG = "sensors";

SensorAxes_t ACCELERO_Data = { 0, 0, 0 };
SensorAxes_t GYRO_Data = { 0, 0, 0 };
SensorAxes_t MAGNETO_Data = { 0, 0, 0 };
float PRESSURE_Data = 0;
float HUMIDITY_Data = 0;
float TEMPERATURE_Data = 0;

ACCELERO_Drv_t* ACCELERO_Driver = &LSM6DSL_X_Drv;
LSM6DSL_X_Data_t LSM6DSL_X_0_Data;
ACCELERO_Data_t ACCELERO_X_0_Data = {
		.pComponentData = ( void * )&LSM6DSL_X_0_Data,
		.pExtData       = 0,
};
DrvContextTypeDef ACCELERO_Handle = {
		.who_am_i      = LSM6DSL_ACC_GYRO_WHO_AM_I,
		.address       = LSM6DSL_ACC_GYRO_I2C_ADDRESS_HIGH,
		.instance      = 0,
		.isInitialized = 0,
		.isEnabled     = 0,
		.isCombo       = 1,
		.pData         = ( void * )&ACCELERO_X_0_Data,
		.pVTable       = ( void * )&LSM6DSL_X_Drv,
		.pExtVTable    = ( void * )&LSM6DSL_X_ExtDrv,
};

GYRO_Drv_t* GYRO_Driver = &LSM6DSL_G_Drv;
LSM6DSL_G_Data_t LSM6DSL_G_0_Data;
GYRO_Data_t GYRO_G_0_Data = {
		.pComponentData = ( void * )&LSM6DSL_G_0_Data,
		.pExtData       = 0,
};
DrvContextTypeDef GYRO_Handle = {
		.who_am_i      = LSM6DSL_ACC_GYRO_WHO_AM_I,
		.address       = LSM6DSL_ACC_GYRO_I2C_ADDRESS_HIGH,
		.instance      = 0,
		.isInitialized = 0,
		.isEnabled     = 0,
		.isCombo       = 1,
		.pData         = ( void * )&GYRO_G_0_Data,
		.pVTable       = ( void * )&LSM6DSL_G_Drv,
		.pExtVTable    = ( void * )&LSM6DSL_G_ExtDrv,
};

MAGNETO_Drv_t* MAGNETO_Driver = &LIS2MDLDrv;
LIS2MDL_Data_t LIS2MDL_0_Data;
MAGNETO_Data_t MAGNETO_0_Data = {
		.pComponentData = ( void * )&LIS2MDL_0_Data,
		.pExtData       = 0,
};
DrvContextTypeDef MAGNETO_Handle = {
		.who_am_i      = LIS2MDL_MAG_WHO_AM_I,
		.address       = LIS2MDL_MAG_I2C_ADDRESS,
		.instance      = 0,
		.isInitialized = 0,
		.isEnabled     = 0,
		.isCombo       = 0,
		.pData         = ( void * )&MAGNETO_0_Data,
		.pVTable       = ( void * )&LIS2MDLDrv,
		.pExtVTable    = 0,
};

PRESSURE_Drv_t* PRESSURE_Driver = &LPS22HB_P_Drv;
LPS22HB_Combo_Data_t LPS22HB_Combo_Data[LPS22HB_SENSORS_MAX_NUM];
LPS22HB_P_Data_t LPS22HB_P_0_Data = {
		.comboData = &(LPS22HB_Combo_Data[0]),
};
PRESSURE_Data_t PRESSURE_P_0_Data = {
		.pComponentData = ( void * )&LPS22HB_P_0_Data,
		.pExtData       = 0,
};
DrvContextTypeDef PRESSURE_Handle = {
		.who_am_i      = LPS22HB_WHO_AM_I_VAL,
		.address       = LPS22HB_ADDRESS_HIGH,
		.instance      = 0,
		.isInitialized = 0,
		.isEnabled     = 0,
		.isCombo       = 1,
		.pData         = ( void * )&PRESSURE_P_0_Data,
		.pVTable       = ( void * )&LPS22HB_P_Drv,
		.pExtVTable    = ( void * )&LPS22HB_P_ExtDrv,
};

HUMIDITY_Drv_t* HUMIDITY_Driver = &HTS221_H_Drv;
HTS221_Combo_Data_t HTS221_Combo_Data[HTS221_SENSORS_MAX_NUM];
HTS221_H_Data_t HTS221_H_0_Data = {
		.comboData = &(HTS221_Combo_Data[0]),
};
HUMIDITY_Data_t HUMIDITY_H_0_Data = {
		.pComponentData = ( void * )&HTS221_H_0_Data,
		.pExtData       = 0,
};
DrvContextTypeDef HUMIDITY_Handle = {
		.who_am_i      = HTS221_WHO_AM_I_VAL,
		.address       = HTS221_ADDRESS_DEFAULT,
		.instance      = 0,
		.isInitialized = 0,
		.isEnabled     = 0,
		.isCombo       = 1,
		.pData         = ( void * )&HUMIDITY_H_0_Data,
		.pVTable       = ( void * )&HTS221_H_Drv,
		.pExtVTable    = 0,
};

TEMPERATURE_Drv_t* TEMPERATURE_Driver = &HTS221_T_Drv;
HTS221_T_Data_t HTS221_T_0_Data = {
		.comboData = &(HTS221_Combo_Data[0]),
};
TEMPERATURE_Data_t TEMPERATURE_T_0_Data = {
		.pComponentData = ( void * )&HTS221_T_0_Data,
		.pExtData       = 0,
};
DrvContextTypeDef TEMPERATURE_Handle = {
		.who_am_i      = HTS221_WHO_AM_I_VAL,
		.address       = HTS221_ADDRESS_DEFAULT,
		.instance      = 0,
		.isInitialized = 0,
		.isEnabled     = 0,
		.isCombo       = 1,
		.pData         = ( void * )&TEMPERATURE_T_0_Data,
		.pVTable       = ( void * )&HTS221_T_Drv,
		.pExtVTable    = 0,
};

#define SENSORS_I2C_MASTER (TWIHS0_REGS)

uint8_t Sensor_IO_Write(void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite)
{
	DrvContextTypeDef *ctx = (DrvContextTypeDef *)handle;
	
    twihs_packet_t packet = {
	    .addr[0]        = WriteAddr,
	    .addr[1]        = 0,
	    .addr_length    = 1,
	    .chip           = ctx->address >> 1,
	    .buffer         = pBuffer,
	    .length         = nBytesToWrite,
    };

    return twihs_master_write(SENSORS_I2C_MASTER, &packet);
}

uint8_t Sensor_IO_Read(void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead)
{
	DrvContextTypeDef *ctx = (DrvContextTypeDef *)handle;

    twihs_packet_t packet = {
	    .addr[0]        = ReadAddr,
	    .addr[1]        = 0,
	    .addr_length    = 1,
	    .chip           = ctx->address >> 1,
	    .buffer         = pBuffer,
	    .length         = nBytesToRead,
    };

	return twihs_master_read(SENSORS_I2C_MASTER, &packet);
}

uint8_t Sensors_Init(void)
{
	uint8_t status = COMPONENT_OK;

	DPRINT_I(TAG, "Sensors_Init: initializing I2C sensors...");

	/* Init accelero sensor. */
	if (ACCELERO_Driver->Init((void *)&ACCELERO_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_Init: error LSM6DSL_X sensor not found!");
		status = COMPONENT_ERROR;
	}

	/* Init gyro sensor. */
	if (GYRO_Driver->Init((void *)&GYRO_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_Init: error LSM6DSL_G sensor not found!");
		status = COMPONENT_ERROR;
	}

	/* Init magneto sensor. */
	if (MAGNETO_Driver->Init((void *)&MAGNETO_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_Init: error LIS2MDL sensor not found!");
		status = COMPONENT_ERROR;
	}

	/* Init pressure sensor. */
	if (PRESSURE_Driver->Init((void *)&PRESSURE_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_Init: error LPS22HB sensor not found!");
		status = COMPONENT_ERROR;
	}

	/* Init humidity sensor. */
	if (HUMIDITY_Driver->Init((void *)&HUMIDITY_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_Init: error HTS221_H sensor not found!");
		status = COMPONENT_ERROR;
	}

	/* Init temperature sensor. */
	if (TEMPERATURE_Driver->Init((void *)&TEMPERATURE_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_Init: error HTS221_T sensor not found!");
		status = COMPONENT_ERROR;
	}

	return status;
}

uint8_t Sensors_DeInit(void)
{
	uint8_t status = COMPONENT_OK;

	/* Deinit accelero sensor. */
	if (ACCELERO_Driver->DeInit((void *)&ACCELERO_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_DeInit: LSM6DSL_X deinit error!");
		status = COMPONENT_ERROR;
	}

	/* Deinit gyro sensor. */
	if (GYRO_Driver->DeInit((void *)&GYRO_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_DeInit: LSM6DSL_G deinit error!");
		status = COMPONENT_ERROR;
	}

	/* Deinit magneto sensor. */
	if (MAGNETO_Driver->DeInit((void *)&MAGNETO_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_DeInit: LIS2MDL deinit error!");
		status = COMPONENT_ERROR;
	}

	/* Deinit pressure sensor. */
	if (PRESSURE_Driver->DeInit((void *)&PRESSURE_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_DeInit: LPS22HB deinit error!");
		status = COMPONENT_ERROR;
	}

	/* Deinit humidity sensor. */
	if (HUMIDITY_Driver->DeInit((void *)&HUMIDITY_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_DeInit: HTS221_H deinit error!");
		status = COMPONENT_ERROR;
	}

	/* Deinit temperature sensor. */
	if (TEMPERATURE_Driver->DeInit((void *)&TEMPERATURE_Handle) != COMPONENT_OK) {
		DPRINT_E(TAG, "Sensors_DeInit: HTS221_T deinit error!");
		status = COMPONENT_ERROR;
	}

	return status;
}

uint8_t Sensors_Accelero_Handler(void)
{
	uint8_t status = 0;
	int count = 0;

	/* Enable sensor. */
	ACCELERO_Driver->Sensor_Enable(&ACCELERO_Handle);
	/* Read once to clear DRDY. */
	ACCELERO_Driver->Get_Axes(&ACCELERO_Handle, &ACCELERO_Data);

	/* Wait for data ready. */
	while (ACCELERO_Driver->Get_DRDY_Status(&ACCELERO_Handle, &status) == COMPONENT_OK
			&& status == 0
			&& count++ < SENSOR_POLLING) {
	}

	if (count < SENSOR_POLLING) {
		if (ACCELERO_Driver->Get_Axes(&ACCELERO_Handle, &ACCELERO_Data) == COMPONENT_OK) {
			status = COMPONENT_OK;
		}
		else {
			status = COMPONENT_ERROR;
		}
	}
	else {
		status = COMPONENT_TIMEOUT;
	}

	/* Disable sensor. */
	ACCELERO_Driver->Sensor_Disable(&ACCELERO_Handle);
	return status;
}

uint8_t Sensors_Gyro_Handler(void)
{
	uint8_t status = 0;
	int count = 0;

	/* Enable sensor. */
	GYRO_Driver->Sensor_Enable(&GYRO_Handle);
	/* Read once to clear DRDY. */
	GYRO_Driver->Get_Axes(&GYRO_Handle, &GYRO_Data);

	/* Wait for data ready. */
	while (GYRO_Driver->Get_DRDY_Status(&GYRO_Handle, &status) == COMPONENT_OK
			&& status == 0
			&& count++ < SENSOR_POLLING) {
	}

	if (count < SENSOR_POLLING) {
		if (GYRO_Driver->Get_Axes(&GYRO_Handle, &GYRO_Data) == COMPONENT_OK) {
			status = COMPONENT_OK;
		}
		else {
			status = COMPONENT_ERROR;
		}
	}
	else {
		status = COMPONENT_TIMEOUT;
	}

	/* Disable sensor. */
	GYRO_Driver->Sensor_Disable(&GYRO_Handle);
	return status;
}

uint8_t Sensors_Magneto_Handler(void)
{
	uint8_t status = 0;
	int count = 0;

	/* Enable sensor. */
	MAGNETO_Driver->Sensor_Enable(&MAGNETO_Handle);
	/* Read once to clear DRDY. */
	MAGNETO_Driver->Get_Axes(&MAGNETO_Handle, &MAGNETO_Data);

	/* Wait for data ready. */
	while (MAGNETO_Driver->Get_DRDY_Status(&MAGNETO_Handle, &status) == COMPONENT_OK
			&& status == 0
			&& count++ < SENSOR_POLLING) {
	}

	if (count < SENSOR_POLLING) {
		if (MAGNETO_Driver->Get_Axes(&MAGNETO_Handle, &MAGNETO_Data) == COMPONENT_OK) {
			status = COMPONENT_OK;
		}
		else {
			status = COMPONENT_ERROR;
		}
	}
	else {
		status = COMPONENT_TIMEOUT;
	}

	/* Disable sensor. */
	MAGNETO_Driver->Sensor_Disable(&MAGNETO_Handle);
	return status;
}

uint8_t Sensors_Pressure_Handler(void)
{
	uint8_t status = 0;
	int count = 0;

	/* Enable sensor. */
	PRESSURE_Driver->Sensor_Enable(&PRESSURE_Handle);
	/* Read once to clear DRDY. */
	PRESSURE_Driver->Get_Press(&PRESSURE_Handle, &PRESSURE_Data);

	/* Wait for data ready. */
	while (PRESSURE_Driver->Get_DRDY_Status(&PRESSURE_Handle, &status) == COMPONENT_OK
			&& status == 0
			&& count++ < SENSOR_POLLING) {
	}

	if (count < SENSOR_POLLING) {
		if (PRESSURE_Driver->Get_Press(&PRESSURE_Handle, &PRESSURE_Data) == COMPONENT_OK) {
			status = COMPONENT_OK;
		}
		else {
			status = COMPONENT_ERROR;
		}
	}
	else {
		status = COMPONENT_TIMEOUT;
	}

	/* Disable sensor. */
	PRESSURE_Driver->Sensor_Disable(&PRESSURE_Handle);
	return status;
}

uint8_t Sensors_Humidity_Handler(void)
{
	uint8_t status = 0;
	int count = 0;

	/* Enable sensor. */
	HUMIDITY_Driver->Sensor_Enable(&HUMIDITY_Handle);
	/* Read once to clear DRDY. */
	HUMIDITY_Driver->Get_Hum(&HUMIDITY_Handle, &HUMIDITY_Data);

	/* Wait for data ready. */
	while (HUMIDITY_Driver->Get_DRDY_Status(&HUMIDITY_Handle, &status) == COMPONENT_OK
			&& status == 0
			&& count++ < SENSOR_POLLING) {
	}

	if (count < SENSOR_POLLING) {
		if (HUMIDITY_Driver->Get_Hum(&HUMIDITY_Handle, &HUMIDITY_Data) == COMPONENT_OK) {
			status = COMPONENT_OK;
		}
		else {
			status = COMPONENT_ERROR;
		}
	}
	else {
		status = COMPONENT_TIMEOUT;
	}

	/* Disable sensor. */
	HUMIDITY_Driver->Sensor_Disable(&HUMIDITY_Handle);
	return status;
}

uint8_t Sensors_Temperature_Handler(void)
{
	uint8_t status = 0;
	int count = 0;

	/* Enable sensor. */
	TEMPERATURE_Driver->Sensor_Enable(&TEMPERATURE_Handle);
	/* Read once to clear DRDY. */
	TEMPERATURE_Driver->Get_Temp(&TEMPERATURE_Handle, &TEMPERATURE_Data);

	/* Wait for data ready. */
	while (TEMPERATURE_Driver->Get_DRDY_Status(&TEMPERATURE_Handle, &status) == COMPONENT_OK
			&& status == 0
			&& count++ < SENSOR_POLLING) {
	}

	if (count < SENSOR_POLLING) {
		if (TEMPERATURE_Driver->Get_Temp(&TEMPERATURE_Handle, &TEMPERATURE_Data) == COMPONENT_OK) {
			status = COMPONENT_OK;
		}
		else {
			status = COMPONENT_ERROR;
		}
	}
	else {
		status = COMPONENT_TIMEOUT;
	}

	/* Disable sensor. */
	TEMPERATURE_Driver->Sensor_Disable(&TEMPERATURE_Handle);
	return status;
}

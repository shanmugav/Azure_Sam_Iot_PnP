/**
 ******************************************************************************
 * @file    LIS2MDL_MAG_driver.h
 * @author  MEMS Application Team
 * @version V1.2
 * @date    9-August-2016
 * @brief   LIS2MDL driver header file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIS2MDL_MAG_DRIVER__H
#define __LIS2MDL_MAG_DRIVER__H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <mems_common_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/

//these could change accordingly with the architecture

#ifndef __ARCHDEP__TYPES
#define __ARCHDEP__TYPES

typedef unsigned char u8_t;
typedef unsigned short int u16_t;
typedef unsigned int u32_t;
typedef int i32_t;
typedef short int i16_t;
typedef signed char i8_t;

#endif /*__ARCHDEP__TYPES*/

/* Exported common structure --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/************** I2C Address *****************/

#define LIS2MDL_MAG_I2C_ADDRESS   0x3C

/************** Who am I  *******************/

#define LIS2MDL_MAG_WHO_AM_I         0x40

/************** Device Register  *******************/
#define LIS2MDL_MAG_OFFSET_X_L    0X45
#define LIS2MDL_MAG_OFFSET_X_H    0X46
#define LIS2MDL_MAG_OFFSET_Y_L    0X47
#define LIS2MDL_MAG_OFFSET_Y_H    0X48
#define LIS2MDL_MAG_OFFSET_Z_L    0X49
#define LIS2MDL_MAG_OFFSET_Z_H    0X4A
#define LIS2MDL_MAG_WHO_AM_I_REG    0X4F
#define LIS2MDL_MAG_CFG_REG_A   0X60
#define LIS2MDL_MAG_CFG_REG_B   0X61
#define LIS2MDL_MAG_CFG_REG_C   0X62
#define LIS2MDL_MAG_INT_CTRL_REG   0X63
#define LIS2MDL_MAG_INT_SOURCE_REG   0X64
#define LIS2MDL_MAG_INT_THS_L   0X65
#define LIS2MDL_MAG_INT_THS_H   0X66
#define LIS2MDL_MAG_STATUS_REG    0X67
#define LIS2MDL_MAG_OUTX_L    0X68
#define LIS2MDL_MAG_OUTX_H    0X69
#define LIS2MDL_MAG_OUTY_L    0X6A
#define LIS2MDL_MAG_OUTY_H    0X6B
#define LIS2MDL_MAG_OUTZ_L    0X6C
#define LIS2MDL_MAG_OUTZ_H    0X6D
#define LIS2MDL_MAG_TEMP_OUT_L    0X6E
#define LIS2MDL_MAG_TEMP_OUT_H    0X6F

/************** Generic Function  *******************/

/*******************************************************************************
* Register      : Generic - All
* Address       : Generic - All
* Bit Group Name: None
* Permission    : W
*******************************************************************************/
mems_status_t LIS2MDL_MAG_WriteReg( void *handle, u8_t Reg, u8_t *Bufp, u16_t len );

/*******************************************************************************
* Register      : Generic - All
* Address       : Generic - All
* Bit Group Name: None
* Permission    : R
*******************************************************************************/
mems_status_t LIS2MDL_MAG_ReadReg( void *handle, u8_t Reg, u8_t *Bufp, u16_t len );

/**************** Base Function  *******************/

/*******************************************************************************
* Register      : WHO_AM_I_REG
* Address       : 0X0F
* Bit Group Name: WHO_AM_I_BIT
* Permission    : RO
*******************************************************************************/
#define   LIS2MDL_MAG_WHO_AM_I_BIT_MASK   0xFF
#define   LIS2MDL_MAG_WHO_AM_I_BIT_POSITION   0
mems_status_t LIS2MDL_MAG_R_WHO_AM_I_(void *handle, u8_t *value);

/*******************************************************************************
* Register      : CFG_REG_A
* Address       : 0X60
* Bit Group Name: MD
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_MD_CONTINUOUS      = 0x00,
  LIS2MDL_MAG_MD_SINGLE      = 0x01,
  LIS2MDL_MAG_MD_POWER_DOWN_DUMMY      = 0x02,
  LIS2MDL_MAG_MD_POWER_DOWN     = 0x03,
} LIS2MDL_MAG_MD_t;

#define   LIS2MDL_MAG_MD_MASK   0x03
mems_status_t  LIS2MDL_MAG_W_SystemOperatingMode(void *handle, LIS2MDL_MAG_MD_t newValue);
mems_status_t LIS2MDL_MAG_R_SystemOperatingMode(void *handle, LIS2MDL_MAG_MD_t *value);

/*******************************************************************************
* Register      : CFG_REG_C
* Address       : 0X62
* Bit Group Name: BDU
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_BDU_DISABLE      = 0x00,
  LIS2MDL_MAG_BDU_ENABLE     = 0x10,
} LIS2MDL_MAG_BDU_t;

#define   LIS2MDL_MAG_BDU_MASK    0x10
mems_status_t  LIS2MDL_MAG_W_BlockDataUpdate(void *handle, LIS2MDL_MAG_BDU_t newValue);
mems_status_t LIS2MDL_MAG_R_BlockDataUpdate(void *handle, LIS2MDL_MAG_BDU_t *value);

/*******************************************************************************
* Register      : CFG_REG_A
* Address       : 0X60
* Bit Group Name: DO
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_DO_10Hz      = 0x00,
  LIS2MDL_MAG_DO_20Hz      = 0x04,
  LIS2MDL_MAG_DO_50Hz      = 0x08,
  LIS2MDL_MAG_DO_100Hz     = 0x0C,
} LIS2MDL_MAG_DO_t;

#define   LIS2MDL_MAG_DO_MASK   0x0C
mems_status_t  LIS2MDL_MAG_W_OutputDataRate(void *handle, LIS2MDL_MAG_DO_t newValue);
mems_status_t LIS2MDL_MAG_R_OutputDataRate(void *handle, LIS2MDL_MAG_DO_t *value);

/*******************************************************************************
* Register      : <REGISTER_L> - <REGISTER_H>
* Output Type   : Magnetic
* Permission    : RO
*******************************************************************************/
mems_status_t LIS2MDL_MAG_Get_Magnetic(void *handle, u8_t *buff);

/**************** Advanced Function  *******************/

/*******************************************************************************
* Register      : CFG_REG_C
* Address       : 0X62
* Bit Group Name: ST
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_ST_DISABLE     = 0x00,
  LIS2MDL_MAG_ST_ENABLE      = 0x02,
} LIS2MDL_MAG_ST_t;

#define   LIS2MDL_MAG_ST_MASK   0x02
mems_status_t  LIS2MDL_MAG_W_SelfTest(void *handle, LIS2MDL_MAG_ST_t newValue);
mems_status_t LIS2MDL_MAG_R_SelfTest(void *handle, LIS2MDL_MAG_ST_t *value);

/*******************************************************************************
* Register      : CFG_REG_A
* Address       : 0X60
* Bit Group Name: TEMP_EN
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_TEMP_EN_DISABLE      = 0x00,
  LIS2MDL_MAG_TEMP_EN_ENABLE     = 0x80,
} LIS2MDL_MAG_TEMP_EN_t;

#define   LIS2MDL_MAG_TEMP_EN_MASK    0x80
mems_status_t  LIS2MDL_MAG_W_TemperatureSensor(void *handle, LIS2MDL_MAG_TEMP_EN_t newValue);
mems_status_t LIS2MDL_MAG_R_TemperatureSensor(void *handle, LIS2MDL_MAG_TEMP_EN_t *value);

/*******************************************************************************
* Register      : CFG_REG_A
* Address       : 0X60
* Bit Group Name: SOFT_RST
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_SOFT_RST_NO      = 0x00,
  LIS2MDL_MAG_SOFT_RST_YES     = 0x02,
} LIS2MDL_MAG_SOFT_RST_t;

#define   LIS2MDL_MAG_SOFT_RST_MASK   0x02
mems_status_t  LIS2MDL_MAG_W_SoftRST(void *handle, LIS2MDL_MAG_SOFT_RST_t newValue);
mems_status_t LIS2MDL_MAG_R_SoftRST(void *handle, LIS2MDL_MAG_SOFT_RST_t *value);

/*******************************************************************************
* Register      : CFG_REG_A
* Address       : 0X60
* Bit Group Name: REBOOT
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_REBOOT_NO      = 0x00,
  LIS2MDL_MAG_REBOOT_YES     = 0x04,
} LIS2MDL_MAG_REBOOT_t;

#define   LIS2MDL_MAG_REBOOT_MASK   0x04
mems_status_t  LIS2MDL_MAG_W_Reboot(void *handle, LIS2MDL_MAG_REBOOT_t newValue);
mems_status_t LIS2MDL_MAG_R_Reboot(void *handle, LIS2MDL_MAG_REBOOT_t *value);

/*******************************************************************************
* Register      : CFG_REG_A
* Address       : 0X60
* Bit Group Name: LP
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_LP_DISABLE     = 0x00,
  LIS2MDL_MAG_LP_ENABLE      = 0x10,
} LIS2MDL_MAG_LP_t;

#define   LIS2MDL_MAG_LP_MASK   0x10
mems_status_t  LIS2MDL_MAG_W_FastLowPowerXYZ(void *handle, LIS2MDL_MAG_LP_t newValue);
mems_status_t LIS2MDL_MAG_R_FastLowPowerXYZ(void *handle, LIS2MDL_MAG_LP_t *value);

/*******************************************************************************
* Register      : CFG_REG_C
* Address       : 0X62
* Bit Group Name: BLE
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_BLE_INVERT     = 0x00,
  LIS2MDL_MAG_BLE_DEFAULT      = 0x08,
} LIS2MDL_MAG_BLE_t;

#define   LIS2MDL_MAG_BLE_MASK    0x08
mems_status_t  LIS2MDL_MAG_W_LittleBigEndianInversion(void *handle, LIS2MDL_MAG_BLE_t newValue);
mems_status_t LIS2MDL_MAG_R_LittleBigEndianInversion(void *handle, LIS2MDL_MAG_BLE_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: XDA
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_XDA_NOT_AVAILABLE      = 0x00,
  LIS2MDL_MAG_XDA_AVAILABLE      = 0x01,
} LIS2MDL_MAG_XDA_t;

#define   LIS2MDL_MAG_XDA_MASK    0x01
mems_status_t LIS2MDL_MAG_R_NewXData(void *handle, LIS2MDL_MAG_XDA_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: YDA
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_YDA_NOT_AVAILABLE      = 0x00,
  LIS2MDL_MAG_YDA_AVAILABLE      = 0x02,
} LIS2MDL_MAG_YDA_t;

#define   LIS2MDL_MAG_YDA_MASK    0x02
mems_status_t LIS2MDL_MAG_R_NewYData(void *handle, LIS2MDL_MAG_YDA_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: ZDA
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_ZDA_NOT_AVAILABLE      = 0x00,
  LIS2MDL_MAG_ZDA_AVAILABLE      = 0x04,
} LIS2MDL_MAG_ZDA_t;

#define   LIS2MDL_MAG_ZDA_MASK    0x04
mems_status_t LIS2MDL_MAG_R_NewZData(void *handle, LIS2MDL_MAG_ZDA_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: ZYXDA
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_ZYXDA_NOT_AVAILABLE      = 0x00,
  LIS2MDL_MAG_ZYXDA_AVAILABLE      = 0x08,
} LIS2MDL_MAG_ZYXDA_t;

#define   LIS2MDL_MAG_ZYXDA_MASK    0x08
mems_status_t LIS2MDL_MAG_R_NewXYZData(void *handle, LIS2MDL_MAG_ZYXDA_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: XOR
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_XOR_NOT_OVERRUN      = 0x00,
  LIS2MDL_MAG_XOR_OVERRUN      = 0x10,
} LIS2MDL_MAG_XOR_t;

#define   LIS2MDL_MAG_XOR_MASK    0x10
mems_status_t LIS2MDL_MAG_R_DataXOverrun(void *handle, LIS2MDL_MAG_XOR_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: YOR
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_YOR_NOT_OVERRUN      = 0x00,
  LIS2MDL_MAG_YOR_OVERRUN      = 0x20,
} LIS2MDL_MAG_YOR_t;

#define   LIS2MDL_MAG_YOR_MASK    0x20
mems_status_t LIS2MDL_MAG_R_DataYOverrun(void *handle, LIS2MDL_MAG_YOR_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: ZOR
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_ZOR_NOT_OVERRUN      = 0x00,
  LIS2MDL_MAG_ZOR_OVERRUN      = 0x40,
} LIS2MDL_MAG_ZOR_t;

#define   LIS2MDL_MAG_ZOR_MASK    0x40
mems_status_t LIS2MDL_MAG_R_DataZOverrun(void *handle, LIS2MDL_MAG_ZOR_t *value);

/*******************************************************************************
* Register      : STATUS_REG
* Address       : 0X67
* Bit Group Name: ZYXOR
* Permission    : RO
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_ZYXOR_NOT_OVERRUN      = 0x00,
  LIS2MDL_MAG_ZYXOR_OVERRUN      = 0x80,
} LIS2MDL_MAG_ZYXOR_t;

#define   LIS2MDL_MAG_ZYXOR_MASK    0x80
mems_status_t LIS2MDL_MAG_R_DataXYZOverrun(void *handle, LIS2MDL_MAG_ZYXOR_t *value);

/*******************************************************************************
* Register      : INT_CTRL_REG
* Address       : 0X63
* Bit Group Name: IEN
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_IEN_DISABLE      = 0x00,
  LIS2MDL_MAG_IEN_ENABLE     = 0x01,
} LIS2MDL_MAG_IEN_t;

#define   LIS2MDL_MAG_IEN_MASK    0x01
mems_status_t  LIS2MDL_MAG_W_InterruptEnable(void *handle, LIS2MDL_MAG_IEN_t newValue);
mems_status_t LIS2MDL_MAG_R_InterruptEnable(void *handle, LIS2MDL_MAG_IEN_t *value);

/*******************************************************************************
* Register      : INT_CTRL_REG
* Address       : 0X63
* Bit Group Name: LIR
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_LIR_LATCHED      = 0x00,
  LIS2MDL_MAG_LIR_NOT_LATCHED      = 0x02,
} LIS2MDL_MAG_LIR_t;

#define   LIS2MDL_MAG_LIR_MASK    0x02
mems_status_t  LIS2MDL_MAG_W_LatchInterruptRq(void *handle, LIS2MDL_MAG_LIR_t newValue);
mems_status_t LIS2MDL_MAG_R_LatchInterruptRq(void *handle, LIS2MDL_MAG_LIR_t *value);

/*******************************************************************************
* Register      : INT_CTRL_REG
* Address       : 0X63
* Bit Group Name: IEA
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_IEA_LOW      = 0x00,
  LIS2MDL_MAG_IEA_HIGH     = 0x04,
} LIS2MDL_MAG_IEA_t;

#define   LIS2MDL_MAG_IEA_MASK    0x04
mems_status_t  LIS2MDL_MAG_W_InterruptActive(void *handle, LIS2MDL_MAG_IEA_t newValue);
mems_status_t LIS2MDL_MAG_R_InterruptActive(void *handle, LIS2MDL_MAG_IEA_t *value);

/*******************************************************************************
* Register      : INT_CTRL_REG
* Address       : 0X63
* Bit Group Name: ZIEN
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_ZIEN_DISABLE     = 0x00,
  LIS2MDL_MAG_ZIEN_ENABLE      = 0x20,
} LIS2MDL_MAG_ZIEN_t;

#define   LIS2MDL_MAG_ZIEN_MASK   0x20
mems_status_t  LIS2MDL_MAG_W_InterruptOnZ(void *handle, LIS2MDL_MAG_ZIEN_t newValue);
mems_status_t LIS2MDL_MAG_R_InterruptOnZ(void *handle, LIS2MDL_MAG_ZIEN_t *value);

/*******************************************************************************
* Register      : INT_CTRL_REG
* Address       : 0X63
* Bit Group Name: YIEN
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_YIEN_DISABLE     = 0x00,
  LIS2MDL_MAG_YIEN_ENABLE      = 0x40,
} LIS2MDL_MAG_YIEN_t;

#define   LIS2MDL_MAG_YIEN_MASK   0x40
mems_status_t  LIS2MDL_MAG_W_InterruptOnY(void *handle, LIS2MDL_MAG_YIEN_t newValue);
mems_status_t LIS2MDL_MAG_R_InterruptOnY(void *handle, LIS2MDL_MAG_YIEN_t *value);

/*******************************************************************************
* Register      : INT_CTRL_REG
* Address       : 0X63
* Bit Group Name: XIEN
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_XIEN_DISABLE     = 0x00,
  LIS2MDL_MAG_XIEN_ENABLE      = 0x80,
} LIS2MDL_MAG_XIEN_t;

#define   LIS2MDL_MAG_XIEN_MASK   0x80
mems_status_t  LIS2MDL_MAG_W_InterruptOnX(void *handle, LIS2MDL_MAG_XIEN_t newValue);
mems_status_t LIS2MDL_MAG_R_InterruptOnX(void *handle, LIS2MDL_MAG_XIEN_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: INT
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_INT_DOWN     = 0x00,
  LIS2MDL_MAG_INT_UP     = 0x01,
} LIS2MDL_MAG_INT_t;

#define   LIS2MDL_MAG_INT_MASK    0x01
mems_status_t  LIS2MDL_MAG_W_InterruptFlag(void *handle, LIS2MDL_MAG_INT_t newValue);
mems_status_t LIS2MDL_MAG_R_InterruptFlag(void *handle, LIS2MDL_MAG_INT_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: MROI
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_MROI_IN_RANGE      = 0x00,
  LIS2MDL_MAG_MROI_OVERFLOW      = 0x02,
} LIS2MDL_MAG_MROI_t;

#define   LIS2MDL_MAG_MROI_MASK   0x02
mems_status_t  LIS2MDL_MAG_W_MagneticFieldOverflow(void *handle, LIS2MDL_MAG_MROI_t newValue);
mems_status_t LIS2MDL_MAG_R_MagneticFieldOverflow(void *handle, LIS2MDL_MAG_MROI_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: NTH_Z
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_NTH_Z_DOWN     = 0x00,
  LIS2MDL_MAG_NTH_Z_UP     = 0x04,
} LIS2MDL_MAG_NTH_Z_t;

#define   LIS2MDL_MAG_NTH_Z_MASK    0x04
mems_status_t  LIS2MDL_MAG_W_NegativeThresholdFlagZ(void *handle, LIS2MDL_MAG_NTH_Z_t newValue);
mems_status_t LIS2MDL_MAG_R_NegativeThresholdFlagZ(void *handle, LIS2MDL_MAG_NTH_Z_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: NTH_Y
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_NTH_Y_DOWN     = 0x00,
  LIS2MDL_MAG_NTH_Y_UP     = 0x08,
} LIS2MDL_MAG_NTH_Y_t;

#define   LIS2MDL_MAG_NTH_Y_MASK    0x08
mems_status_t  LIS2MDL_MAG_W_NegativeThresholdFlagY(void *handle, LIS2MDL_MAG_NTH_Y_t newValue);
mems_status_t LIS2MDL_MAG_R_NegativeThresholdFlagY(void *handle, LIS2MDL_MAG_NTH_Y_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: NTH_X
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_NTH_X_DOWN     = 0x00,
  LIS2MDL_MAG_NTH_X_UP     = 0x10,
} LIS2MDL_MAG_NTH_X_t;

#define   LIS2MDL_MAG_NTH_X_MASK    0x10
mems_status_t  LIS2MDL_MAG_W_NegativeThresholdFlagX(void *handle, LIS2MDL_MAG_NTH_X_t newValue);
mems_status_t LIS2MDL_MAG_R_NegativeThresholdFlagX(void *handle, LIS2MDL_MAG_NTH_X_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: PTH_Z
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_PTH_Z_DOWN     = 0x00,
  LIS2MDL_MAG_PTH_Z_UP     = 0x20,
} LIS2MDL_MAG_PTH_Z_t;

#define   LIS2MDL_MAG_PTH_Z_MASK    0x20
mems_status_t  LIS2MDL_MAG_W_PositiveThresholdFlagZ(void *handle, LIS2MDL_MAG_PTH_Z_t newValue);
mems_status_t LIS2MDL_MAG_R_PositiveThresholdFlagZ(void *handle, LIS2MDL_MAG_PTH_Z_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: PTH_Y
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_PTH_Y_DOWN     = 0x00,
  LIS2MDL_MAG_PTH_Y_UP     = 0x40,
} LIS2MDL_MAG_PTH_Y_t;

#define   LIS2MDL_MAG_PTH_Y_MASK    0x40
mems_status_t  LIS2MDL_MAG_W_PositiveThresholdFlagY(void *handle, LIS2MDL_MAG_PTH_Y_t newValue);
mems_status_t LIS2MDL_MAG_R_PositiveThresholdFlagY(void *handle, LIS2MDL_MAG_PTH_Y_t *value);

/*******************************************************************************
* Register      : INT_SOURCE_REG
* Address       : 0X64
* Bit Group Name: PTH_X
* Permission    : RW
*******************************************************************************/
typedef enum
{
  LIS2MDL_MAG_PTH_X_DOWN     = 0x00,
  LIS2MDL_MAG_PTH_X_UP     = 0x80,
} LIS2MDL_MAG_PTH_X_t;

#define   LIS2MDL_MAG_PTH_X_MASK    0x80
mems_status_t  LIS2MDL_MAG_W_PositiveThresholdFlagX(void *handle, LIS2MDL_MAG_PTH_X_t newValue);
mems_status_t LIS2MDL_MAG_R_PositiveThresholdFlagX(void *handle, LIS2MDL_MAG_PTH_X_t *value);

/*******************************************************************************
* Register      : <REGISTER_L> - <REGISTER_H>
* Output Type   : Temperature
* Permission    : RO
*******************************************************************************/
mems_status_t LIS2MDL_MAG_Get_Temperature(void *handle, u8_t *buff);

/*******************************************************************************
* Register      : <REGISTER_L> - <REGISTER_H>
* Output Type   : MagneticThreshold
* Permission    : RW
*******************************************************************************/
mems_status_t LIS2MDL_MAG_Set_MagneticThreshold(void *handle, u8_t *buff);
mems_status_t LIS2MDL_MAG_Get_MagneticThreshold(void *handle, u8_t *buff);

/*******************************************************************************
* Register      : Generic - All
* Address       : Generic - All
* Bit Group Name: None
* Permission    : W
*******************************************************************************/
void LIS2MDL_MAG_SwapHighLowByte(u8_t *bufferToSwap, u8_t numberOfByte, u8_t dimension);

#ifdef __cplusplus
}
#endif

#endif

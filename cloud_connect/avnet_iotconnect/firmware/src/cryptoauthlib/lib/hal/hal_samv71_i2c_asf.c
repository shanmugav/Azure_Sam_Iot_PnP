/**
 * \file
 * \brief ATCA Hardware abstraction layer for SAMV71 I2C over ASF drivers.
 *
 * This code is structured in two parts.  Part 1 is the connection of the ATCA HAL API to the physical I2C
 * implementation. Part 2 is the ASF I2C primitives to set up the interface.
 *
 * Prerequisite: add SERCOM I2C Master Polled support to application in Atmel Studio
 *
 * \copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT,
 * SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE
 * OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
 * MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
 * FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL
 * LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED
 * THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR
 * THIS SOFTWARE.
 */

#include <string.h>
#include <stdio.h>

#include "twihs_asf.h"
#include "atca_hal.h"
#include "hal_samv71_i2c_asf.h"
#include "atca_device.h"
#include "atca_execution.h"

/** \brief logical to physical bus mapping structure */
static ATCAI2CMaster_t i2c_hal_data[MAX_I2C_BUSES];   // map logical, 0-based bus number to index

/** \brief method to change the bus speed of I2C
 * \param[in] iface  interface on which to change bus speed
 * \param[in] speed  baud rate (typically 100000 or 400000)
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_change_baud(ATCAIface iface, uint32_t speed)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    twihs_registers_t *twihs_device = i2c_hal_data[bus].twi_module;

    // if necessary, revert baud rate to what came in.
    if (twihs_set_speed(twihs_device, speed, 300000000 / 2) == FAIL) {
        return ATCA_COMM_FAIL;
    }

    return ATCA_SUCCESS;
}

/** \brief initialize an I2C interface using given config
 * \param[in] hal - opaque ptr to HAL data
 * \param[in] cfg - interface configuration
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
    twihs_options_t opt_twi_master;
    
    if (cfg->atcai2c.bus >= MAX_I2C_BUSES) {
        return ATCA_COMM_FAIL;
    }
    ATCAI2CMaster_t* data = &i2c_hal_data[cfg->atcai2c.bus];

    if (data->ref_ct <= 0) {
        // Bus isn't being used, enable it
        opt_twi_master.master_clk = 300000000 / 2;
        opt_twi_master.speed = cfg->atcai2c.baud;

        switch (cfg->atcai2c.bus)
        {
        case 0:     /* Enable the peripheral clock for TWI */
            data->twi_id = ID_TWIHS0;
            data->twi_module = TWIHS0_REGS;
            NVIC_DisableIRQ(TWIHS0_IRQn);

            break;

        case 1:     /* Enable the peripheral clock for TWI */
            data->twi_id = ID_TWIHS1;
            data->twi_module = TWIHS1_REGS;
            NVIC_DisableIRQ(TWIHS1_IRQn);

            break;

        case 2:     /* Enable the peripheral clock for TWI */
            data->twi_id = ID_TWIHS2;
            data->twi_module = TWIHS2_REGS;
            NVIC_DisableIRQ(TWIHS2_IRQn);
            break;

        default:
            return ATCA_COMM_FAIL;
        }
        
        if (twihs_master_init(data->twi_module, &opt_twi_master) != TWIHS_SUCCESS) {
            return ATCA_COMM_FAIL;
        }

        // store this for use during the release phase
        data->bus_index = cfg->atcai2c.bus;
        // buses are shared, this is the first instance
        data->ref_ct = 1;
    }
    else {
        // Bus is already is use, increment reference counter
        data->ref_ct++;
    }

    ((ATCAHAL_t*)hal)->hal_data = data;

    return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C post init
 * \param[in] iface  instance
 * \return ATCA_SUCCESS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
    return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C send over ASF
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    twihs_registers_t *twihs_device = i2c_hal_data[bus].twi_module;

    twihs_packet_t packet = {
        .addr[0]        = 0,
        .addr[1]        = 0,
        .addr_length    = 0,
        .chip           = cfg->atcai2c.slave_address >> 1,
        .buffer         = txdata,
    };
    // txdata[0] is using _reserved byte of the ATCAPacket
    txdata[0] = 0x03;   // insert the Word Address Value, Command token
    txlength++;         // account for word address value byte.
    packet.length = txlength;

	if (twihs_master_write(twihs_device, &packet) == 0) {
		return ATCA_SUCCESS;
	}

	return ATCA_COMM_FAIL;
}

/** \brief HAL implementation of I2C receive function for ASF I2C
 * \param[in]    iface     Device to interact with.
 * \param[out]   rxdata    Data received will be returned here.
 * \param[inout] rxlength  As input, the size of the rxdata buffer.
 *                         As output, the number of bytes received.
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    int retries = cfg->rx_retries;
    int status = !ATCA_SUCCESS;
    twihs_registers_t *twihs_device = i2c_hal_data[bus].twi_module;
    uint16_t rxdata_max_size = *rxlength;

    twihs_packet_t packet = {
        .chip   = cfg->atcai2c.slave_address >> 1,
        .buffer = rxdata,
        .length = 1
    };

    *rxlength = 0;
    if (rxdata_max_size < 1) {
        return ATCA_SMALL_BUFFER;
    }

    // Read Length byte i.e. first byte from device
    while (retries-- > 0 && status != ATCA_SUCCESS) {
        if (twihs_master_read(twihs_device, &packet) != TWIHS_SUCCESS) {
            status = ATCA_COMM_FAIL;
        }
        else {
            status = ATCA_SUCCESS;
        }
    }

    if (status != ATCA_SUCCESS) {
        return status;
    }
    if (rxdata[0] < ATCA_RSP_SIZE_MIN) {
        return ATCA_INVALID_SIZE;
    }
    if (rxdata[0] > rxdata_max_size) {
        return ATCA_SMALL_BUFFER;
    }

    // Update receive length with first byte received and set to read rest of the data
    packet.length = rxdata[0] - 1;
    packet.buffer = &rxdata[1];
    if (twihs_master_read(twihs_device, &packet) != TWIHS_SUCCESS) {
        return ATCA_COMM_FAIL;
    }

    *rxlength = rxdata[0];
    return ATCA_SUCCESS;
}

/** \brief manages reference count on given bus and releases resource if no more refences exist
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 * \return ATCA_SUCCESS
 */
ATCA_STATUS hal_i2c_release(void *hal_data)
{
    ATCAI2CMaster_t *hal = (ATCAI2CMaster_t*)hal_data;
    twihs_registers_t *twihs_device = i2c_hal_data[hal->bus_index].twi_module;

    // if the use count for this bus has gone to 0 references, disable it.  protect against an unbracketed release
    if (hal && --(hal->ref_ct) <= 0) {
        twihs_disable_master_mode(twihs_device);
        hal->ref_ct = 0;
    }

    return ATCA_SUCCESS;
}

/** \brief wake up CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to wakeup
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int retries = cfg->rx_retries;
    int bus = cfg->atcai2c.bus;
    uint32_t bdrt = cfg->atcai2c.baud;
    int status = 1;
    uint8_t data[4];
    twihs_registers_t *twihs_device = i2c_hal_data[bus].twi_module;

    // If not already at 100kHz, change it.
    if (bdrt != 100000) {
    	hal_i2c_change_baud(iface, 100000);
    }

    // Send 0x00 as wake pulse.
	memset(data, 0x00, 4);
    twihs_packet_t packet = {
        .addr[0]        = 0,
        .addr[1]        = 0,
        .addr_length    = 0,
        .chip           = 0,
        .buffer         =  &data[0],
        .length         = 1
    };
    twihs_master_write(twihs_device, &packet);

    // Rounded up to the nearest ms.
	// Wait tWHI + tWLO which is configured based on device type and configuration structure.
    atca_delay_ms(((uint32_t)cfg->wake_delay + (1000 - 1)) / 1000);

    // if necessary, revert baud rate to what came in.
    if (bdrt != 100000) {
    	hal_i2c_change_baud(iface, bdrt);
    }

    // Check for wake response.
	packet.chip = cfg->atcai2c.slave_address >> 1;
	packet.length = 4;
	while (retries-- > 0 && status != 0) {
		status = twihs_master_read(twihs_device, &packet);
	}
	
    if (status != 0) {
        if (retries <= 0) {
            return ATCA_TOO_MANY_COMM_RETRIES;
        }
        return ATCA_COMM_FAIL;
    }

    return hal_check_wake(data, 4);
}

/** \brief idle CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to idle
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    uint8_t data[4];
    twihs_registers_t *twihs_device = i2c_hal_data[bus].twi_module;

	// Idle word address value.
    data[0] = 0x02;
    twihs_packet_t packet = {
        .addr[0]        = 0,
        .addr[1]        = 0,
        .addr_length    = 0,
        .chip           = cfg->atcai2c.slave_address >> 1,
        .buffer         = data,
		.length         = 1,
    };
    if (twihs_master_write(twihs_device, &packet) != 0) {
        return ATCA_COMM_FAIL;
    }

    return ATCA_SUCCESS;
}

/** \brief sleep CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to sleep
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */

ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
    ATCAIfaceCfg *cfg = atgetifacecfg(iface);
    int bus = cfg->atcai2c.bus;
    uint8_t data[4];
    twihs_registers_t *twihs_device = i2c_hal_data[bus].twi_module;

	// Sleep word address value.
    data[0] = 0x01;
    twihs_packet_t packet = {
        .addr[0]        = 0,
        .addr[1]        = 0,
        .addr_length    = 0,
        .chip           = cfg->atcai2c.slave_address >> 1,
        .buffer         = data,
		.length         = 1,
    };
    if (twihs_master_write(twihs_device, &packet) != 0) {
        return ATCA_COMM_FAIL;
    }

    return ATCA_SUCCESS;
}

/** \brief discover i2c buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] i2c_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 * \return ATCA_SUCCESS
 */
ATCA_STATUS hal_i2c_discover_buses(int i2c_buses[], int max_buses)
{
    return ATCA_UNIMPLEMENTED;
}

/** \brief discover any CryptoAuth devices on a given logical bus number
 * \param[in]  bus_num  Logical bus number on which to look for CryptoAuth devices
 * \param[out] cfg      Pointer to head of an array of interface config structures which get filled in by this method
 * \param[out] found    Number of devices found on this bus
 * \return ATCA_SUCCESS on success, otherwise an error code.
 */
ATCA_STATUS hal_i2c_discover_devices(int bus_num, ATCAIfaceCfg cfg[], int *found)
{
    return ATCA_UNIMPLEMENTED;
}

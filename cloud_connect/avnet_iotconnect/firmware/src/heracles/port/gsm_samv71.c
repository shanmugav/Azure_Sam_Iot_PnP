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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

/** Heracles includes. */
#include "heracles/gsm.h"
#include "heracles/gsm_serial.h"
#include "heracles/gsm_port.h"

/** Platform includes. */
#include "definitions.h"                // SYS function prototypes

// PIO definition are located in file "plib_pio.h"

#define GSM_PWRKEY_ON()      GSM_PWRKEY_Set()
#define GSM_PWRKEY_OFF()     GSM_PWRKEY_Clear()
#define GSM_POWER_ON()       GSM_POWER_Set()
#define GSM_POWER_OFF()      GSM_POWER_Clear()
#define GSM_GET_STATUS()     GSM_STATUS_Get()

extern volatile uint32_t ms_ticks;

/**
 * \brief Enter critical section by disabling interrupts.
 *
 * \return Interrupt mask to be saved.
 */
uint32_t gsm_enter_critical_section(void)
{
	NVIC_DisableIRQ(UART3_IRQn);
	return 0;
}

/**
 * \brief Exit critical section by re-enabling interrupts.
 *
 * \param[in] status Interrupt mask to be restored.
 */
void gsm_exit_critical_section(unsigned int status)
{
	NVIC_EnableIRQ(UART3_IRQn);
}

/**
 * \brief Get tick count.
 *
 * \return Number of tick this start in milliseconds.
 */
uint32_t gsm_port_get_tick(void)
{
	return ms_ticks;
}

/**
 * \brief Wait for the specified amount of milliseconds.
 *
 * \param[in] delay Delay in milliseconds.
 */
void gsm_port_delay(uint32_t delay)
{
	  uint32_t tickstart = ms_ticks;

	  while ((ms_ticks - tickstart) < delay) {
	  }
}

/**
 * \brief Power ON SIMCom module.
 *
 * \return Response code.
 */
int gsm_port_power_on(int timeout)
{
	/* Configure Heracles module IOs. */
	gsm_port_io_init();

	GSM_POWER_OFF();
	GSM_PWRKEY_OFF();
    gsm_port_delay(1250);
	
	/* PWRKEY High. */
	GSM_POWER_ON();
    GSM_PWRKEY_ON();
    gsm_port_delay(1000);
	/* PWRKEY Low. */
    GSM_PWRKEY_OFF();
    gsm_port_delay(1000);
	/* PWRKEY High. */
    GSM_PWRKEY_ON();

    while (!GSM_GET_STATUS()) {
    	gsm_port_delay(10);
		if (!timeout--) {
			return GSM_RESP_ERROR;
		}
    }

	return GSM_RESP_OK;
}

/**
 * \brief Power OFF SIMCom module.
 *
 * \return Response code.
 */
int gsm_port_power_off(void)
{
	int timeout = 25;

	/* PWRKEY Low. */
    GSM_PWRKEY_OFF();

    gsm_port_delay(1000);

	/* PWRKEY High. */
    GSM_PWRKEY_ON();

    /* Wait for status to go low (max 2s according to datasheet or timeout). */
	while (GSM_GET_STATUS() == 1) {
		gsm_port_delay(10);
		if (!timeout--) {
			return GSM_RESP_ERROR;
		}
	}

	/* Power Off. */
	GSM_POWER_OFF();

	/* PWRKEY Low. */
    GSM_PWRKEY_OFF();

	return GSM_RESP_OK;
}

/**
 * \brief Get SIMCom modem status pin state.
 *
 * \param[in] delay Delay in milliseconds.
 */
int gsm_port_get_status(void)
{
	return GSM_GET_STATUS();
}

/**
 * \brief Initialize IO lines to SIMCom module.
 */
void gsm_port_io_init(void)
{
	/* Already configured by Harmony3. */
}

/**
 * \brief Free IO lines to SIMCom module.
 */
void gsm_port_io_deinit(void)
{
	/* Unused. */
}

/**
 * \brief Open serial port to SIMCom module.
 *
 * \param[in] callback Function pointer to manage serial events from SIMCom module.
 * \param[in] at Set to 1 to enable AT ECHO (recommended to avoid URC collision), 0 otherwise.
 */
void gsm_port_uart_open(void)
{
    /* Configure UART interrupt. */
    UART3_REGS->UART_IER = (UART_IER_RXRDY_Msk /*| UART_IER_OVRE_Msk | UART_IER_FRAME_Msk | UART_IER_PARE_Msk*/);
    
	/* Configure and enable interrupt of UART. */
	NVIC_EnableIRQ(UART3_IRQn);
}

/**
 * \brief Close serial port to SIMCom module.
 */
void gsm_port_uart_close(void)
{
    /* Disable receiver and transmitter */
	UART3_REGS->UART_CR = UART_CR_RXDIS_Msk | UART_CR_TXDIS_Msk;
    
    /* Disable UART interrupt. */
    UART3_REGS->UART_IDR = (UART_IER_RXRDY_Msk /*| UART_IER_OVRE_Msk | UART_IER_FRAME_Msk | UART_IER_PARE_Msk */);

	/* Disable USART */
	NVIC_DisableIRQ(UART3_IRQn);
}

/**
 * \brief Send one byte over serial to SIMCom module.
 *
 * \param[out] c Character to send.
 */
void gsm_port_uart_putc(uint8_t c)
{
	UART3_WriteByte(c);
}

/**
 * \brief Interrupt routine from SIMCom module.
 */
void UART3_Handler(void)
{
	uint32_t status = UART3_REGS->UART_SR;
	uint8_t in;
	
	status &= UART3_REGS->UART_IMR;

	/* UART received data flag. */
	if (status & UART_IER_RXRDY_Msk) {
		in = UART3_REGS->UART_RHR;
		gsm_serial_isr_data(in);
	}
	/* Overflow error flag. */
	else if (status & UART_IER_OVRE_Msk) {
		gsm_serial_isr_overflow();
	}
	/* Framing error flag. */
	else if (status & UART_IER_FRAME_Msk) {
		gsm_serial_isr_framing();
	}
	/* Noise error flag. */
	else if (status & UART_IER_PARE_Msk) {
		gsm_serial_isr_noise();
	}
}

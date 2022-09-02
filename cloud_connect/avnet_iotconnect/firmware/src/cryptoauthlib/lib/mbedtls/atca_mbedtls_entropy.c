/**
 * \brief Add mbedTLS entropy Function with hardware TRNG.
 *
 * \copyright (c) 2015-2019 Microchip Technology Inc. and its subsidiaries.
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

/* mbedTLS boilerplate includes */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/entropy.h"
#include "mbedtls/platform_util.h"

/* Cryptoauthlib Includes */
#include "cryptoauthlib.h"
#include "basic/atca_basic.h"
#include "atca_mbedtls_wrap.h"
#include <string.h>

#define ATCACERT_MIN(x, y) ((x) < (y) ? (x) : (y))
#define ATCACERT_MAX(x, y) ((x) >= (y) ? (x) : (y))

#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
/** Generate random number */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen);
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
	uint32_t to_copy = 0;
	uint8_t rnd[32];

    *olen = len;
    while (len > 0) {
		atcab_random((uint8_t *)&rnd);
		to_copy = ATCACERT_MIN(sizeof(rnd), len);
        memcpy(output, rnd, to_copy);
        output += to_copy;
        len -= to_copy;
    }

    return 0;
}
#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT */

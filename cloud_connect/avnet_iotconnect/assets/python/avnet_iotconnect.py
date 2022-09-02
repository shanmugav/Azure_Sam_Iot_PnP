# -*- coding: utf-8 -*-
# 2018 to present - Copyright Microchip Technology Inc. and its subsidiaries.

# Subject to your compliance with these terms, you may use Microchip software
# and any derivatives exclusively with Microchip products. It is your
# responsibility to comply with third party license terms applicable to your
# use of third party software (including open source software) that may
# accompany Microchip software.

# THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
# EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
# WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR
# PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL,
# PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY
# KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
# HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
# FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
# ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
# THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

import binascii
import shutil
from tp_utils.tp_print import print
from helper import (
            connect_to_prototyping_board, generate_custom_pki,
            verify_cert_chain, verify_SE_with_random_challenge,
            generate_verif_cert)


class AvnetIoTConnectUsecase():
    def __init__(self, boards, user_input=None):
        self.boards = boards

    def generate_resources(self, b=None):
        element = connect_to_prototyping_board(self.boards, b)
        assert element, 'Connection to Board failed'

        print('Generating Custom PKI certificates...', canvas=b)
        generate_custom_pki(b)
        self.root_crt = 'root_crt.crt'
        self.root_cer = 'root_crt.cer'
        self.root_key = 'root_key.key'
        self.signer_crt = 'signer_FFFF.crt'
        self.signer_key = 'signer_FFFF.key'
        self.cust_def_1_c = 'cust_def_1_signer.c'
        self.cust_def_1_h = 'cust_def_1_signer.h'
        self.cust_def_2_c = 'cust_def_2_device.c'
        self.cust_def_2_h = 'cust_def_2_device.h'
        serial_number = element.get_device_serial_number().hex().upper()
        self.device_crt = f'device_{serial_number}.crt'
        # Update certificate in project files
        try:
            shutil.copyfile(self.root_crt, self.root_cer)
            shutil.copyfile(self.cust_def_1_c, "./firmware/src/certs/" + self.cust_def_1_c)
            shutil.copyfile(self.cust_def_1_h, "./firmware/src/certs/" + self.cust_def_1_h)
            shutil.copyfile(self.cust_def_2_c, "./firmware/src/certs/" + self.cust_def_2_c)
            shutil.copyfile(self.cust_def_2_h, "./firmware/src/certs/" + self.cust_def_2_h)
        except:
            print('Skipping file copy...')

    def register_root(self, b=None):
        # Register signer
        print('Generating Verification Root Certificate for Avnet IoTConnect cloud...', canvas=b)
        generate_verif_cert(b)

    def verify_cert_chain(self, b=None):
        device_cert, crt_template = verify_cert_chain(
                    self.root_crt, self.root_key,
                    self.signer_crt, self.signer_key,
                    self.device_crt, b)
        self.device_crt = device_cert
        self.crt_template = crt_template

    def verify_SE_with_random_challenge(self, b=None):
        verify_SE_with_random_challenge(
                    self.device_crt, self.crt_template['device'], b)

    def prompt_avnet_gui(self, qtuifile, b=None):
        thing_id = None
        for extension in self.device_crt.extensions:
            if extension.oid._name != 'subjectKeyIdentifier':
                continue
            thing_id = binascii.b2a_hex(extension.value.digest).decode('ascii')

        if thing_id is None:
            raise ValueError("Can't find thing name from device certificate")


# Standard boilerplate to call the main() function to begin
# the program.
if __name__ == '__main__':
    pass

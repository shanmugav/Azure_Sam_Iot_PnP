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

import os
import binascii
from cloud_connect import AWSConnect
from tp_utils.tp_settings import TPSettings
from pathlib import Path
from tp_utils.tp_print import print
import tp_utils.tp_input_dialog as tp_userinput
from helper import (connect_to_prototyping_board, generate_custom_pki,
                    verify_cert_chain_custompki, verify_cert_chain,
                    verify_SE_with_random_challenge, generate_manifest,
                    restore_mchp_certs_on_device, generate_project_config_h)


class AWSConnectBase():
    def __init__(self, boards):
        self.boards = boards
        self.connection = AWSConnect()

    def connect_to_cloud(self, b=None):
        self.__config_aws_cli(b)

    def connect_to_board(self, b=None):
        self.element = connect_to_prototyping_board(self.boards, b)
        assert self.element, 'Connection to Board failed'
        self.serial_number = self.element.get_device_serial_number()

    def __config_aws_cli(self, b=None):
        print('Configure AWS CLI...', canvas=b)
        tp_settings = TPSettings()
        acc_setup_path = 'file:///'+os.path.join(
                        tp_settings.get_tpds_core_path(),
                        'docs',
                        'AWS_demo_account_setup.html').replace('\\', '/')
        csv_file_path = os.path.join(
                        tp_settings.get_tpds_core_path(),
                        'docs',
                        'AWS_test_account_credentials.csv').replace('\\', '/')
        print(f'Loading credentials from {csv_file_path}')

        msg_box_info = (
            f'<font color=#0000ff><b>Invalid AWS account credentials'
            f'</b></font><br>'
            f'<br>To setup an AWS account, please refer<br>'
            f'<a href={acc_setup_path}>AWS Account Setup Guide: {acc_setup_path}</a>'
            f'<br>To input AWS account credentials, open below CSV file<br>'
            f'<a href={csv_file_path}>AWS Account Credentials CSV File: {csv_file_path}'
            f'</a>')
        if('replace_your' in Path(csv_file_path).read_text()):
            acc_cred_diag = tp_userinput.TPMessageBox(
                title="AWS account credentials",
                info=msg_box_info)
            acc_cred_diag.invoke_dialog()

        self.connection.set_credentials(csv_file_path)
        with open('aws_connect.h', 'w') as f:
            f.write('#ifndef _AWS_CONNECT_H\n')
            f.write('#define _AWS_CONNECT_H\n\n')
            f.write('#include "cryptoauthlib.h"\n\n')
            f.write('#ifdef __cplusplus\n')
            f.write('extern "C" {\n')
            f.write('#endif\n\n')
            cloud_endpoint = self.connection.iot.describe_endpoint(
                                endpointType='iot:Data').get(
                                'endpointAddress')
            f.write(
                f'#define CLOUD_ENDPOINT "{cloud_endpoint}"\n\n')
            f.write('#ifdef __cplusplus\n')
            f.write('}\n')
            f.write('#endif\n')
            f.write('#endif\n')


class AWSCustomPKI(AWSConnectBase):
    def __init__(self, boards):
        super().__init__(boards)

    def generate_resources(self, b=None):
        self.connect_to_board(b)

        print('Generationg Crypto asset CustomPKI certs...', canvas=b)
        generate_custom_pki(b)
        self.root_crt = 'root_crt.crt'
        self.root_key = 'root_key.key'
        self.signer_crt = 'signer_FFFF.crt'
        self.signer_key = 'signer_FFFF.key'
        self.device_crt = f'device_{self.serial_number.hex().upper()}.crt'
        generate_project_config_h(cert_type='CUSTOM', address=0x6C)

    def register_certificates(self, b=None):
        self.connect_to_cloud(b)
        # Register signer
        print(f'Registering {self.signer_crt} to AWS account...', canvas=b)
        self.connection.register_signer(
                                    signer_cert=self.signer_crt,
                                    signer_key=self.signer_key)
        print('Completed...', canvas=b)

    def verify_cert_chain(self, b=None):
        device_cert, crt_template = verify_cert_chain_custompki(
                    self.root_crt, self.root_key,
                    self.signer_crt, self.signer_key,
                    self.device_crt, b)
        self.device_crt = device_cert
        self.crt_template = crt_template

    def verify_SE_with_random_challenge(self, b=None):
        verify_SE_with_random_challenge(
                    b, self.device_crt, device_crt_template = self.crt_template['device'])

    def prompt_aws_gui(self, qtuifile, b=None):
        thing_id = None
        for extension in self.device_crt.extensions:
            if extension.oid._name != 'subjectKeyIdentifier':
                continue
            thing_id = binascii.b2a_hex(extension.value.digest).decode('ascii')

        if thing_id is None:
            raise ValueError("Can't find thing name from device certificate")

        self.connection.execute_aws_gui(thing_id=thing_id.lower(),
                                        qtUiFile=qtuifile)


class AWSIoTAuthentication(AWSConnectBase):
    def __init__(self, boards):
        super().__init__(boards)

    def generate_resources(self, b=None):
        self.connect_to_board(b)

        mchp_certs, r_manifest = restore_mchp_certs_on_device(
                            self.serial_number, b)
        self.device_crt = mchp_certs.get('device')
        self.signer_crt = mchp_certs.get('signer')
        self.root_crt = mchp_certs.get('root')
        if r_manifest:
            self.manifest = r_manifest
        else:
            self.manifest = generate_manifest(
                b, self.signer_crt.certificate, self.device_crt.certificate)
        generate_project_config_h(cert_type='MCHP', address=0x6C)

    def register_device(self, b=None):
        self.connect_to_cloud(b)
        print('Registering device to AWS account...', canvas=b)
        self.connection.register_from_manifest(
                                    self.manifest.get('json_file'),
                                    self.manifest.get('ca_cert'))
        print('Completed...', canvas=b)

    def verify_cert_chain(self, b=None):
        if(self.root_crt is not None):
            self.dev_cert = verify_cert_chain(
                b, self.signer_crt.certificate, self.device_crt.certificate, self.root_crt.certificate)
        else:
            self.dev_cert = verify_cert_chain(
                b, self.signer_crt.certificate, self.device_crt.certificate)
        if self.dev_cert is None:
            raise ValueError('Certificate chain validation is failed')

    def verify_SE_with_random_challenge(self, b=None):
        verify_SE_with_random_challenge(b, self.dev_cert)


# Standard boilerplate to call the main() function to begin
# the program.
if __name__ == '__main__':
    pass

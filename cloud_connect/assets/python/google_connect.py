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
import base64
import json
from calendar import timegm
from datetime import datetime, timedelta
from cloud_connect import GCPConnect
from tp_utils.tp_settings import TPSettings
from pathlib import Path
import cryptoauthlib as cal
import cryptography
from tp_utils.tp_print import print
import resource_generation
from cryptography.hazmat.primitives.asymmetric import utils
from cryptography.hazmat.primitives import hashes
import tp_utils.tp_input_dialog as tp_userinput
from helper import (connect_to_prototyping_board, generate_custom_pki,
                    verify_cert_chain_custompki, verify_cert_chain,
                    verify_SE_with_random_challenge, generate_manifest,
                    restore_mchp_certs_on_device, generate_project_config_h)


class GoogleConnectBase():
    def __init__(self, boards):
        self.boards = boards
        self.connection = GCPConnect()

    def connect_to_cloud(self, b=None):
        self.__config_gcp_credentials(b)

    def connect_to_board(self, b=None):
        self.element = connect_to_prototyping_board(self.boards, b)
        assert self.element, 'Connection to Board failed'
        self.serial_number = self.element.get_device_serial_number()

    def generate_JWT(self, crt_template=None, b=None):
        print('JWTs are used for short-lived authentication between', canvas=b)
        print('devices and MQTT bridges. Since JWTs are time based,', canvas=b)
        print('generating one will be timed out within short time', canvas=b)
        print('So we will generate again in embedded project', canvas=b)
        print('Generate JWT...', canvas=b)
        token = {
            "iat": datetime.utcnow(),
            "exp": datetime.utcnow() + timedelta(minutes=20),
            "aud": self.connection.project_id,
        }

        token["iat"] = timegm(token["iat"].utctimetuple())
        token["exp"] = timegm(token["exp"].utctimetuple())

        # payload
        json_payload = json.dumps(
            token,
            separators=(',', ':'),
            cls=None
        ).encode('utf-8')

        # header
        header = {'typ': 'JWT', 'alg': 'ES256'}
        json_header = json.dumps(
            header,
            separators=(',', ':'),
            cls=None
        ).encode('utf-8')

        # JWT
        jwt = []
        jwt.append(
            base64.urlsafe_b64encode(json_header).replace(b'=', b''))
        jwt.append(
            base64.urlsafe_b64encode(json_payload).replace(b'=', b''))

        # calculate digest
        tbs_digest = hashes.Hash(
            hashes.SHA256(),
            backend=cryptography.hazmat.backends.default_backend()
        )
        tbs_digest.update(b'.'.join(jwt))
        digest = tbs_digest.finalize()[:32]

        # sign the digest
        signature = bytearray(64)
        if crt_template is None:
            device_private_key_slot = 0
        else:
            device_private_key_slot = crt_template.get('device').private_key_slot,

        assert cal.atcab_sign(
                device_private_key_slot,
                digest,
                signature) == cal.Status.ATCA_SUCCESS, \
            "Signing JWT failed"
        r = int.from_bytes(signature[0:32], byteorder='big', signed=False)
        s = int.from_bytes(signature[32:64], byteorder='big', signed=False)
        sign = utils.encode_dss_signature(r, s)
        jwt.append(
            base64.urlsafe_b64encode(sign).replace(b'=', b''))
        print('JWT:')
        print(jwt)
        print('OK', canvas=b)

    def prompt_gcp_gui(self, qtUiFile, b=None):
        self.connection.execute_gcp_gui(qtUiFile)

    def __config_gcp_credentials(self, b=None):
        print('Configuring Google account...', canvas=b)
        tp_settings = TPSettings()
        acc_setup_path = 'file:///'+os.path.join(
                        tp_settings.get_tpds_core_path(),
                        'docs',
                        'GCP_demo_account_setup.html').replace('\\', '/')
        iot_manifest = os.path.join(
                                tp_settings.get_tpds_core_path(),
                                'docs', 'iot-manifest.json')
        data_view = os.path.join(
                                tp_settings.get_tpds_core_path(),
                                'docs', 'data-view.json')
        csv_file_path = os.path.join(
                                tp_settings.get_tpds_core_path(),
                                'docs',
                                'GCP_test_account_credentials.csv')
        print(f'Loading credentials from {csv_file_path}')

        gcp_file_error = (
            f'GCP json files are unavailable.<br>'
            f'Refer GCP account setup guide for more details.<br>'
            f'<a href={acc_setup_path}>GCP Account Setup Guide: {acc_setup_path}</a>')
        if(not(os.path.exists(iot_manifest) and os.path.exists(data_view))):
            acc_cred_diag = tp_userinput.TPMessageBox(
                title="GCP account json files",
                info=gcp_file_error)
            acc_cred_diag.invoke_dialog()

        msg_box_info = (
            f'<font color=#0000ff><b>Invalid GCP account credentials'
            f'</b></font><br>'
            f'<br>To setup an GCP account, please refer<br>'
            f'<a href={acc_setup_path}>GCP Account Setup Guide: {acc_setup_path}</a>'
            f'<br>To input GCO account credentials, open below CSV file<br>'
            f'<a href={csv_file_path}>GCP Account Credentials CSV File: {csv_file_path}'
            f'</a>')
        if('replace_your' in Path(csv_file_path).read_text()):
            acc_cred_diag = tp_userinput.TPMessageBox(
                title="GCP account credentials",
                info=msg_box_info)
            acc_cred_diag.invoke_dialog()

        self.connection.set_credentials(iot_manifest, data_view,
                                        csv_file_path)
        print(f'Google Registry Id: {self.connection.registry_id}', canvas=b)
        print(f'Google Region: {self.connection.region}', canvas=b)
        with open('google_connect.h', 'w') as f:
            f.write('#ifndef _GOOGLE_CONNECT_H\n')
            f.write('#define _GOOGLE_CONNECT_H\n\n')
            f.write('#include "cryptoauthlib.h"\n\n')
            f.write('#ifdef __cplusplus\n')
            f.write('extern "C" {\n')
            f.write('#endif\n\n')
            f.write(
                f'static const char config_gcp_project_id[] = \
                "{self.connection.project_id}";\n\n')
            f.write(
                f'static const char config_gcp_registry_id[] = \
                "{self.connection.registry_id}";\n\n')
            f.write(
                f'static const char config_gcp_region_id[] = \
                "{self.connection.region}";\n\n')
            f.write('#ifdef __cplusplus\n')
            f.write('}\n')
            f.write('#endif\n')
            f.write('#endif\n')


class GoogleCustomPKI(GoogleConnectBase):
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
        resources = resource_generation.TFLXResources()
        resources.generate_manifest(signer_cert=self.signer_crt,
                                    device_cert=self.device_crt)
        self.manifest_ca_key = 'manifest_ca.key'
        self.manifest_ca_cert = 'manifest_ca.crt'
        self.manifest_file = 'TFLXTLS_devices_manifest.json'
        generate_project_config_h(cert_type='CUSTOM', address=0x6C)

    def register_certificates(self, b=None):
        self.connect_to_cloud(b)
        # Register Device
        print(
            f'Registering {self.device_crt} to Google account...', canvas=b)
        self.connection.register_from_manifest(self.manifest_file,
                                               self.manifest_ca_cert)
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
                    b, self.device_crt, device_crt_template=self.crt_template['device'])


class GoogleIoTAuthentication(GoogleConnectBase):
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
        # Register Device
        print('Registering device to Google account...', canvas=b)
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

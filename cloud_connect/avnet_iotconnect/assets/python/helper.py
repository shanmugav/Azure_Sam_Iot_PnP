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
import flash_program
import secure_element
import tp_utils
from tp_utils.tp_print import print
import tp_utils.tp_input_dialog as tp_userinput
import resource_generation
from certs.tflex_certs import TFLEXCerts
import cryptoauthlib as cal
import certs
from cryptography import x509
from cryptography.hazmat.primitives import serialization
from tp_utils.tp_keys import TPAsymmetricKey
from cryptography.hazmat.primitives.asymmetric import ec, utils
from cryptography.hazmat.primitives import hashes
from create_certs_verif import create_iotconnect_root_verif


def connect_to_prototyping_board(board, b):
    print('Connecting to Secure Element... ', canvas=b)
    if board is None:
        print('Prototyping board MUST be selected!', canvas=b)
        return None
    if board.get_selected_board() is None:
        print('Select board to run an Usecase!', canvas=b)
        return None

    print('FW status: ', end='')
    kit_parser = flash_program.FlashProgram()
    print(kit_parser.check_board_status())
    assert kit_parser.is_board_connected(), \
        'Board not found! Verify I2C switch position and plug USB cable back again.'
    factory_hex = board.get_kit_hex()
    if not kit_parser.is_factory_programmed():
        assert factory_hex, \
            'Factory hex is unavailable to program'
        print('Programming factory hex...', canvas=b)
        tp_settings = tp_utils.tp_settings.TPSettings()
        path = os.path.join(
            tp_settings.get_tpds_core_path(),
            'assets', 'Factory_Program.X',
            factory_hex)
        print(f'Programming {path} file')
        kit_parser.load_hex_image(path)
    element = secure_element.ECC608A(address=0x6C)
    print('OK')
    print('Device details: {}'.format(element.get_device_details()))
    return element


def generate_custom_pki(b):
    resources = resource_generation.TFLXResources()
    mchp_certs = resources.get_mchp_certs_from_device()
    if mchp_certs:
        print('MCHP Certs are available on device')
        if not resources.get_mchp_backup_certs():
            print('MCHP Certs backup is unavailable... Take backup!')
            resources.backup_mchp_certs(mchp_certs)

    text_box_desc = (
        '''<font color=#0000ff><b>User Organization Name Setup</b></font><br>
        <br>The Custom PKI configuration requires additional information about your organization.<br>
        <br>Please enter your Organization Name:<br>''')
    custom_org = tp_userinput.TPInputTextBox(
                                desc=text_box_desc,
                                dialog_title='Avnet IoTConnect')
    custom_org.invoke_dialog()
    print(f'User Organization Name: {custom_org.user_text}', canvas=b)
    assert (
        (custom_org.user_text is not None)
        and (len(custom_org.user_text))), \
        'User Organization name is NOT valid!'

    text_box_desc2 = (
        '''<font color=#0000ff><b>User Company ID Setup</b></font><br>
        <br>The Custom PKI configuration requires additional information about your cloud account.<br>
        <br>Please enter your IoTConnect Company ID:<br>''')
    custom_id = tp_userinput.TPInputTextBox(
                                desc=text_box_desc2,
                                dialog_title='Avnet IoTConnect')
    custom_id.invoke_dialog()
    print(f'User IoTConnect Company ID: {custom_id.user_text}', canvas=b)
    assert (
        (custom_id.user_text is not None)
        and (len(custom_id.user_text))), \
        'User Company ID is NOT valid!'
    resources.generate_custom_pki(custom_org.user_text, custom_id.user_text)
    print(f'', canvas=b)


def generate_verif_cert(b):
    text_box_desc = (
        '''<font color=#0000ff><b>Verification Root Certificate Setup</b></font><br>
        <br>It is required to activate root CA generated during step 1 to allow signed Secure Element to connect to the cloud.<br>
        <br>Please login to your IoTConnect account, upload root CA and copy/paste verification code below.<br>
        <br>Then press OK and finally upload the verification root certificate to your cloud account:<br>
        <font color=#aaaaaa><br>Note: this step only needs to be done once and may be skipped if necessary.</font><br>''')
    verif = tp_userinput.TPInputTextBox(
                                desc=text_box_desc,
                                dialog_title='Avnet IoTConnect')
    verif.invoke_dialog()
    if (verif.user_text is None) or (len(str(verif.user_text)) == 0):
        print(f'No Verification Code provided, skipping Verification Root Certificate generation!', canvas=b)
    else:
        print(f'User Verification Code: {verif.user_text}', canvas=b)
        create_iotconnect_root_verif(verif.user_text)
    print(f'', canvas=b)

def verify_cert_chain(
                    root_crt, root_key,
                    signer_crt, signer_key,
                    device_crt, b):
    custom_certs = TFLEXCerts()
    custom_certs.root.set_certificate(root_crt)
    custom_certs.root.key = TPAsymmetricKey(root_key)
    custom_certs.signer.set_certificate(signer_crt)
    custom_certs.signer.key = TPAsymmetricKey(signer_key)
    custom_certs.device.set_certificate(device_crt)
    crt_template = custom_certs.get_tflex_py_definitions()

    print('Verifying Custom PKI from Secure Element: ', canvas=b)
    signer_cert_der_len = cal.AtcaReference(0)
    assert cal.CertStatus.ATCACERT_E_SUCCESS == cal.atcacert_max_cert_size(
        crt_template['signer'],
        signer_cert_der_len)
    signer_cert_der = bytearray(signer_cert_der_len.value)
    assert cal.CertStatus.ATCACERT_E_SUCCESS == cal.atcacert_read_cert(
        crt_template['signer'],
        custom_certs.root.key.get_public_key().public_bytes(
            format=serialization.PublicFormat.UncompressedPoint,
            encoding=serialization.Encoding.X962)[1:],
        signer_cert_der,
        signer_cert_der_len)
    signer_cert = x509.load_der_x509_certificate(
        signer_cert_der,
        certs.cert_utils.get_backend())

    device_cert_der_len = cal.AtcaReference(0)
    assert cal.CertStatus.ATCACERT_E_SUCCESS == cal.atcacert_max_cert_size(
        crt_template['device'],
        device_cert_der_len)
    device_cert_der = bytearray(device_cert_der_len.value)
    assert cal.CertStatus.ATCACERT_E_SUCCESS == cal.atcacert_read_cert(
        crt_template['device'],
        custom_certs.signer.key.get_public_key().public_bytes(
            format=serialization.PublicFormat.UncompressedPoint,
            encoding=serialization.Encoding.X962)[1:],
        device_cert_der,
        device_cert_der_len)
    device_cert = x509.load_der_x509_certificate(
        device_cert_der,
        certs.cert_utils.get_backend())
    print('OK')

    print('Certs from Device...')
    dev_certs = TFLEXCerts()
    dev_certs.signer.set_certificate(signer_cert)
    dev_certs.device.set_certificate(device_cert)
    print(dev_certs.signer.get_certificate_in_text())
    print(dev_certs.device.get_certificate_in_text())

    print('Processing Root...', end='', canvas=b)
    is_cert_valid = custom_certs.root.is_signature_valid(
        custom_certs.root.certificate.public_key())
    print('Valid' if is_cert_valid else 'Invalid', canvas=b)

    print('Processing Signer...', end='', canvas=b)
    is_cert_valid = dev_certs.signer.is_signature_valid(
        custom_certs.root.certificate.public_key())
    print('Valid' if is_cert_valid else 'Invalid', canvas=b)

    print('Processing Device...', end='', canvas=b)
    is_cert_valid = dev_certs.device.is_signature_valid(
        dev_certs.signer.certificate.public_key())
    print('Valid' if is_cert_valid else 'Invalid', canvas=b)
    print(f'', canvas=b)
    return device_cert, crt_template


def verify_SE_with_random_challenge(device_crt, device_crt_template, b):
    print('Sending random challenge...', canvas=b)
    challenge = os.urandom(32)
    print(f'Challenge: {challenge.hex().upper()}')

    print('Reading response from Secure Element...', canvas=b)
    response = bytearray(64)
    assert cal.atcacert_get_response(
        device_crt_template.private_key_slot,
        challenge, response) == cal.CertStatus.ATCACERT_E_SUCCESS
    print(f'Response: {response.hex().upper()}')

    print('Verifying response...', canvas=b)
    r = int.from_bytes(response[0:32], byteorder='big', signed=False)
    s = int.from_bytes(response[32:64], byteorder='big', signed=False)
    sign = utils.encode_dss_signature(r, s)
    try:
        device_crt.public_key().verify(
            sign, challenge, ec.ECDSA(
                utils.Prehashed(hashes.SHA256())))
    except Exception as err:
        raise ValueError(err)
    print('Valid', canvas=b)
    print(f'', canvas=b)


# Standard boilerplate to call the main() function to begin
# the program.
if __name__ == '__main__':
    pass

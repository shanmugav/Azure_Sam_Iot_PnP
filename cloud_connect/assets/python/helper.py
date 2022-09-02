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
import json
import flash_program
import secure_element
import tp_utils
from tp_utils.tp_print import print
import tp_utils.tp_input_dialog as tp_userinput
from certs.tflex_certs import TFLEXCerts
from tp_utils.tp_keys import TPAsymmetricKey
import resource_generation
import cryptoauthlib as cal
from cryptography.hazmat.primitives import serialization
from cryptography import x509
import certs
from certs.certs_backup import CertsBackup
from certs.cert import Cert
import manifest
from cryptography.hazmat.primitives.asymmetric import ec, utils
from cryptography.hazmat.primitives import hashes
from certs.create_cert_defs import CertDef


def connect_to_prototyping_board(board, b):
    print('Connecting to Secure Element: ', canvas=b)
    if board is None:
        print('Prototyping board MUST be selected!', canvas=b)
        return None
    if board.get_selected_board() is None:
        print('Select board to run Usecase!', canvas=b)
        return None

    print('Connect to Secure Element: ', end='')
    kit_parser = flash_program.FlashProgram()
    print(kit_parser.check_board_status())
    assert kit_parser.is_board_connected(), \
        'Check the Kit parser board connections'
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
        '''<font color=#0000ff><b>Enter Org Name for Custom PKI</b></font><br>
        <br>The organization name entered here would be used to
        generate TFLXTLS certificates.<br>''')
    custom_org = tp_userinput.TPInputTextBox(
                                desc=text_box_desc,
                                dialog_title='CustomPKI Org')
    custom_org.invoke_dialog()
    print(f'User Org Name: {custom_org.user_text}', canvas=b)
    assert (
        (custom_org.user_text is not None)
        and (len(custom_org.user_text))), \
        'Enter valid custom Org name'
    resources.generate_custom_pki(custom_org.user_text)


def verify_cert_chain_custompki(
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

    print('Reading certificates from device: ', canvas=b)
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
    return device_cert, crt_template


def verify_cert_chain(
                    b, signer_crt,
                    device_crt, root_crt=None):
    m_certs = TFLEXCerts()
    m_certs.signer.set_certificate(signer_crt)
    m_certs.device.set_certificate(device_crt)

    print('Verify cert chain...', canvas=b)
    if(root_crt is not None):
        m_certs.root.set_certificate(root_crt)
        is_chain_valid = \
            m_certs.root.is_signature_valid(
                m_certs.root.certificate.public_key()) and \
            m_certs.signer.is_signature_valid(
                m_certs.root.certificate.public_key()) and \
            m_certs.device.is_signature_valid(
                m_certs.signer.certificate.public_key())
    else:
        is_chain_valid = \
            m_certs.device.is_signature_valid(
                m_certs.signer.certificate.public_key())

    if is_chain_valid:
        print('Valid', canvas=b)
        return m_certs.device.certificate
    else:
        print('Invalid', canvas=b)
        return None


def verify_SE_with_random_challenge(b, device_crt, device_crt_template=None):
    print('Generate challenge...', canvas=b)
    challenge = os.urandom(32)
    print(f'OK(Challenge: {challenge.hex().upper()}')

    print('Get response from SE...', canvas=b)
    response = bytearray(64)

    if device_crt_template is None:
        device_private_key_slot = 0
    else:
        device_private_key_slot = device_crt_template.private_key_slot
    assert cal.atcacert_get_response(
        device_private_key_slot,
        challenge, response) == cal.CertStatus.ATCACERT_E_SUCCESS
    print(f'OK(Response: {response.hex().upper()}')

    print('Verify response...', canvas=b)
    r = int.from_bytes(response[0:32], byteorder='big', signed=False)
    s = int.from_bytes(response[32:64], byteorder='big', signed=False)
    sign = utils.encode_dss_signature(r, s)
    try:
        device_crt.public_key().verify(
            sign, challenge, ec.ECDSA(
                utils.Prehashed(hashes.SHA256())))
        print('OK')
    except Exception as err:
        raise ValueError(err)


def get_user_option(b):
    print('Select Manifest option', canvas=b)
    item_list = ['Generate Manifest', 'Upload Manifest']
    dropdown_desc = (
        '''<font color=#0000ff><b>Select Manifest Option</b></font><br>
        <br>Generate Manifest - Generates Manifest file for connected device
        locally<br>
        Upload Manifest - Use existing Manifest file. Requires Manifest
        and its CA files <br>''')
    user_input = tp_userinput.TPInputDropdown(
                                item_list=item_list,
                                desc=dropdown_desc,
                                dialog_title='Manifest Selection')
    user_input.invoke_dialog()
    print(f'Selected option is: {user_input.user_option}', canvas=b)
    assert user_input.user_option is not None, \
        'Select valid Manifest Option'

    return user_input.user_option


def get_user_manifest(b):
    print('Select Manifest JSON file...', canvas=b)
    manifest = tp_userinput.TPInputFileUpload(
                                file_filter=['*.json'],
                                nav_dir=os.getcwd(),
                                dialog_title='Upload Manifest')
    manifest.invoke_dialog()
    print(
        f'Selected manifest file is: {manifest.file_selection}',
        canvas=b)

    print('Select Manifest CA file...', canvas=b)
    manifest_ca = tp_userinput.TPInputFileUpload(
                                file_filter=['*.crt'],
                                nav_dir=os.getcwd(),
                                dialog_title='Upload Manifest CA')
    manifest_ca.invoke_dialog()
    print(
        f'Selected manifest CA file is: {manifest_ca.file_selection}',
        canvas=b)

    return {
        'json_file': manifest.file_selection,
        'ca_cert': manifest_ca.file_selection
    }


def generate_manifest(b, signer_crt, device_crt):
    print('Generating Manifest...', canvas=b)
    resources = resource_generation.TFLXResources()
    resources.generate_manifest(signer_crt, device_crt)
    print('Completed', canvas=b)

    return {
        'json_file': 'TFLXTLS_devices_manifest.json',
        'ca_cert': 'manifest_ca.crt'
    }


def extract_certs_from_manifest(b, device_sn, manifest_obj, key_slot=0):
    device_manifest = manifest_obj.get('json_file')
    device_manifest_ca = manifest_obj.get('ca_cert')

    if device_manifest is None or device_manifest_ca is None:
        return None

    if os.path.exists(device_manifest) \
            and device_manifest.endswith('.json'):
        with open(device_manifest) as json_data:
            device_manifest = json.load(json_data)

    if not isinstance(device_manifest, list):
        raise ValueError('Unsupported manifest format to process')

    manifest_ca = certs.Cert()
    manifest_ca.set_certificate(device_manifest_ca)
    iterator = manifest.ManifestIterator(device_manifest)
    while iterator.index != 0:
        se = manifest.Manifest().decode_manifest(
                iterator.__next__(), manifest_ca.certificate)
        if se.get('uniqueId') == str(device_sn.hex()):
            se_certs = manifest.Manifest().extract_public_data_pem(se)
            slot = next((sub for sub in se_certs if sub.get(
                'id') == str(key_slot)), None)
            return slot.get('certs')

    return None


def restore_mchp_certs_on_device(device_sn, b):
    resources = resource_generation.TFLXResources()
    mchp_certs = resources.get_mchp_certs_from_device()
    is_chain_valid = False
    if(mchp_certs is not None):
        is_chain_valid = \
            mchp_certs.get('root').is_signature_valid(
                mchp_certs.get('root').certificate.public_key()) and \
            mchp_certs.get('signer').is_signature_valid(
                mchp_certs.get('root').certificate.public_key()) and \
            mchp_certs.get('device').is_signature_valid(
                mchp_certs.get('signer').certificate.public_key())
    r_manifest = None  # Return manifest file
    if is_chain_valid and mchp_certs:
        print('MCHP Certs are available on the device')
    else:
        print('MCHP Certs are not available on the device... Trying from backup')
        # Read from the backup file or manifest file
        certs_backup = CertsBackup()
        mchp_certs = certs_backup.fetch_from_file(device_sn=device_sn)
        if not len(mchp_certs):
            print('MCHP Certs backup is not available... Provide Manifest file')
            # if not available in backup file, prompt for manifest file
            r_manifest = get_user_manifest(b)
            device_certs_from_manifest = extract_certs_from_manifest(
                                                    b, device_sn, r_manifest)
            if device_certs_from_manifest and len(device_certs_from_manifest) >= 2:
                mchp_certs.update({'root': None})
                mchp_certs.update({'signer': device_certs_from_manifest[1]})
                mchp_certs.update({'device': device_certs_from_manifest[0]})

        if len(mchp_certs):
            signer_cert = Cert()
            signer_cert.set_certificate(mchp_certs.get('signer'))
            device_cert = Cert()
            device_cert.set_certificate(mchp_certs.get('device'))
            certs = TFLEXCerts()
            certs.set_tflex_certificates(
                            None,
                            signer_cert.certificate,
                            device_cert.certificate)

            template = dict()

            cert_def = CertDef()
            cert_def.set_certificate(
                certs.signer.certificate, None, template_id=1)
            template.update({'signer': cert_def.get_py_definition()})

            cert_def = CertDef()
            cert_def.set_certificate(
                certs.device.certificate, certs.signer.certificate, template_id=2)
            template.update({'device': cert_def.get_py_definition()})

            # write signer and device cert into device
            # template = certs.get_tflex_py_definitions()
            assert cal.atcacert_write_cert(
                    template.get('signer'),
                    certs.signer.get_certificate_in_der(),
                    len(certs.signer.get_certificate_in_der())) \
                == cal.Status.ATCA_SUCCESS, \
                "Loading signer certificate into slot failed"
            assert cal.atcacert_write_cert(
                    template.get('device'),
                    certs.device.get_certificate_in_der(),
                    len(certs.device.get_certificate_in_der())) \
                == cal.Status.ATCA_SUCCESS, \
                "Loading device certificate into slot failed"
            print('OK')
        else:
            raise ValueError((
                f'Unable to restore MCHP certs on the device...\n\n'
                f'Please try with another SE that has MCHP certs.'
            ))

        mchp_certs.update({'signer': signer_cert})
        mchp_certs.update({'device': device_cert})
        if(mchp_certs.get('root') is not None):
            root_cert = Cert()
            root_cert.set_certificate(mchp_certs.get('root'))
            mchp_certs.update({'root': root_cert})
    return mchp_certs, r_manifest


def generate_project_config_h(cert_type='MCHP', address=0x6A, azure_support=False):
    with open('project_config.h', 'w') as f:
        f.write(f'#ifndef _PROJECT_CONFIG_H\n')
        f.write(f'#define _PROJECT_CONFIG_H\n\n')

        f.write('#ifdef __cplusplus\n')
        f.write('extern "C" {\n')
        f.write('#endif\n\n')
        if cert_type == 'MCHP':
            f.write('#define CLOUD_CONNECT_WITH_MCHP_CERTS\n')
        else:
            f.write('#define CLOUD_CONNECT_WITH_CUSTOM_CERTS\n')
        if azure_support:
            f.write('#define DEVICE_CERT_SUPPORTS_AZURE_CONNECTION\n')
        f.write(f'#define SECURE_ELEMENT_ADDRESS 0x{address:02X}\n\n')
        f.write('#ifdef __cplusplus\n')
        f.write('}\n')
        f.write('#endif\n')
        f.write('#endif\n')

from pathlib import Path
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric import ec
from cryptoauthlib import *
from datetime import datetime, timedelta, timezone


def load_or_create_key_pair(filename):
    """
    Load an EC P256 private key from file or create a new one a save it if the
    key file doesn't exist.
    """
    filename = Path(filename)
    priv_key = None
    if filename.is_file():
        # Load existing key
        with open(str(filename), 'rb') as f:
            priv_key = serialization.load_pem_private_key(
                data=f.read(),
                password=None,
                backend=None)
    if priv_key is None:
        # No existing private key found, generate new private key
        priv_key = ec.generate_private_key(
            curve=ec.SECP256R1(),
            backend=None)

        # Save private key to file
        with open(str(filename), 'wb') as f:
            pem_key = priv_key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.PKCS8,
                encryption_algorithm=serialization.NoEncryption())
            f.write(pem_key)
    return priv_key

def create_iotconnect_root_verif(verif_code, root_key_path='root_key.key', root_verif_path='root_verification.cer'):
        """
        Create a verification certificate to be sent to the Avnet Cloud to give a proof of possession of the
        root certificate private key using verification code provided by the server. This is mandatory for
        the Avnet IoTConnect cloud to activate the root CA and later verify the device certificate chain.
        """
        # Extract Common Name and Organization Name from root CA
        root_common_name=''
        root_org_name=''
        with open("root_crt.crt", "rb") as f:
            cert_info = x509.load_pem_x509_certificate(f.read(), default_backend())
            root_common_name = cert_info.issuer.get_attributes_for_oid(NameOID.COMMON_NAME)[0].value
            root_org_name = cert_info.issuer.get_attributes_for_oid(NameOID.ORGANIZATION_NAME)[0].value

        #print('------------- Creating Manifest Log Signer ----------- ')
        print('\nGenerating root verification certificate...', end='')
        # Load the root CA key pair
        #print('Loading Manifest logger key')
        root_private_key = load_or_create_key_pair(root_key_path)

        # Create root CA verification certificate
        #print('Generating self-signed logging certificate')
        builder = x509.CertificateBuilder()
        builder = builder.serial_number(x509.random_serial_number())

        name = x509.Name(
            [x509.NameAttribute(x509.oid.NameOID.ORGANIZATION_NAME, root_org_name),
            x509.NameAttribute(x509.oid.NameOID.COMMON_NAME, root_common_name)])
        valid_date = datetime.utcnow().replace(tzinfo=timezone.utc)

        builder = builder.issuer_name(name)
        builder = builder.not_valid_before(valid_date)
        builder = builder.not_valid_after(valid_date + timedelta(days=365 * 25))
        builder = builder.subject_name(x509.Name([x509.NameAttribute(x509.oid.NameOID.COMMON_NAME, verif_code)]))
        builder = builder.public_key(root_private_key.public_key())
        builder = builder.add_extension(
            x509.SubjectKeyIdentifier.from_public_key(root_private_key.public_key()),
            critical=False)
        builder = builder.add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True)

        # Self-sign certificate
        root_verif_cert = builder.sign(
            private_key=root_private_key,
            algorithm=hashes.SHA256(),
            backend=default_backend())

        root_verif_cert_data = root_verif_cert.public_bytes(encoding=serialization.Encoding.PEM)

        # Write root CA certificate to file
        with open(root_verif_path, 'wb') as f:
            f.write(root_verif_cert_data)
            print('OK (saved to ' + f.name + ')')

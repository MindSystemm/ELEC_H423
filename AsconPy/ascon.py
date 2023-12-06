import sys
import numpy as np
sys.path.append("./build/")
import AsconPy

help(AsconPy)

# Testing
key = np.array([0x0e, 0xdc, 0x97, 0xa7, 0x95, 0xc1, 0x08, 0x65, 0xbf, 0x7d, 0xac, 0x1c, 0xc8, 0x53, 0x23, 0x48], np.uint8)
ascon = AsconPy.Ascon(key)

plaintext_str = np.frombuffer(b'This should be encrypted!', dtype=np.uint8)
authdata_str = np.frombuffer(b'This should be authenticated!', dtype=np.uint8)

cipher = ascon.encrypt(plaintext_str, len(plaintext_str), authdata_str, len(authdata_str))
print(cipher)

plaintext = ascon.decrypt(cipher, len(cipher), authdata_str, len(authdata_str))
print(plaintext)
print(f"String: {''.join([chr(b) for b in plaintext])}")
#include "Ascon.h"
#include "Classes.h"

#define CRYPTO_KEY {0x0e, 0xdc, 0x97, 0xa7, 0x95, 0xc1, 0x08, 0x65, 0xbf, 0x7d, 0xac, 0x1c, 0xc8, 0x53, 0x23, 0x48}

int main(int argc, char* argv[]) {
    /*
    CryptoData* cryptodata = (CryptoData*) malloc(sizeof(CryptoData));
    uint8_t iv[IV_LENGTH] = {0xC9,0x9E,0x7C,0x7A,0x0B,0x53,0xF5,0xFC,0x85,0xCA,0x42,0xC5,0x32,0x93,0xCB,0xBE};
    uint8_t tag[TAG_LENGTH] = {0x0B,0x91,0x6C,0x10,0xD3,0xD6,0xCA,0x94,0x1F,0x6C,0x4F,0xD8,0xB6,0x80,0xEF,0xFC};
    memcpy(cryptodata->header.iv, iv, IV_LENGTH);
    memcpy(cryptodata->header.tag, tag, TAG_LENGTH);
    cryptodata->header.cipher_l = 34;
    uint8_t cipher[34] = {0xA3,0x09,0x2F,0x43,0x43,0x77,0xA7,0xAB,0x16,0x0B,0x7E,0x56,0xD9,0x58,0xA9,0x51,0x83,0xDE,0xED,0xA9,0x34,0x71,0xDA,0xBB,0x93,0x1A,0x63,0xC6,0xD6,0x51,0x22,0x1F,0x0E,0xCC};
    cryptodata->cipher = (uint8_t*) malloc(34);
    memcpy(cryptodata->cipher, cipher, 34);

    print_ascon(cryptodata);

    // Decrypt
    init_nonce();
    uint8_t key[KEY_LENGTH] = CRYPTO_KEY;
    char authdata_str[] = "This should be authenticated!";
    Plaintext* plaintext = decrypt_auth(key, cryptodata, (uint8_t*) authdata_str, sizeof(authdata_str));

    if (plaintext == nullptr) {
        printf("The plaintext was null");
    } else {
        print_ascon(plaintext);
        printf("Decrypted: %s\n", (char*) plaintext->plaintext);
    }

    free_crypto(cryptodata);
    free_plaintext(plaintext);
    */

    uint8_t key[KEY_LENGTH] = CRYPTO_KEY;
    Ascon ascon(key);

    char plaintext_str[] = "This should be encrypted!";
    char authdata_str[] = "This should be authenticated!";
    ByteArray cipher = ascon.encrypt((uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));

    if (cipher.length == 0) {
        printf("Cipher was null");
    } else {
        //print_byte_array(cipher);
    }

    ByteArray plaintext = ascon.decrypt(cipher.data, cipher.length, (uint8_t*) authdata_str, sizeof(authdata_str));

    if (plaintext.length == 0) {
        printf("Plaintext was null");
    } else {
        //print_byte_array(plaintext);
        printf("Plaintext: %s\n", (char*) plaintext.data);
    }
}
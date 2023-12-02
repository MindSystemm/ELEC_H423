#include "Ascon.h"

// Global variable made once and reused
// No mutlithreading so safe to be global
Ascon128 ascon;

void free_crypto(CryptoData* d) {
    free(d->cipher);
    free(d);
}

void free_plaintext(Plaintext* p) {
    free(p->plaintext);
    free(p);
}

bool setup_ascon(uint8_t key[KEY_LENGTH], uint8_t* iv) {
    ascon.clear();
    if (!ascon.setKey(key, KEY_LENGTH)){ return false; }
    if (!ascon.setIV(iv, IV_LENGTH)){ return false; }
    return true;
}

// Encrypt and authenticate the plaintext + authenticate the authdata
CryptoData* encrypt_auth(uint8_t* plaintext, uint8_t plaintext_l, uint8_t* authdata, uint8_t authdata_l) {
    uint8_t key[KEY_LENGTH] = CRYPTO_KEY;
    
    CryptoData* cryptoData = (CryptoData*) malloc(sizeof(CryptoData));

    // Setup header
    esp_fill_random(cryptoData->header.iv, IV_LENGTH);
    cryptoData->header.cipher_l = plaintext_l;
    
    // Allocate data
    cryptoData->cipher = (uint8_t*) malloc(plaintext_l); // The output buffer must have at least as many bytes as the input buffer. 

    // Setup ascon
    if (!setup_ascon(key, cryptoData->header.iv)) {
        free_crypto(cryptoData);
        return nullptr;
    }

    // Authdata
    ascon.addAuthData(authdata, authdata_l);
    // Encryption
    ascon.encrypt(cryptoData->cipher, plaintext, plaintext_l);
    // Tag
    ascon.computeTag(cryptoData->header.tag, TAG_LENGTH);

    return cryptoData;
}

// Decrypt and check authentication of the plaintext + check authentication of the authdata
Plaintext* decrypt_auth(CryptoData* cryptoData, uint8_t* authdata, uint8_t authdata_l) {
    uint8_t key[KEY_LENGTH] = CRYPTO_KEY;

    Plaintext* plaintext = (Plaintext*) malloc(sizeof(Plaintext));
    plaintext->plaintext_l = cryptoData->header.cipher_l; // The output buffer must have at least as many bytes as the input buffer. 
    plaintext->plaintext = (uint8_t*) malloc(plaintext->plaintext_l);
    
    // Setup ascon
    if (!setup_ascon(key, cryptoData->header.iv)) { return nullptr; }

    // Authdata
    ascon.addAuthData(authdata, authdata_l);
    // Decryption
    ascon.decrypt(plaintext->plaintext, cryptoData->cipher, cryptoData->header.cipher_l);
    // Check authentication (aka tag)
    if (!ascon.checkTag(cryptoData->header.tag, TAG_LENGTH)) {
        // Authentication check failed -> discard
        free_plaintext(plaintext);
        free_crypto(cryptoData);
        return nullptr;
    }

    free_crypto(cryptoData);
    return plaintext;
}
#include "Ascon.h"

// Global variables made once and reused
// No mutlithreading so safe to be global
Ascon128 ascon;
uint64_t last_nonce;

void init_nonce() {
  last_nonce = EEPROM.readULong64(NONCE_ADDR);
  if (last_nonce < NONCE_MARGIN || last_nonce == UINT64_MAX) {
    last_nonce = NONCE_MARGIN;
  }
  uint64_t x = -1;
  Serial.printf("Nonce loaded: %llu\n", last_nonce);
}

void update_nonce(uint64_t nonce) {
  if (nonce > last_nonce) {
    last_nonce = nonce;
    EEPROM.writeULong64(NONCE_ADDR, last_nonce);
    EEPROM.commit();
    Serial.printf("Nonce updated to: %llu\n", last_nonce);
  }
}

bool replay(uint64_t nonce) {
  return nonce < (last_nonce - NONCE_MARGIN);
}

void free_crypto(CryptoData* d) {
    free(d->cipher);
    free(d);
}

void free_plaintext(Plaintext* p) {
    free(p->plaintext);
    free(p);
}

char* byte_arr_to_str(byte* arr, uint64_t l) {
    uint64_t total_str_length = 1 + ((l-1)*5) + 4 + 1 + 1; // '[' + array of '0xXX,' + last 0xXX + ']' + '\0'
    char* bffr = (char*) malloc(total_str_length);
    char* ptr = bffr;

    *ptr = '[';
    ptr++;

    // Add array data
    for (uint64_t i = 0; i < l; i++) {
        if (i + 1 == l) {
            // Last element => no comma
            sprintf(ptr, "0x%02X", arr[i]);
            ptr += 4;
        } else {
            sprintf(ptr, "0x%02X,", arr[i]);
            ptr += 5;
        }
    }

    *ptr = ']';
    ptr++;
    *ptr = '\0';
    
    return bffr;
}

void print_ascon(CryptoData* d) {
    Serial.printf("CryptoData:\n");

    if (d == nullptr) {
        Serial.printf("\tNULL\n");
        return;
    }

    Serial.printf("\theader:\n");
    
    // Print header
    char* arr = byte_arr_to_str(d->header.iv, IV_LENGTH);
    Serial.printf("\t\tiv: %s\n", arr);
    free(arr);
    arr = byte_arr_to_str(d->header.tag, TAG_LENGTH);
    Serial.printf("\t\ttag: %s\n", arr);
    free(arr);
    Serial.printf("\t\tcipher_l: %d\n", d->header.cipher_l);

    // Print data
    arr = byte_arr_to_str(d->cipher, d->header.cipher_l);
    Serial.printf("\tcipher: %s\n", arr);
    free(arr);
}

void print_ascon(Plaintext* p) {
    Serial.printf("Plaintext:\n");

    if (p == nullptr) {
        Serial.printf("\tNULL\n");
        return;
    }

    // Print data
    Serial.printf("\tplaintext_l: %d\n", p->plaintext_l);
    char* arr = byte_arr_to_str(p->plaintext, p->plaintext_l);
    Serial.printf("\tplaintext: %s\n", arr);
    free(arr);
}

bool setup_ascon(uint8_t key[KEY_LENGTH], uint8_t* iv) {
    ascon.clear();
    if (!ascon.setKey(key, KEY_LENGTH)){ return false; }
    if (!ascon.setIV(iv, IV_LENGTH)){ return false; }
    return true;
}

// Adds the 64 nonce to the last 8 bytes of a byte array
void add_nonce(uint8_t* arr, uint64_t l) {
    // Move pointer to last byte
    arr = arr + (l - 1);
    uint64_t nonce = last_nonce;
        
    for (uint64_t i = 0; i < 8; i++){
        *(arr - i) = nonce & 0xFF;
        nonce = nonce >> 8;
    }
}

// Extracts nonce from the last 8 bytes of a byte array
uint64_t extract_nonce(uint8_t* arr, uint64_t l) {
    // Move pointer to last 4 bytes
    arr = arr + (l - 8);
    uint64_t nonce = 0;

    for (uint64_t i = 0; i < 8; i++){
        nonce = nonce << 8;
        nonce = nonce | arr[i];
        
        // Remove nonce
        arr[i] = 0;
    }

    return nonce;
}

// Encrypt and authenticate the plaintext + authenticate the authdata
CryptoData* encrypt_auth(uint8_t key[KEY_LENGTH], uint8_t* plaintext, uint64_t plaintext_l, uint8_t* authdata, uint64_t authdata_l) {
    CryptoData* cryptoData = (CryptoData*) malloc(sizeof(CryptoData));

    char* auth = byte_arr_to_str(authdata, authdata_l);
    Serial.printf("Auth: %s\n", auth);
    free(auth);

    // Add nonce
    plaintext_l += 8; // + 8 because nonce has to be added
    uint8_t* plaintext_bffr = (uint8_t*) malloc(plaintext_l);
    memcpy(plaintext_bffr, plaintext, plaintext_l);
    update_nonce(last_nonce + 1);
    add_nonce(plaintext_bffr, plaintext_l);

    // Setup header
    esp_fill_random(cryptoData->header.iv, IV_LENGTH);
    cryptoData->header.cipher_l = plaintext_l;
    
    // Allocate data
    cryptoData->cipher = (uint8_t*) malloc(cryptoData->header.cipher_l); // The output buffer must have at least as many bytes as the input buffer. 

    // Setup ascon
    if (!setup_ascon(key, cryptoData->header.iv)) {
        free_crypto(cryptoData);
        free(plaintext_bffr);
        return nullptr;
    }

    // Authdata
    uint8_t* authdata_c = (uint8_t*) malloc(authdata_l); // Need to make a copy since authdata would otherwise be altered
    memcpy(authdata_c, authdata, authdata_l);
    ascon.addAuthData(authdata_c, authdata_l);
    free(authdata_c);
    // Encryption
    ascon.encrypt(cryptoData->cipher, plaintext_bffr, plaintext_l);
    // Tag
    ascon.computeTag(cryptoData->header.tag, TAG_LENGTH);

    free(plaintext_bffr);

    return cryptoData;
}

// Decrypt and check authentication of the plaintext + check authentication of the authdata
Plaintext* decrypt_auth(uint8_t key[KEY_LENGTH], CryptoData* cryptoData, uint8_t* authdata, uint64_t authdata_l) {
    if (cryptoData->header.cipher_l < 8) { return nullptr; } // No room for nonce
    
    Plaintext* plaintext = (Plaintext*) malloc(sizeof(Plaintext));
    plaintext->plaintext_l = cryptoData->header.cipher_l - 8; // The output buffer must have at least as many bytes as the input buffer. - 8 because the nonce at the end will be removed
    plaintext->plaintext = (uint8_t*) malloc(plaintext->plaintext_l);
    
    // Setup ascon
    if (!setup_ascon(key, cryptoData->header.iv)) { return nullptr; }

    // Authdata
    uint8_t* authdata_c = (uint8_t*) malloc(authdata_l); // Need to make a copy since authdata would otherwise be altered
    memcpy(authdata_c, authdata, authdata_l);
    ascon.addAuthData(authdata_c, authdata_l);
    free(authdata_c);
    // Decryption
    uint8_t plaintext_bffr_l = plaintext->plaintext_l + 8;
    uint8_t* plaintext_bffr = (uint8_t*) malloc(plaintext_bffr_l);
    ascon.decrypt(plaintext_bffr, cryptoData->cipher, cryptoData->header.cipher_l);
    memcpy(plaintext->plaintext, plaintext_bffr, plaintext->plaintext_l);
    // Check authentication (aka tag)
    if (!ascon.checkTag(cryptoData->header.tag, TAG_LENGTH)) {
        // Authentication check failed -> discard
        free_plaintext(plaintext);
        free(plaintext_bffr);
        return nullptr;
    }
    // Check nonce
    uint64_t nonce = extract_nonce(plaintext_bffr, plaintext_bffr_l);
    if (replay(nonce)) {
        // Nonce check failed -> discard
        free_plaintext(plaintext);
        free(plaintext_bffr);
        return nullptr;
    }
    update_nonce(nonce);

    free(plaintext_bffr);

    return plaintext;
}
#include "Ascon.h"

// Global variables made once and reused
// No mutlithreading so safe to be global
Ascon128 ascon;
uint64_t last_nonce;

void init_nonce() {
  last_nonce = 0; // TODO: Read from file
  if (last_nonce < NONCE_MARGIN || last_nonce == UINT64_MAX) {
    last_nonce = NONCE_MARGIN;
  }
  uint64_t x = -1;
  printf("Nonce loaded: %llu\n", last_nonce);
}

void update_nonce(uint64_t nonce) {
  if (nonce > last_nonce) {
    last_nonce = nonce;
    // TODO: write nonce to file
    printf("Nonce updated to: %llu\n", last_nonce);
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

char* byte_arr_to_str(uint8_t* arr, uint64_t l) {
    uint64_t total_str_length = 1 + ((l-1)*5) + 4 + 1 + 1; // '[' + array of '0xXX,' + last 0xXX + ']' + '\0'
    char* bffr = (char*) malloc(total_str_length);
    char* ptr = bffr;
    printf("byte_arr_to_str 1\n");
    printf("l: %zu\n", l);
    printf("total_str_length: %zu\n", total_str_length);
    printf("bffr: %p\n", bffr);
    printf("ptr: %p\n", ptr);

    *ptr = '[';
    ptr++;

    printf("byte_arr_to_str 1.1\n");

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
    printf("byte_arr_to_str 2\n");

    *ptr = ']';
    ptr++;
    *ptr = '\0';

    printf("byte_arr_to_str 3\n");
    
    return bffr;
}

void print_ascon(CryptoData* d) {
    printf("CryptoData:\n");

    if (d == nullptr) {
        printf("\tNULL\n");
        return;
    }

    printf("\theader:\n");
    
    // Print header
    char* arr = byte_arr_to_str(d->header.iv, IV_LENGTH);
    printf("\t\tiv: %s\n", arr);
    free(arr);
    arr = byte_arr_to_str(d->header.tag, TAG_LENGTH);
    printf("\t\ttag: %s\n", arr);
    free(arr);
    printf("\t\tcipher_l: %zu\n", d->header.cipher_l);

    // Print data
    printf("p: %p\n", d->cipher);
    arr = byte_arr_to_str(d->cipher, d->header.cipher_l);
    printf("\tcipher: %s\n", arr);
    free(arr);
}

void print_ascon(Plaintext* p) {
    printf("Plaintext:\n");

    if (p == nullptr) {
        printf("\tNULL\n");
        return;
    }

    // Print data
    printf("\tplaintext_l: %d\n", p->plaintext_l);
    char* arr = byte_arr_to_str(p->plaintext, p->plaintext_l);
    printf("\tplaintext: %s\n", arr);
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

// TODO: Make random
void fill_iv(uint8_t* iv, uint64_t l) {
    for (uint64_t i = 0; i < l; i++){
        iv[i] = 0;
    }
}

// Encrypt and authenticate the plaintext + authenticate the authdata
CryptoData* encrypt_auth(uint8_t key[KEY_LENGTH], uint8_t* plaintext, uint64_t plaintext_l, uint8_t* authdata, uint64_t authdata_l) {
    CryptoData* cryptoData = (CryptoData*) malloc(sizeof(CryptoData));

    // Add nonce
    plaintext_l += 8; // + 8 because nonce has to be added
    uint8_t* plaintext_bffr = (uint8_t*) malloc(plaintext_l);
    memcpy(plaintext_bffr, plaintext, plaintext_l);
    update_nonce(last_nonce + 1);
    add_nonce(plaintext_bffr, plaintext_l);

    // Setup header
    fill_iv(cryptoData->header.iv, IV_LENGTH);
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
    printf("decrypt 1.1\n"); 
    char* auth = byte_arr_to_str(authdata, authdata_l);
    printf("Auth: %s\n", auth);
    free(auth);
    Plaintext* plaintext = (Plaintext*) malloc(sizeof(Plaintext));
    plaintext->plaintext_l = cryptoData->header.cipher_l - 8; // The output buffer must have at least as many bytes as the input buffer. - 8 because the nonce at the end will be removed
    plaintext->plaintext = (uint8_t*) malloc(plaintext->plaintext_l);
    printf("decrypt 1.2\n"); 
    // Setup ascon
    if (!setup_ascon(key, cryptoData->header.iv)) { return nullptr; }
    printf("decrypt 1.3\n"); 
    // Authdata
    uint8_t* authdata_c = (uint8_t*) malloc(authdata_l); // Need to make a copy since authdata would otherwise be altered
    memcpy(authdata_c, authdata, authdata_l);
    ascon.addAuthData(authdata_c, authdata_l);
    free(authdata_c);
    printf("decrypt 1.4\n"); 
    // Decryption
    uint8_t plaintext_bffr_l = plaintext->plaintext_l + 8;
    uint8_t* plaintext_bffr = (uint8_t*) malloc(plaintext_bffr_l);
    printf("decrypt 1.4.1\n"); 
    printf("l:%d\n", plaintext_bffr_l);
    printf("p:%p\n", plaintext_bffr);
    ascon.decrypt(plaintext_bffr, cryptoData->cipher, cryptoData->header.cipher_l);
    printf("decrypt 1.4.2\n"); 
    memcpy(plaintext->plaintext, plaintext_bffr, plaintext->plaintext_l);
    printf("decrypt 1.5\n"); 
    // Check authentication (aka tag)
    if (!ascon.checkTag(cryptoData->header.tag, TAG_LENGTH)) {
        // Authentication check failed -> discard
        free_plaintext(plaintext);
        free(plaintext_bffr);
        return nullptr;
    }
    printf("decrypt 1.6\n"); 
    // Check nonce
    uint64_t nonce = extract_nonce(plaintext_bffr, plaintext_bffr_l);
    if (replay(nonce)) {
        // Nonce check failed -> discard
        free_plaintext(plaintext);
        free(plaintext_bffr);
        return nullptr;
    }
    update_nonce(nonce);
    printf("decrypt 1.7\n"); 

    free(plaintext_bffr);
    printf("decrypt 1.8\n"); 
    return plaintext;
}
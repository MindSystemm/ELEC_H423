#ifndef CLASSES_H
#define CLASSES_H

#include "Ascon.h"
#include <tuple>

typedef struct {
    uint8_t* data;
    size_t length;
} Byte_Array;

void free_byte_array(Byte_Array* b);

void print_byte_array(Byte_Array* b);

class Ascon {
    uint8_t key[KEY_LENGTH];

    private:
        Byte_Array* cryptodata_to_byte_arr(CryptoData* d) {
            size_t return_size = this->get_needed_size(d->header.cipher_l);
            size_t cryptodata_header_size = return_size - d->header.cipher_l;

            Byte_Array* byte_array = (Byte_Array*) malloc(sizeof(Byte_Array));

            byte_array->length = return_size;
            byte_array->data = (uint8_t*) malloc(byte_array->length);

            // Copy cryptodata without cipher pointer and cipher data
            memcpy(byte_array->data, d, cryptodata_header_size);

            // append cipher data
            memcpy(byte_array->data + cryptodata_header_size, d->cipher, return_size - cryptodata_header_size);

            return byte_array;
        }

        Byte_Array* plaintext_to_byte_arr(Plaintext* p) {
            Byte_Array* byte_array = (Byte_Array*) malloc(sizeof(Byte_Array));

            byte_array->length = p->plaintext_l;
            byte_array->data = (uint8_t*) malloc(byte_array->length);

            // Copy plaintext array
            memcpy(byte_array->data, p->plaintext, p->plaintext_l);

            return byte_array;
        }

        CryptoData* cryptodata_from_byte_arr(uint8_t* arr, size_t l) {
            size_t cryptodata_header_size = this->get_needed_size(0);
            size_t cipher_size = l - cryptodata_header_size;

            CryptoData* cryptodata = (CryptoData*) malloc(sizeof(CryptoData));

            // Copy cryptodata without cipher pointer and cipher data
            memcpy(cryptodata, arr, cryptodata_header_size);

            // append cipher data
            cryptodata->cipher = (uint8_t*) malloc(cipher_size);
            memcpy(cryptodata->cipher, arr + cryptodata_header_size, cipher_size);

            return cryptodata;
        }

        size_t get_needed_size(size_t plaintext_l) {
            return sizeof(CryptoData) - sizeof(uint8_t*) + (plaintext_l * sizeof(uint8_t));
        }
    
    public:
        Ascon(uint8_t key[KEY_LENGTH]) {
            init_nonce();
            memcpy(this->key, key, KEY_LENGTH);
        }

        Byte_Array* encrypt(uint8_t* plaintext, size_t plaintext_l, uint8_t* authdata, size_t authdata_l) {
            CryptoData* cryptodata = encrypt_auth(this->key, plaintext, plaintext_l, authdata, authdata_l);

            if (cryptodata == nullptr) {
                return nullptr;
            }

            Byte_Array* byte_array = this->cryptodata_to_byte_arr(cryptodata);

            free_crypto(cryptodata);

            return byte_array;
        }

        Byte_Array* decrypt(uint8_t* cipher_data, size_t cipher_data_l, uint8_t* authdata, size_t authdata_l) {
            CryptoData* cryptodata = this->cryptodata_from_byte_arr(cipher_data, cipher_data_l);
            
            Plaintext* plaintext = decrypt_auth(this->key, cryptodata, authdata, authdata_l);

            if (plaintext == nullptr) {
                free_crypto(cryptodata);
                return nullptr;
            }

            Byte_Array* byte_array = this->plaintext_to_byte_arr(plaintext);

            free_crypto(cryptodata);
            free_plaintext(plaintext);

            return byte_array;
        }
};

#endif
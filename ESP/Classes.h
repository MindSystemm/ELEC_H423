#ifndef CLASSES_H
#define CLASSES_H

#include "Ascon.h"
#include <tuple>

class ByteArray {
    public:
        uint8_t* data = nullptr;
        uint64_t length = 0;
};

class Ascon {
    uint8_t key[KEY_LENGTH];

    private:
        ByteArray cryptodata_to_byte_arr(CryptoData* d) {
            uint64_t return_size = this->get_needed_size(d->header.cipher_l);
            uint64_t cryptodata_header_size = return_size - d->header.cipher_l;

            ByteArray byte_array;

            byte_array.length = return_size;
            byte_array.data = (uint8_t*) malloc(byte_array.length);

            // Copy cryptodata without cipher pointer and cipher data
            memcpy(byte_array.data, d, cryptodata_header_size);

            // append cipher data
            memcpy(byte_array.data + cryptodata_header_size, d->cipher, return_size - cryptodata_header_size);

            return byte_array;
        }

        ByteArray plaintext_to_byte_arr(Plaintext* p) {
            ByteArray byte_array;

            byte_array.length = p->plaintext_l;
            byte_array.data = (uint8_t*) malloc(byte_array.length);

            // Copy plaintext array
            memcpy(byte_array.data, p->plaintext, p->plaintext_l);

            return byte_array;
        }

        CryptoData* cryptodata_from_byte_arr(uint8_t* arr, uint64_t l) {
            uint64_t cryptodata_header_size = this->get_needed_size(0);
            uint64_t cipher_size = l - cryptodata_header_size;

            CryptoData* cryptodata = (CryptoData*) malloc(sizeof(CryptoData));

            // Copy cryptodata without cipher pointer and cipher data
            memcpy(cryptodata, arr, cryptodata_header_size);

            // append cipher data
            cryptodata->cipher = (uint8_t*) malloc(cipher_size);
            memcpy(cryptodata->cipher, arr + cryptodata_header_size, cipher_size);

            return cryptodata;
        }

        uint64_t get_needed_size(uint64_t plaintext_l) {
            return sizeof(CryptoData) + (plaintext_l * sizeof(uint8_t));
        }
    
    public:
        Ascon(uint8_t* key) {
            init_nonce();
            memcpy(this->key, key, KEY_LENGTH);
        }

        ByteArray encrypt(uint8_t* plaintext, uint64_t plaintext_l, uint8_t* authdata, uint64_t authdata_l) {
            CryptoData* cryptodata = encrypt_auth(this->key, plaintext, plaintext_l, authdata, authdata_l);

            if (cryptodata == nullptr) {
                ByteArray null;
                null.data = nullptr;
                null.length = 0;
                return null;
            }

            Serial.println("------ Encryption ------");
            print_ascon(cryptodata);
            Serial.println("------------------------");

            ByteArray byte_array = this->cryptodata_to_byte_arr(cryptodata);

            free_crypto(cryptodata);

            return byte_array;
        }

        ByteArray decrypt(uint8_t* cipher_data, uint64_t cipher_data_l, uint8_t* authdata, uint64_t authdata_l) {
            if (cipher_data_l < sizeof(CryptoData) - sizeof(uint8_t*)) {
                // Not enough data
                ByteArray null;
                null.data = nullptr;
                null.length = 0;
                return null;
            }
            
            CryptoData* cryptodata = this->cryptodata_from_byte_arr(cipher_data, cipher_data_l);

            Serial.println("------ Decryption ------");
            print_ascon(cryptodata);
            Serial.println("------------------------");
            
            Plaintext* plaintext = decrypt_auth(this->key, cryptodata, authdata, authdata_l);

            if (plaintext == nullptr) {
                free_crypto(cryptodata);
                ByteArray null;
                null.data = nullptr;
                null.length = 0;
                return null;
            }

            ByteArray byte_array = this->plaintext_to_byte_arr(plaintext);

            free_crypto(cryptodata);
            free_plaintext(plaintext);

            return byte_array;
        }
};

#endif
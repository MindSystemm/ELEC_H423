#ifndef CLASSES_H
#define CLASSES_H

#include "Ascon.h"
#include <tuple>

uint64_t py_arr_size(py::array_t<uint8_t> arr);
uint8_t* py_arr_to_uint8_t(py::array_t<uint8_t> arr);
py::array_t<uint8_t> uint8_t_arr_to_py(uint8_t* arr, uint64_t l);

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
        Ascon(py::array_t<uint8_t> arr) {
            uint8_t* key = py_arr_to_uint8_t(arr);
            init_nonce();
            memcpy(this->key, key, KEY_LENGTH);
        }

        py::array_t<uint8_t> encrypt(py::array_t<uint8_t> plaintext_py, py::array_t<uint8_t> authdata_py) {
            uint8_t* plaintext = py_arr_to_uint8_t(plaintext_py);
            uint8_t* authdata = py_arr_to_uint8_t(authdata_py);
            uint64_t plaintext_l = py_arr_size(plaintext_py);
            uint64_t authdata_l = py_arr_size(authdata_py);

            CryptoData* cryptodata = encrypt_auth(this->key, plaintext, plaintext_l, authdata, authdata_l);

            printf("------ Encryption ------\n");
            print_ascon(cryptodata);
            printf("------------------------\n");

            if (cryptodata == nullptr) {
                free(plaintext);
                free(authdata);

                py::array_t<uint8_t> null_arr = py::array_t<uint8_t>({0});
                return null_arr;
            }

            ByteArray byte_array_tmp = this->cryptodata_to_byte_arr(cryptodata);

            py::array_t<uint8_t> byte_array = uint8_t_arr_to_py(byte_array_tmp.data, byte_array_tmp.length);

            free_crypto(cryptodata);
            free(byte_array_tmp.data);
            free(plaintext);
            free(authdata);

            return byte_array;
        }

        py::array_t<uint8_t> decrypt(py::array_t<uint8_t> cipher_data_py, py::array_t<uint8_t> authdata_py) {
            uint64_t cipher_data_l = py_arr_size(cipher_data_py);
            uint64_t authdata_l = py_arr_size(authdata_py);
            if (cipher_data_l < sizeof(CryptoData) - sizeof(uint8_t*)) {
                // Not enough data
                py::array_t<uint8_t> null_arr = py::array_t<uint8_t>({0});
                return null_arr;
            }
            
            uint8_t* cipher_data = py_arr_to_uint8_t(cipher_data_py);
            uint8_t* authdata = py_arr_to_uint8_t(authdata_py);

            CryptoData* cryptodata = this->cryptodata_from_byte_arr(cipher_data, cipher_data_l);

            printf("------ Decryption ------\n");
            print_ascon(cryptodata);
            printf("------------------------\n");

            Plaintext* plaintext = decrypt_auth(this->key, cryptodata, authdata, authdata_l);

            if (plaintext == nullptr) {
                free_crypto(cryptodata);
                free(cipher_data);
                free(authdata);
                py::array_t<uint8_t> null_arr = py::array_t<uint8_t>({0});
                return null_arr;
            }

            py::array_t<uint8_t> byte_array = uint8_t_arr_to_py(plaintext->plaintext, plaintext->plaintext_l);

            free_crypto(cryptodata);
            free_plaintext(plaintext);
            free(cipher_data);
            free(authdata);

            return byte_array;
        }
};

#endif
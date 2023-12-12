#ifndef ASCON_H
#define ASCON_H

#define _CRT_RAND_S

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "./Crypto/Crypto.h"
#include "./Crypto/CryptoLW.h"
#include "./Crypto/Ascon128.h"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

#define KEY_LENGTH 16
#define IV_LENGTH 16
#define TAG_LENGTH 16

#define NONCE_ADDR 0
#define NONCE_MARGIN 10

typedef struct {
  uint8_t iv[IV_LENGTH];      // Initialization vector
  uint8_t tag[TAG_LENGTH];     // Authentication tag
  uint64_t cipher_l;
} CryptoHeader;

typedef struct {
  CryptoHeader header;
  uint8_t* cipher;     // Data that was authenticated and encrypted
} CryptoData;

typedef struct {
    uint64_t plaintext_l;
    uint8_t* plaintext;
} Plaintext;

void init_nonce();

char* byte_arr_to_str(uint8_t* arr, uint64_t l);

void free_crypto(CryptoData* d);
void free_plaintext(Plaintext* p);

void print_ascon(CryptoData* d);
void print_ascon(Plaintext* p);

CryptoData* encrypt_auth(uint8_t key[KEY_LENGTH], uint8_t* plaintext, uint64_t plaintext_l, uint8_t* authdata, uint64_t authdata_l);

Plaintext* decrypt_auth(uint8_t key[KEY_LENGTH], CryptoData* cryptoData, uint8_t* authdata, uint64_t authdata_l);

#endif
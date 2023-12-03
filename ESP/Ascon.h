#ifndef ASCON_H
#define ASCON_H

#include <cstdlib>
#include "Config.h"
#include "esp_random.h"
#include <stdio.h>
#include <string.h>

#include <Crypto.h>
#include <CryptoLW.h>
#include <Ascon128.h>

#include <Arduino.h>
#include <EEPROM.h>

#define KEY_LENGTH 16
#define IV_LENGTH 16
#define TAG_LENGTH 16

#define NONCE_ADDR 0
#define NONCE_MARGIN 10

#define UINT64_MAX 0xFFFFFFFFFFFFFFFF

typedef struct {
  uint8_t iv[16];      // Initialization vector
  uint8_t tag[16];     // Authentication tag
  uint8_t cipher_l;
} CryptoHeader;

typedef struct {
  CryptoHeader header;
  uint8_t* cipher;     // Data that was authenticated and encrypted
} CryptoData;

typedef struct {
    uint8_t plaintext_l;
    uint8_t* plaintext;
} Plaintext;

void init_nonce();

void free_crypto(CryptoData* d);
void free_plaintext(Plaintext* p);

void print_ascon(CryptoData* d);
void print_ascon(Plaintext* p);

CryptoData* encrypt_auth(uint8_t* plaintext, uint8_t plaintext_l, uint8_t* authdata, uint8_t authdata_l);

Plaintext* decrypt_auth(CryptoData* cryptoData, uint8_t* authdata, uint8_t authdata_l);

#endif
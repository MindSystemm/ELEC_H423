#include "Tests.h"

// A debug function
void test() {

  init_nonce();

  char plaintext_str[] = "This should be encrypted!";
  char authdata_str[] = "This should be authenticated!";

  uint8_t key[KEY_LENGTH] = CRYPTO_KEY;

  // #########################
  // ###### Should work ######
  // #########################
  Serial.println("====== Example that should work ======");
  CryptoData* cryptodata = encrypt_auth(key, (uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));

  if (cryptodata == nullptr) {
    Serial.println("cryptodata was null");
    return;
  }

  print_ascon(cryptodata);
  Plaintext* plaintext = decrypt_auth(key, cryptodata, (uint8_t*) authdata_str, sizeof(authdata_str));
  
  if (plaintext == nullptr) {
    Serial.println("plaintext was null");
    free_crypto(cryptodata);
    return;
  }

  print_ascon(plaintext);

  char plaintext_recovered[plaintext->plaintext_l + 1];
  strncpy(plaintext_recovered, (char*) plaintext->plaintext, plaintext->plaintext_l);
  Serial.printf("plaintext: %s\n", plaintext_recovered);

  // Don't free cryptodata => used by next test
  free_plaintext(plaintext);

  Serial.println();

  // #####################################
  // ###### Shouldn't work (replay) ######
  // #####################################
  Serial.printf("====== Example that shouldn't work (replay: margin = %d) ======\n", NONCE_MARGIN);

  CryptoData* cryptodata_old = encrypt_auth(key, (uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));
  if (cryptodata_old == nullptr) {
      Serial.println("cryptodata_old was null");
  }
  print_ascon(cryptodata_old);

  for (uint64_t i = 0; i < NONCE_MARGIN + 1; i++) {
    // Encrypting increases nonce
    cryptodata = encrypt_auth(key, (uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));

    // Decrypting checks nonce
    plaintext = decrypt_auth(key, cryptodata_old, (uint8_t*) authdata_str, sizeof(authdata_str));

    if (i < NONCE_MARGIN) {
      // Should not be null
      if (plaintext == nullptr) {
        Serial.println("plaintext was null (before margin exit)");
      } else {
        Serial.println("plaintext was not null");
        free_plaintext(plaintext);
      }

      if (cryptodata == nullptr) {
        Serial.println("cryptodata was null (before margin exit)");
      } else {
        free_crypto(cryptodata);
      }
    }
  }
  
  if (plaintext == nullptr) {
    Serial.println("plaintext was null");
    free_crypto(cryptodata);
    free_crypto(cryptodata_old);
  } else {
    print_ascon(plaintext);

    char plaintext_recovered2[plaintext->plaintext_l + 1];
    strncpy(plaintext_recovered2, (char*) plaintext->plaintext, plaintext->plaintext_l);
    Serial.printf("plaintext: %s\n", plaintext_recovered2);

    free_crypto(cryptodata);
    free_crypto(cryptodata_old);
    free_plaintext(plaintext);
  }

  Serial.println();

  // ################################################
  // ###### Shouldn't work (auth data altered) ######
  // ################################################
  Serial.println("====== Example that shouldn't work (authenticated data altered) ======");

  char authdata_different_str[] = "This should not be authenticated!";

  cryptodata = encrypt_auth(key, (uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));
  
  if (cryptodata == nullptr) {
    Serial.println("cryptodata was null");
    return;
  }
  
  print_ascon(cryptodata);

  plaintext = decrypt_auth(key, cryptodata, (uint8_t*) authdata_different_str, sizeof(authdata_different_str));
  
  if (plaintext == nullptr) {
    Serial.println("plaintext was null");
    free_crypto(cryptodata);
  } else {
    print_ascon(plaintext);

    char plaintext_recovered3[plaintext->plaintext_l + 1];
    strncpy(plaintext_recovered3, (char*) plaintext->plaintext, plaintext->plaintext_l);
    Serial.printf("plaintext: %s\n", plaintext_recovered3);

    free_crypto(cryptodata);
    free_plaintext(plaintext);
  }

  Serial.println();

  // #############################################
  // ###### Shouldn't work (cipher altered) ######
  // #############################################
  Serial.println("====== Example that shouldn't work (cipher altered) ======");

  cryptodata = encrypt_auth(key, (uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));
  
  if (cryptodata == nullptr) {
    Serial.println("cryptodata was null");
    return;
  }
  
  print_ascon(cryptodata);

  // Alter cipher
  cryptodata->cipher[0]++;

  plaintext = decrypt_auth(key, cryptodata, (uint8_t*) authdata_str, sizeof(authdata_str));
  
  if (plaintext == nullptr) {
    Serial.println("plaintext was null");
    free_crypto(cryptodata);
  } else {
    print_ascon(plaintext);

    char plaintext_recovered4[plaintext->plaintext_l + 1];
    strncpy(plaintext_recovered4, (char*) plaintext->plaintext, plaintext->plaintext_l);
    Serial.printf("plaintext: %s\n", plaintext_recovered4);

    free_crypto(cryptodata);
    free_plaintext(plaintext);
  }
}
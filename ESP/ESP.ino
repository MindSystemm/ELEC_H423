#include "Config.h"
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Ascon.h"

DHT dht(26, DHT11);

#define BUTTON_PIN  27
#define LED_PIN_RED 32
#define LED_PIN_WHITE 33
#define BTN_TRIGGER_TIME 3000 // 3 seconds

WiFiClient espClient;
PubSubClient client(espClient);

bool paused = false;


// A debug function
void test() {

  char plaintext_str[] = "This should be encrypted!";
  char authdata_str[] = "This should be authenticated!";

  // #########################
  // ###### Should work ######
  // #########################
  Serial.println("====== Example that should work ======");
  CryptoData* cryptodata = encrypt_auth((uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));

  if (cryptodata == nullptr) {
    Serial.println("cryptodata was null");
    return;
  }

  print_ascon(cryptodata);
  Plaintext* plaintext = decrypt_auth(cryptodata, (uint8_t*) authdata_str, sizeof(authdata_str));
  
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

  CryptoData* cryptodata_old = encrypt_auth((uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));
  if (cryptodata_old == nullptr) {
      Serial.println("cryptodata_old was null");
  }
  print_ascon(cryptodata_old);

  for (size_t i = 0; i < NONCE_MARGIN + 1; i++) {
    // Encrypting increases nonce
    cryptodata = encrypt_auth((uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));

    // Decrypting checks nonce
    plaintext = decrypt_auth(cryptodata_old, (uint8_t*) authdata_str, sizeof(authdata_str));

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

  cryptodata = encrypt_auth((uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));
  
  if (cryptodata == nullptr) {
    Serial.println("cryptodata was null");
    return;
  }
  
  print_ascon(cryptodata);

  plaintext = decrypt_auth(cryptodata, (uint8_t*) authdata_different_str, sizeof(authdata_different_str));
  
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

  cryptodata = encrypt_auth((uint8_t*) plaintext_str, sizeof(plaintext_str), (uint8_t*) authdata_str, sizeof(authdata_str));
  
  if (cryptodata == nullptr) {
    Serial.println("cryptodata was null");
    return;
  }
  
  print_ascon(cryptodata);

  // Alter cipher
  cryptodata->cipher[0]++;

  plaintext = decrypt_auth(cryptodata, (uint8_t*) authdata_str, sizeof(authdata_str));
  
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

void setup_wifi(){
  WiFi.begin(SSID_ESP, PASSWD_ESP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void handle_pause(byte* payload) {
  // Set global var to true if payload is not zero
  paused = payload[0] != 0;
  print_pause_state();
}

void print_pause_state() {
  if (paused) {
    Serial.println("Paused");
  } else {
    Serial.println("Resumed");
  }
}

void pause_all() {
  uint8_t data[] = {1};
  client.publish("pause", data, 1);
}

void resume_all() {
  uint8_t data[] = {0};
  client.publish("pause", data, 1);
}

void callback(char *topic, byte *payload, unsigned int length) {
  char payload_str[length+1];
  memcpy(payload_str, payload, sizeof(payload_str));
  payload_str[length] = '\0';

  Serial.println("-----------------------");
  Serial.printf("Message arrived in topic: %s\n", topic);
  Serial.printf("Message: %s (", payload_str);
  for (unsigned int i = 0; i < length; i++) {
    Serial.printf(" 0x%02x", payload[i]);
  }
  Serial.printf(" )\n");
  Serial.println("-----------------------");

  if (strcmp(topic, "pause") == 0 and length > 0) {
    handle_pause(payload);
  }
}

void setup_broker(){
  // Connect to the NQTT broker
  Serial.print("Connecting to: ");
  Serial.print(MQTT_ADDRESS_ESP);
  Serial.print(" with port ");
  Serial.println(MQTT_PORT_ESP);


  client.setServer(MQTT_ADDRESS_ESP, MQTT_PORT_ESP);
  client.setCallback(callback);
  while (!client.connected()) {
      String client_id = String("ESP32-") + String(WiFi.macAddress());
      Serial.printf("Connecting to MQTT broker (%s:%d) with ID %s\n", MQTT_ADDRESS_ESP, MQTT_PORT_ESP, client_id.c_str());
      if (client.connect(client_id.c_str())) {
          Serial.println(" > connected to broker");
      } else {
          Serial.print(" > failed with state ");
          Serial.println(client.state());
          delay(2000);
      }
  }
}

void publish(char* topic, float val) {
  String stringified_data = String("{ \"id\": \"") + ID_ESP + "\", \"val\": " + String(val) + " }";

  int data_length = stringified_data.length() + 1;
  char stringified_data_arr[data_length];
  stringified_data.toCharArray(stringified_data_arr, data_length);

  client.publish(topic, stringified_data_arr);
}

int btn_begin = 0;
int btn_press_time = 0;
int btn_pressed = false;
bool update_btn() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    if (!btn_pressed) {
      btn_begin = millis();
      btn_pressed = true;
    }
    btn_press_time = millis() - btn_begin;

    return true;
  } else if (btn_pressed) {
    // Just released => still keep time
    btn_press_time = millis() - btn_begin;
    btn_begin = 0;
    btn_pressed = false;
    return false;
  } else {
    // Released => reset
    btn_press_time = 0;
    return false;
  }
}

void setup() {
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_WHITE, OUTPUT);
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_WHITE, LOW);

  pinMode(BUTTON_PIN, INPUT);
  
  dht.begin();
  Serial.begin(115200);

  EEPROM.begin(8); // 8 bytes: 1 uint8_t

  init_nonce();

  Serial.println("################# Test #################");
  test();
  Serial.println("########################################");

  setup_wifi();
  setup_broker();

  client.subscribe("pause");
}

int last_update = millis();
bool triggered_paused_update = false; // Used to detect button release between pause state updates
void loop() {
  bool btn_active = update_btn();

  if (btn_active) {
    digitalWrite(LED_PIN_RED, HIGH);

    if (btn_press_time >= BTN_TRIGGER_TIME && !triggered_paused_update) {
      // Button has been pushed for at least 3 seconds
      triggered_paused_update = true;

      if (paused) {
        resume_all();
      } else {
        pause_all();
      }   
    }
  } else {
    // Btn released 
    digitalWrite(LED_PIN_RED, LOW);

    // So we have to release the button before a new pause state update can be triggered
    triggered_paused_update = false;

    if (btn_press_time > 0 && btn_press_time < BTN_TRIGGER_TIME) {
      // Button was pressed shortly
      // => Cycle paused state
      paused = !paused;
      print_pause_state();
    }
  }

  if (paused) {
    digitalWrite(LED_PIN_WHITE, HIGH);
  } else {
    digitalWrite(LED_PIN_WHITE, LOW);
  }

  if (millis() - last_update >= 1000) {
    // Update every second
    // Reconnect to wifi if disconnected
    if (WiFi.status() != WL_CONNECTED){
      setup_wifi();
    }

    // Reconnect to MQTT broker if disconnected
    if (!client.connected()) {
      setup_broker();
    }

    if (!paused) {
      float temp = dht.readTemperature();
      float humidity = dht.readHumidity();

      publish("temperature", temp);
      publish("humidity", humidity);

      digitalWrite(LED_PIN_WHITE, HIGH);
    }

    client.loop();

    last_update = millis();
  }

  delay(100);
}

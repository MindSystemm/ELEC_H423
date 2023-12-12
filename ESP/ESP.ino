#include "Config.h"
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Classes.h"
#include "Tests.h"
#include <cstring>

DHT dht(26, DHT11);

#define BUTTON_PIN  27
#define LED_PIN_RED 32
#define LED_PIN_WHITE 33
#define BTN_TRIGGER_TIME 3000 // 3 seconds

#define NONCE_SYNC_INT 9 // Nonce margin in Ascon.h is 10 => use 9 here to play safe

WiFiClient espClient;
PubSubClient client(espClient);
uint8_t key[KEY_LENGTH] = CRYPTO_KEY;
Ascon crypto(key);

bool paused = false;

void setup_wifi(){
  WiFi.begin(SSID_ESP, PASSWD_ESP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void publish_encrypted(char* topic, uint8_t* data, uint64_t length) {
  // Encrypt
  ByteArray byte_array = crypto.encrypt(data, length, (uint8_t*) topic, std::strlen(topic));

  client.publish(topic, byte_array.data, byte_array.length);

  // Cleanup
  free(byte_array.data);
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
  publish_encrypted("pause", data, 1);
}

void resume_all() {
  uint8_t data[] = {0};
  publish_encrypted("pause", data, 1);
}

void callback(char *topic, byte *payload_enc, unsigned int length) {
  // Decrypt and verify
  ByteArray byte_array = crypto.decrypt(payload_enc, length, (uint8_t*) topic, std::strlen(topic));

  if (byte_array.data == nullptr || byte_array.length == 0) {
    // Decryption unsuccessful (wrong key, message altered, replayed)
     Serial.println("Message invalid (decryption failed)");
    return;
  }
  
  char payload_str[byte_array.length+1];
  memcpy(payload_str, byte_array.data, sizeof(payload_str));
  payload_str[byte_array.length] = '\0';

  Serial.println("-----------------------");
  Serial.printf("Message arrived in topic: %s\n", topic);
  Serial.printf("Message: %s (", payload_str);
  for (unsigned int i = 0; i < byte_array.length; i++) {
    Serial.printf(" 0x%02x", byte_array.data[i]);
  }
  Serial.printf(" )\n");
  Serial.println("-----------------------");

  if (strcmp(topic, "pause") == 0 and byte_array.length > 0) {
    handle_pause(byte_array.data);
  } else if (strcmp(topic, "nonce") == 0) {
    Serial.println("Nonce synced");
  }

  // Cleanup
  free(byte_array.data);
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

  publish_encrypted(topic, (uint8_t*) stringified_data_arr, data_length);
}

void publish_nonce() {
  // Just publishes and empty encrypted message to sync nonce
  uint8_t data[1] = {0};
  publish_encrypted("nonce", data, 1);
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

  EEPROM.begin(8); // 8 bytes: 1 uint64_t

  Serial.println("################# Test #################");
  test();
  Serial.println("########################################");

  setup_wifi();
  setup_broker();

  client.subscribe("pause");
  client.subscribe("nonce");
}

int last_update = millis();
bool triggered_paused_update = false; // Used to detect button release between pause state updates
size_t loops_until_nonce_update = NONCE_SYNC_INT;
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

      loops_until_nonce_update--;

      if (loops_until_nonce_update == 0) {
        publish_nonce();
        loops_until_nonce_update = NONCE_SYNC_INT;
      }


      digitalWrite(LED_PIN_WHITE, HIGH);
    }

    client.loop();

    last_update = millis();
  }

  delay(100);
}

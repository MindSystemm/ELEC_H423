#include "Config.h"
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>


DHT dht(26, DHT11);

#define BUTTON_PIN  27
#define LED_PIN 32

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi(){
  WiFi.begin(SSID_ESP, PASSWD_ESP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void display_message(char *message) {
  Serial.printf("Display: \"%s\"\n", message);
}

void callback(char *topic, byte *payload, unsigned int length) {
  char payload_str[length+1];
  memcpy(payload_str, payload, sizeof(payload_str));
  payload_str[length] = '\0';

  Serial.println("-----------------------");
  Serial.printf("Message arrived in topic: %s\n", topic);
  Serial.printf("Message: %s\n", payload_str);
  Serial.println("-----------------------");

  if (strcmp(topic, "display") == 0) {
    display_message(payload_str);
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

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  
  dht.begin();
  delay(2000);
  Serial.begin(115200);

  setup_wifi();
  setup_broker();

  client.subscribe("display");
}

int last_update = millis();

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED is ON");
  } else {
    digitalWrite(LED_PIN, LOW);
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

    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    publish("temperature", temp);
    publish("humidity", humidity);
    Serial.println("Sent data successfully");

    client.loop();

    last_update = millis();
  }

  delay(100);
}

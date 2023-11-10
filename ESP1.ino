#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

DHT dht(26, DHT11);
WiFiClient espClient;              
PubSubClient client(espClient);  

#define BUTTON_PIN  27
#define LED_PIN 32

const char *ssid = "Willwolf";
const char *password = "12345678";
const char *mqtt_server = "192.168.113.107";
const int mqtt_port = 1883; //Default port of Mosquitto 
const char *clientID = "ESP32_1";


void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(LED_PIN, LOW);
  
  dht.begin();
  delay(2000);
  Serial.begin(9600); //baud
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED is ON");
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  Serial.print("Temp : ");
  Serial.print(temp);
  Serial.println(" °C");
  Serial.print("Humidity : ");
  Serial.print(humidity);
  Serial.println(" %");
  client.publish("humidityData", String(humidity).c_str());
  client.publish("temperatureData", String(temp).c_str());
  delay(2000);
}

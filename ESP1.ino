#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

DHT dht(26, DHT11);
WiFiClient espClient;              
PubSubClient client(espClient);  

#define BUTTON_PIN  27
#define LED_PIN 32

//Network credentials
const char *ssid = "MyWIFI";
const char *password = "EasyPasa"; 

//MQTT credentials
const char *mqtt_server = "192.168.88.88"; //Own IP address
const int mqtt_port = 12000; //Default port of Mosquitto 
const char *clientID = "ESP32_1";

//Topic structure to publich to MQTT broker
const char *temperature_topic = "sens ors/esp32_1/temperature";
const char *humidity_topic = "sensors/esp32_1/humidity";

void setup_wifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void setup_server(){
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


void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  
  dht.begin();
  delay(2000);
  Serial.begin(115200); //baud

  setup_wifi();
  setup_server()
}

void 

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED is ON");
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  if (Wifi.status() != WL_CONNECTED){
    delay(1000);
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); //Maintenance for MQTT connection.
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  client.publish(temperature_topic, String(temperature).c_str());
  client.publish(humidity_topic, String(humidity).c_str());
  delay(5000);
}

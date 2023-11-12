#include <DHT.h>
#include <WiFi.h>
#include <ThingsBoard.h>

DHT dht(26, DHT11);
WiFiClient espClient;      
ThingsBoard tb(espClient);

#define BUTTON_PIN  27
#define LED_PIN 32
#define ACCESS_TOKEN  "MY_ACCESS_TOKEN"

//Network credentials
const char *ssid = "MyWIFI";
const char *password = "EasyPasa"; 

//MQTT credentials
const char *mqtt_server = "mqtt.thingsboard.cloud";
const int mqtt_port = 1883;

//Topic structure to publich to MQTT broker
const char *topic = "v1/devices/me/telemetry";


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
  if (!tb.connected()) {;
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(mqtt_server);
    Serial.print(" with token ");
    Serial.println(ACCESS_TOKEN);
    if (!tb.connect(mqtt_server, ACCESS_TOKEN, mqtt_port)) {
      Serial.println("Failed to connect");
      return;
    } else{
      Serial.println("Successfully connected");
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
  setup_server();
}

void loop() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED is ON");
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  if (WiFi.status() != WL_CONNECTED){
    delay(1000);
    setup_wifi();
  }

  // Reconnect to ThingsBoard, if needed
  if (!tb.connected()) {
    delay(1000);
    setup_server();
  }


  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  tb.sendTelemetryData("temperature", temp);
  tb.sendTelemetryData("humidity", humidity);
  Serial.println("Sent data successfully");

  tb.loop();

  delay(5000);
}

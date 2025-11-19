/* ASSIGNMENT 3 - Advanced Networking
(Modified from Assignment 2)

Group 32:
Fabiano de Sá Filho
Felipe Noleto
João Antônio Astolfi
*/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncMqttClient.h>
#include <avdweb_Switch.h>
#include <Adafruit_BME280.h>

// WIFI
#define WIFI_SSID "ADN-IOT"
#define WIFI_PASSWORD "WBNuyawB2a"
#define ADNGROUP "adn-group32"

// MQTT
#define MQTT_HOST IPAddress(192, 168, 0, 1) // Broker IP from ass03.pdf Task 1.2
#define MQTT_PORT 1883

// GLOBAL VARS
AsyncMqttClient mqttClient;
Adafruit_BME280 bme;

// FUNCTIONS
void connect_to_wifi();
void setup_mqtt();
void slow_blink();
void fast_blink();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total);

void setup() {
  Serial.begin(115200);

  // LED setup
  pinMode(LED_BUILTIN, OUTPUT);

  connect_to_wifi();

  // SEnsor
  bme.begin(0x76);

  setup_mqtt();

  Serial.println("\nConnected to MQTT broker.");
  digitalWrite(LED_BUILTIN, HIGH); // Turn on LED when WiFi and MQTT are ready
}



// MAIN
void loop() {

  // Reads
  float temp = bme.readTemperature();
  float p = bme.readPressure();
  float hum = bme.readHumidity();

  Serial.printf("Sending Temp to broker: %.2f \n", temp);

  // Converts float to char
  char buffer[4];
  memcpy(&buffer, &temp, 4);

  // Sends
  mqttClient.publish("adn/group32/temp", 0, false, buffer);

  // Waits 5 seconds to read again
  delay(5000);
}




void onMqttConnect(bool sessionPresent) {
  Serial.println("MQTT Connected.");
  digitalWrite(LED_BUILTIN, HIGH);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("MQTT Disconnected. Reason: %d\n", (int)reason);
  digitalWrite(LED_BUILTIN, LOW);
  // Try to reconnect
  mqttClient.connect();
}

//MQTT Message Received Callback
void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total) {

  //log incoming messages for debugging
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Create a null-terminated string from the payload
  char msg[len + 1];
  strncpy(msg, payload, len);
  msg[len] = '\0';
  Serial.println(msg);
}

void connect_to_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(ADNGROUP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Timeout vars
  unsigned long start_time = millis();
  const unsigned long timeout = 10000; // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - start_time < timeout) {
    delay(1000);
    slow_blink();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n Connected to WIFI \n");
    // Removed LED HIGH here, will set it after MQTT connects
  } else {
    Serial.printf("Could not connect");
    while (1) {
      fast_blink();
    }
  }

  // If connection successful, prints IP:
  Serial.printf("\n IP Address: %s\n", WiFi.localIP().toString().c_str());
}

void setup_mqtt() {
  // MQTT setup (from slides)
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setClientId(ADNGROUP);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  Serial.println("Connecting to MQTT broker...");
  mqttClient.connect();

  // Wait for MQTT connection (implemented similar to wifi function)
  unsigned long start_time = millis();
  const unsigned long timeout = 10000; // 10 seconds
  while (!mqttClient.connected() && millis() - start_time < timeout) {
    Serial.print(".");
    delay(500);
  }

  if (!mqttClient.connected()) {
    Serial.println("\nCould not connect to MQTT broker!");
    while (1) {
      fast_blink();
    }
  }
}

void slow_blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}

void fast_blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
}

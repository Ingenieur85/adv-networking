/* ASSIGNMENT 6 - Advanced Networking
(Modified from Assignment 5)

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
#include <HardwareSerial.h>
#include <TinyGPS++.h>

// WIFI
#define WIFI_SSID "ADN-IOT"
#define WIFI_PASSWORD "WBNuyawB2a"
#define ADNGROUP "adn-group32"
#define MQTT_TOPIC "adn/group32/gps"

// MQTT
#define MQTT_HOST IPAddress(192, 168, 0, 1) // Broker IP from ass03.pdf Task 1.2
#define MQTT_PORT 1883

// GLOBAL VARS
AsyncMqttClient mqttClient;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

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

  // Gps Setup
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  delay(1000);


  connect_to_wifi();
  setup_mqtt();

  Serial.println("\nConnected to MQTT broker.");
  digitalWrite(LED_BUILTIN, HIGH); // Turn on LED when WiFi and MQTT are ready
}



// MAIN
void loop() {

  // Reads
  while(gpsSerial.available() > 0) gps.encode(gpsSerial.read());

  double lat = gps.location.lat();
  double lng = gps.location.lng();
  double alt = gps.altitude.meters();
  uint32_t hdop_raw = gps.hdop.value();
  double hdop_display = (double)hdop_raw / 100.0;

  uint32_t num_satellites = gps.satellites.value();
  uint32_t age = gps.location.age();
  uint32_t failed_checksums = gps.failedChecksum();
  uint32_t processed_chars = gps.charsProcessed();

  Serial.printf("--- GPS Status ---\n");
  Serial.printf("Latitude: %.6f\n", lat);
  Serial.printf("Longitude: %.6f\n", lng);
  Serial.printf("Altitude: %.2f m\n", alt);
  Serial.printf("HDOP: %.3f\n", hdop_display);
  
  // Print debug information as required by the prompt
  Serial.printf("Satellites: %u\n", num_satellites);
  Serial.printf("Age: %u ms\n", age);
  Serial.printf("Failed Checksums: %u\n", failed_checksums);
  Serial.printf("Processed Chars: %u\n", processed_chars);
  Serial.printf("------------------\n");


  // Builds payload
  if (gps.location.isValid()) {
    Serial.printf("Sending GPS MQTT: Lat: %.6f, Lng: %.6f, Alt: %.2f, hdop_raw: %u \n", lat, lng, alt, hdop_raw);

    char buffer[32];
    memcpy(&buffer[0], &lat, sizeof(double));
    memcpy(&buffer[8], &lng, sizeof(double));
    memcpy(&buffer[16], &alt, sizeof(double));
    memcpy(&buffer[24], &hdop_display, sizeof(double));
    // Sends
    mqttClient.publish(MQTT_TOPIC, 0, false, buffer, 32);
  }
  else {
    Serial.println("No Fix yet. Skipping MQTT publish.");
  }

  // 3. Waits 5 seconds to read again (Blocking Delay)
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

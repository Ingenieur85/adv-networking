/* 
ASSIGNMENT 2 - Advanced Networking

Group 32:
Fabiano de Sá Filho
Felipe Noleto
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define WIFI_SSID "ADN-IOT"
#define WIFI_PASSWORD "WBNuyawB2a"
#define ADNGROUP "adn-groupXY"

// Global Vars
HTTPClient http;

void connect_to_wifi();
void slow_blink();
void fast_blink(); 


void setup() {
  Serial.begin(115200);

  //LED setup
  pinMode(LED_BUILTIN, OUTPUT);

  //wiFi setup
  connect_to_wifi();
  
  // If connection succeful, prints IP:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

}


void loop() {
  while(1) {
    //do nothing
  }
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
    //WiFi.localIP()
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n Connected to WIFI \n");
    digitalWrite(LED_BUILTIN, HIGH);
  } 
  else {
    Serial.print("Could not connect");
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

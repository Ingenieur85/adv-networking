/* 
ASSIGNMENT 2 - Advanced Networking

Group 32:
Fabiano de Sá Filho
Felipe Noleto
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>


// WIFI
#define WIFI_SSID "ADN-IOT"
#define WIFI_PASSWORD "WBNuyawB2a"
#define ADNGROUP "adn-groupXY"

// HTTP
#define HTTP_BASE_URL "http://192.168.0.55/cm?cmnd=Power%20"
#define HTTP_CMD_ON "ON"
#define HTTP_CMD_OFF "OFF"


// HYSTERESIS
// Light turns ON when lux drops BELOW this value
#define LUX_LOW_THRESH 100
// Light turns OFF when lux goes ABOVE this value
#define LUX_HIGH_THRESH 200 
//How often to check the sensor in millis
#define SENSOR_READ_INTERVAL 2000 


// GLOBAL VARS
HTTPClient http;

// Sensor stuff
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Tracks light state. assume it's OFF at the start
bool is_light_on = false; 

// Time tracking for non-blocking delay
unsigned long last_check_time = 0;


// FUNCTION PROTOTYPES
void connect_to_wifi();
void slow_blink();
void fast_blink();
void configure_sensor();
void send_http_command(const char* command);


void setup() {
  Serial.begin(115200);

  //LED setup
  pinMode(LED_BUILTIN, OUTPUT);

  //wiFi setup
  connect_to_wifi();
  
  // If connection successful, prints IP:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // sensore setup
  Serial.println("Initializing TSL2561 light sensor...");
  configure_sensor();
}


void loop() {
  // implemented a non-blocking delay
  if (millis() - last_check_time > SENSOR_READ_INTERVAL) {
    
    // Get a new sensor event
    sensors_event_t event;
    tsl.getEvent(&event);

    if (event.light) {
      float current_lux = event.light;
      Serial.printf("Current Lux reading: %f\n", current_lux);

      // --- HYSTERESIS LOGIC ---

      if (current_lux < LUX_LOW_THRESH && !is_light_on) {
        Serial.println("turning light ON");
        send_http_command(HTTP_CMD_ON);
        is_light_on = true;
      }

      else if (current_lux > LUX_HIGH_THRESH && is_light_on) {
        Serial.println("turning light OFF");
        send_http_command(HTTP_CMD_OFF);
        is_light_on = false;
      }
      // do nothing more (light level is between thresholds or state already matches)
      
    } else {
      // If event.light is 0, could be a sensor read error
      // Handling by assuming reading is correct and turning light on
      Serial.println("Sensor read is 0");
      if (!is_light_on) {
        Serial.println("turning light ON");
        send_http_command(HTTP_CMD_ON);
        is_light_on = true;
      }
    }

    // Update the last check time for delay logic
    last_check_time = millis();
  }
}

// I modified the http toggle logic for a send command function for future use
void send_http_command(const char* command) {

  String url = String(HTTP_BASE_URL) + String(command);

  Serial.printf("Sending HTTP command: %s\n", command);
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.printf("HTTP response code: %d\n", httpCode);
  } else {
    Serial.printf("Error in HTTP request: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// followed the slides here to setup TSL
void configure_sensor() {
  if (!tsl.begin()) {

    Serial.print("Could not find sensor!");
    while (1) { fast_blink(); } // Halt with fast blink
  }

  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  Serial.println("Light sensor configured.");
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

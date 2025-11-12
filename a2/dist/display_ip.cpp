/* 
ASSIGNMENT 2 - Advanced Networking

Group 32:
Fabiano de Sá Filho
Felipe Noleto
João Antônio Astolfi
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WIFI
#define WIFI_SSID "ADN-IOT"
#define WIFI_PASSWORD "WBNuyawB2a"
#define ADNGROUP "adn-groupXY"


#define OLED_RESET 0 //GPIO0
Adafruit_SSD1306 display(OLED_RESET);


// FUNCTION PROTOTYPES
void connect_to_wifi();
void slow_blink();
void fast_blink();

void setup() {
  Serial.begin(115200);

  // LED setup
  pinMode(LED_BUILTIN, OUTPUT);

  // Connect to WiFi already prints IP to the screen.
  connect_to_wifi();

  // Actual OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //i2c address
  display.display(); 
  display.clearDisplay(); //clear the buffer (splashscreen)

  display.setTextSize(1); 
  display.setTextColor(WHITE); 
  display.setCursor(0,0);
  display.println(WiFi.localIP()); // Modified here from slides
  display.display(); 
  delay(2000); 
  display.clearDisplay();
}

void loop() {
  // Nothing to do in loop
}


void connect_to_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(ADNGROUP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start_time = millis();
  const unsigned long timeout = 10000; // 10 seconds

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < timeout) {
    delay(500);
    Serial.print(".");
    slow_blink();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Serial.println("WiFi connection failed!");
    while (1) fast_blink();
  }
}


void slow_blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  digitalWrite(LED_BUILTIN, LOW);
  delay(300);
}

void fast_blink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(150);
  digitalWrite(LED_BUILTIN, LOW);
  delay(150);
}

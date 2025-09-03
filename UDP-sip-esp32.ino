#include <WiFi.h>
#include <AsyncUDP.h>
#include <Ticker.h>
#include <MD5Builder.h>

#include "classe_sip.h"

const char* ssid = "Ligga Gabriel 2.4g";
const char* pass = "09876543";

SIPClient sip("esp32dev", "aaa", "192.168.18.10", 5060, 5060, 4000);

const int LED_PIN = 2;
const int DEBUG = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  sip.begin();
}

void loop() {
  delay(20);
  digitalWrite(LED_PIN, sip.getEmLigacao());
}

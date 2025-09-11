#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "credentials.h"

#define DEBUG

#include "ConfigServer.h"
#include "DataServer.h"


ConfigServer cs;
DataServer ds;

void setup() {
  delay(1000);

#ifdef DEBUG
  Serial.begin(9600);
  Serial.println();
  Serial.print("Configuring access point...");
#endif
  
  WiFi.softAP(SSID, PASS);
  WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(0,0,0,0), IPAddress(255,255,255,0));

#ifdef DEBUG
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
#endif

  cs.init();

#ifdef DEBUG
  Serial.println("HTTP server started");
#endif 

  uint8_t ds_init = ds.init();
  
#ifdef DEBUG
  if (ds_init) {
    Serial.println("UDP connection open at port 69");
  } else {
    Serial.println("Could not open UDP connection at port 69");
  }
#endif
}

void loop() {
  cs.handle_client();
  ds.recv_packet();
  if (ds.is_active()) ds.send_packet();
}
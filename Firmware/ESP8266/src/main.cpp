#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SparkFun_ADS1219.h>

#include "credentials.h"

#define DEBUG

#include "Config.h"
#include "ConfigServer.h"
#include "DataServer.h"

ConfigServer cs;
DataServer ds;

bool act = false;

SfeADS1219ArdI2C adc;

void setup() {
	delay(1000);

#ifdef DEBUG
	Serial.begin(9600);
	Serial.println();
	Serial.print("Configuring access point...");
#endif

	WiFi.softAP(SSID, PASS);
	WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(0, 0, 0, 0), IPAddress(255, 255, 255, 0));

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
	}
	else {
		Serial.println("Could not open UDP connection at port 69");
	}
#endif

	Wire.begin();
	while (adc.begin(0x40) == false) {
#ifdef DEBUG
		Serial.println("ADC failed to begin. Please check your wiring! Retrying...");
#endif
		delay(500);
  	}

}

void loop() {
	cs.handle_client();
	
	if (!act) {
		adc.startSync();
		act = true;
	}

	ds.recv_packet();
	if (ds.is_active() && adc.dataReady()) {
		adc.readConversion();
		uint32_t data = adc.getConversionRaw() & 0x00FFFFFF;
		ds.send_packet(data);
		act = false;
#ifdef DEBUG
		Serial.printf("Sending: %X\n", data);
#endif
	}

}
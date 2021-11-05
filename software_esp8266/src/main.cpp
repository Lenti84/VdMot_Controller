/*
	Changelog :
	5.11.2021 (Surfgargano)
		change this
			if (millis() > (uint32_t) 100 + timer) {
		to 
			if ((millis()- timer) > (uint32_t) 100) {
		to prevent a 42 day delay in case of wrap around
*/

#include <Arduino.h>
#include <WebOTA.h>
#include "mqtt.h"
#include "main.h"
#include "app.h"
#include "telnet.h"
#include "credentials.h"


const char* host     = "ESP-OTA"; // Used for MDNS resolution
const char* MQTT_BROKER = "192.168.4.201";



void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  pinMode(LED_BUILTIN,OUTPUT);

  init_wifi(ssid, password, host);

	// Defaults to 8080 and "/webota"
	webota.init(80, "/update");

  mqtt_setup();

  app_setup();

  telnet_setup();

  Serial.println("Test Server");

}

void loop() {
  // put your main code here, to run repeatedly:

  static uint32_t timer = 0;
  static uint32_t timer10ms = 0;

  // LED / heartbeat 
  if ((millis()-timer) > (uint32_t) 1000) {
        timer = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        //Serial.println("heartbeat");
        //telnet_msg("heartbeat");
  }

	webota.delay(10);
  webota.handle();


  if ((millis()-timer10ms) > (uint32_t) 1000 ) {
      timer10ms = millis();
       
      telnet_loop();
  }

  // MQTT Loop
  mqtt_loop();

  app_loop();

}
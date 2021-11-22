#include "globals.h"
#include "espota.h"
#include <ArduinoOTA.h>



void ESPota_setup(void) {

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      UART_DBG.println("Start updating " + type);
    })
    .onEnd([]() {
      UART_DBG.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      UART_DBG.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      UART_DBG.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) UART_DBG.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) UART_DBG.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) UART_DBG.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) UART_DBG.println("Receive Failed");
      else if (error == OTA_END_ERROR) UART_DBG.println("End Failed");
    });

  ArduinoOTA.begin();

}


void ESPota_loop (void) {

    ArduinoOTA.handle();

}
/*

     Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "globals.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <FS.h>
//#include "stm32.h"
//#include "stm32ota.h"
#include <ESPmDNS.h>
#include "mqtt.h"
#include "app.h"
#include "telnet.h"
#include "web.h"
#include "credentials.h"
#include <WiFiUdp.h>
#include "espota.h"
#include "Logger.h"


//#include <WebServer_WT32_ETH01.h>
//const char* host = "stm32ota";

Logger logger;      // web service logger

uint8_t vismode = VISMODE_ON;    // visualisation mode for debug messages


void setup(void)
{
  UART_DBG.begin(115200);
  if (!SPIFFS.begin(true)) {
    UART_DBG.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  UART_DBG.println("SPIFFS booted");

  //STM32ota_setup();

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  //RunMode();

  for ( int i = 0; i < 3; i++) {
    //digitalWrite(LED, !digitalRead(LED));
    delay(100);
  }

  while(UART_STM32.available()) UART_STM32.read();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    UART_DBG.print(".");
  }
  UART_DBG.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    UART_DBG.println("MDNS responder started");
  }

  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {
    //MDNS.begin(host);
   
    webserver_setup();   
       
    //MDNS.addService("http", "tcp", 80);
  }

  mqtt_setup();
    
  telnet_setup();

  app_setup();

  ESPota_setup();

}


void loop(void) {
  static uint32_t timer10ms = 0;
  static uint32_t timer1000ms = 0;

  webserver_loop();

  // 10 ms task
  if ((millis()-timer10ms) > (uint32_t) 10 ) {
      timer10ms = millis();             
  }

  // 1000 ms task
  if ((millis()-timer1000ms) > (uint32_t) 1000 ) {
      timer1000ms = millis();       
      //logger.println("Logger test");      // logs data to webservice 
      //logger.println("Logger data test", Logger::DATA);  // logs data to webservice 
      telnet_loop();
  }

  mqtt_loop();

  app_loop();

  ESPota_loop();

  yield();
  //delay(1);
}



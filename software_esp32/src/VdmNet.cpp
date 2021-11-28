/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano

  Comments:


*END************************************************************************/

#define USING_CORE_ESP32_CORE_V200_PLUS     false
// Debug Level from 0 to 4
#define _ETHERNET_WEBSERVER_LOGLEVEL_       3

#define ETH_ADDR        1
#define ETH_POWER_PIN   16
#define ETH_POWER_PIN_ALTERNATIVE 16
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO0_IN

#include <stdint.h>
#include <VdmNet.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "globals.h"
#include "credentials.h"
#include "mqtt.h"
#include <WebServer_WT32_ETH01.h>
#include "web.h"
// Web page
#include "tfs_data.c"

WebServer server(80);

CVdmNet VdmNet;

// Select the IP address according to your local network
IPAddress myIP(192, 168, 2, 232);
IPAddress myGW(192, 168, 2, 1);
IPAddress mySN(255, 255, 255, 0);

// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);


#define aj  "application/json"
#define tp  "text/plain"
#define gz  "Content-Encoding : gzip"

// server handles --------------------------------------------------

void handleRoot () {
 // server.send_P(200, "text/html", (const char*) tfs____web_pages_h_index_html , sizeof(tfs____web_pages_h_index_html)); 
 server.send_P(200, gz, (const char*) tfs____web_pages_h_index_html , sizeof(tfs____web_pages_h_index_html)); 
}

void handlePlain () {
  UART_DBG.println(server.arg("plain"));
  server.send(200);
}


void handleNotFound()
{
  String message = F("File Not Found\n\n");
  
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, F(tp), message);
}

void handleValves()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getValveStatus());
  if (server.method() == HTTP_POST) {
    postValvePos();
  }
}


void handleNetInfo () 
{ 
  server.send(200,aj,getNetInfo(ETH,VdmConfig.configFlash.netConfig));
}

void handleNetConfig()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getNetConfig(VdmConfig.configFlash.netConfig));
  if (server.method() == HTTP_POST) {
    
  }
}

void handleProtConfig()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getProtConfig(VdmConfig.configFlash.protConfig));
  if (server.method() == HTTP_POST) {
    
  }
}

void handleSysInfo () 
{ 
  server.send(200,aj,getSysInfo());
}


CVdmNet::CVdmNet()
{
}

void CVdmNet::init()
{
  serverIsStarted = false;
  dataBrokerIsStarted = false;
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

  // Static IP, leave without this line to get IP via DHCP
  //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
  //ETH.config(myIP, myGW, mySN, myDNS);

  WT32_ETH01_onEvent();
}

void CVdmNet::setup() {
  if (VdmConfig.configFlash.netConfig.netConfigFlags.eth_wifi==0) {
      setupEth();
  } else {
      setupWifi();
  }
  if (VdmConfig.configFlash.protConfig.dataProtocol==mqttProtocol) 
  {
    mqtt_setup(VdmConfig.configFlash.protConfig.brokerIp,VdmConfig.configFlash.protConfig.brokerPort);
  }
}

void CVdmNet::setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    UART_DBG.print(".");
  }
  UART_DBG.println(WiFi.localIP());

  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {

   
  //  webserver_setup();   
       
  
  }
}


void CVdmNet::setupEth() {
  if (WT32_ETH01_isConnected()) {
    if (!serverIsStarted) {
      UART_DBG.println(" server is started ");
      initServer();
      UART_DBG.print(F("HTTP EthernetWebServer is @ IP : "));
      UART_DBG.println(ETH.localIP()); 
      serverIsStarted=true; 
    } else {
      server.handleClient();
    }
  }  
}

void  CVdmNet::initServer() {
    // define on events
  server.on(F("/"), handleRoot);
  server.onNotFound(handleNotFound);
  server.on(F("/plain"),handlePlain);

  server.on(F("/valves"),handleValves);
  server.on(F("/netinfo"),handleNetInfo);
  server.on(F("/netconfig"),handleNetConfig);
  server.on(F("/protconfig"),handleProtConfig);
  server.on(F("/sysinfo"),handleSysInfo);

  server.begin();
}

void CVdmNet::webServerLoop() 
{
  if (serverIsStarted) {
    server.handleClient();
  }
}




/*
void webserver_setup() {

  

    VdmNet.server.on("/status", []() {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");
      result += GetValveStatus();
      result += GetBottom(); 

      VdmNet.server.send(200, "text/html", result);
    });


    VdmNet.server.on("/log", []() {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");
      result += FPSTR(on_log);
      result += GetBottom(); 

      VdmNet.server.send(200, "text/html", result);
    });

    VdmNet.server.on("/getLogData", []() {
      String data = "";
      if (1) {
        while (logger.Available()) {
          data += logger.Pop() + "\n";
        }
      }
      else {
        data += F("SYS: ***CLEARLOG***\n");
        data += F("DATA:Logger is disabled\n");
        data += F("SYS:Logger is disabled\n");
      }

      VdmNet.server.send(200, "text/html", data);
    });

    VdmNet.server.on("/command", []() {
      
        String command = VdmNet.server.arg("cmd");
        logger.println("Command from frontend: '" + command + "'");
        
        VdmNet.server.send(200, "text/html", "OK");
     
    });

    VdmNet.server.on("/credits", []() {

      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");            
      result += "VdMot Controller was created with help and by borrowings from:<br><ul>";
      result += "<li>parts of WebFrontend: https://wiki.fhem.de/wiki/LaCrosseGateway_V1.x</li>";
      result += "<li>ideas for STM32 flash support: https://github.com/csnol/1CHIP-Programmers</li>";
      result += "</ul>";
      result += GetBottom();

      VdmNet.server.send(200, "text/html", makePage("VdMot Controller Credits Page", result));
    });

    VdmNet.server.on("/", []() {
      
      String starthtml = "<h1>VdMot Controller</h1><h2>Version ";
      starthtml += MAJORVERSION "." MINORVERSION;
      starthtml += "</h2><br>";
      starthtml += GetNavigation();
      VdmNet.server.send(200, "text/html", makePage("VdMot Controller Start Page", starthtml));
    });
    

    Serial.println("Setup webserver finished");

    
}

*/

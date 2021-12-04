/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano

  Comments:


***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************

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
#include "time.h"
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
/*
  snprintf(global_response_header_buffer, sizeof(global_response_header_buffer), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %i\r\nCache-Control: public, max-age=%i\r\n%s\r\n", content_type, fsize, cache_time_in_seconds, content_encoding);
 */

void send_P(int code, PGM_P content_type, PGM_P content, size_t contentLength) 
{
    String header;
    header = "HTTP/1.0 200 OK";
    /*\r\nContent-Type:"+String(content_type)+"\r\nContent-Length:"+
             String (contentLength) +"\r\nCache-Control: public\r\n"; // +
        //     "Content-Encoding: gzip\r\n";*/

    //server.sendHeader(header,"",true);
    
    server.sendHeader(String(F("HTTP/1.0 200 OK")), String(FPSTR(content_type)), true);
    server.sendHeader(String(F("Content-Type")), String(FPSTR(content_type)), true);
    server.sendHeader(String(F("Content-Length")), String(FPSTR(contentLength)), true);
 
    server.sendHeader(String(F("Connection")), String(F("close")));
    server.sendContent_P(content, contentLength);
}


void valvesCalib()
{
  // todo
}



void handleRoot() {
  server.send_P(200, "text/html", (const char*) tfs____web_pages_h_index_html , sizeof(tfs____web_pages_h_index_html)); 
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
  if (server.method() == HTTP_GET) server.send(200,aj,getValvesStatus());
  if (server.method() == HTTP_POST) {
    postValvesPos();
    server.send(200,tp,"ok");
  }
}

void handleTemps()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getTempsStatus(VdmConfig.configFlash.tempsConfig));
}

void handleNetInfo() 
{ 
  server.send(200,aj,getNetInfo(ETH,VdmConfig.configFlash.netConfig));
}

void handleNetConfig()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getNetConfig(VdmConfig.configFlash.netConfig));
  if (server.method() == HTTP_POST) {
      VdmConfig.postNetCfg (server.arg("plain"));
      server.send(200,tp,"ok");
  }
}

void handleProtConfig()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getProtConfig(VdmConfig.configFlash.protConfig));
  if (server.method() == HTTP_POST) {
      VdmConfig.postProtCfg (server.arg("plain"));
      server.send(200,tp,"ok");  
  }
}
void handleValvesConfig()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getValvesConfig (VdmConfig.configFlash.valvesConfig));
  if (server.method() == HTTP_POST) {
      VdmConfig.postValvesCfg (server.arg("plain"));
      server.send(200,tp,"ok");
  }
}

void handleTempsConfig()
{
  if (server.method() == HTTP_GET) server.send(200,aj,getTempsConfig (VdmConfig.configFlash.tempsConfig));
  if (server.method() == HTTP_POST) {
      VdmConfig.postTempsCfg (server.arg("plain"));
      server.send(200,tp,"ok");
  }
}

void handleSysInfo() 
{ 
  server.send(200,aj,getSysInfo());
}

void handleCmd() 
{ 
  if (server.method() == HTTP_POST) {
    String payload = server.arg("plain");

    const size_t capacity = JSON_ARRAY_SIZE(5) + 5 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 200;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, payload);
    UART_DBG.println(payload);

    if (doc["vcalib"]==1) {
      valvesCalib();
    }
    if (doc["reboot"]==1) {
      
      ESP.restart();
    }
    if (doc["savecfg"]==1) {
      VdmConfig.writeConfig();
      VdmConfig.readConfig();
     // ESP.restart(); Todo
    }
    if (doc["resetcfg"]==1) {
      VdmConfig.clearConfig();  
      VdmConfig.writeConfig();
      ESP.restart();
    }
    if (doc["restorecfg"]==1) {
      VdmConfig.init();  
      VdmConfig.writeConfig(); 
      ESP.restart();
    }
    doc.clear();
  }
  
  server.send(200,tp,"ok");
}




CVdmNet::CVdmNet()
{
}


void CVdmNet::init()
{
  serverIsStarted = false;
  dataBrokerIsStarted = false;
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);
// todo
  // Static IP, leave without this line to get IP via DHCP
  //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
  //ETH.config(myIP, myGW, mySN, myDNS);

  WT32_ETH01_onEvent();
}



void CVdmNet::setup() {
  setupEth();/* todo :
  if (VdmConfig.configFlash.netConfig.eth_wifi==0) {
      setupEth();
  } else {
      setupWifi();
  }
  if (VdmConfig.configFlash.protConfig.dataProtocol==mqttProtocol) 
  {
    mqtt_setup(VdmConfig.configFlash.protConfig.brokerIp,VdmConfig.configFlash.protConfig.brokerPort);
    dataBrokerIsStarted=true;
  }
  */
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
// todo
   
  //  webserver_setup();   
       
  
  }
}

void CVdmNet::setupNtp() {
  // Init and get the time
  configTime(VdmConfig.configFlash.netConfig.timeOffset, 
             VdmConfig.configFlash.netConfig.daylightOffset, 
             VdmConfig.configFlash.netConfig.timeServer);
}

void CVdmNet::setupEth() {
  if (WT32_ETH01_isConnected()) {
    if (!serverIsStarted) {
      UART_DBG.println(" server is started ");
      initServer();
      UART_DBG.print(F("HTTP EthernetWebServer is @ IP : "));
      UART_DBG.println(ETH.localIP()); 
      serverIsStarted=true; 
      setupNtp();
    } else {
      server.handleClient();
    }
  }  
}

void  CVdmNet::initServer() {
  // define on events
  server.on(F("/"), handleRoot);
  server.onNotFound(handleNotFound);
  server.on(F("/valves"),handleValves);
  server.on(F("/temps"),handleTemps);
  server.on(F("/netinfo"),handleNetInfo);
  server.on(F("/netconfig"),handleNetConfig);
  server.on(F("/protconfig"),handleProtConfig);
  server.on(F("/valvesconfig"),handleValvesConfig);
  server.on(F("/tempsconfig"),handleTempsConfig);
  server.on(F("/sysinfo"),handleSysInfo);
  server.on(F("/cmd"),handleCmd);
  

  server.begin();
}

void CVdmNet::netLoop() 
{
  if (serverIsStarted) {
    server.handleClient();
  }
  if (dataBrokerIsStarted) {
    switch (VdmConfig.configFlash.protConfig.dataProtocol) {
      case mqttProtocol:
      {
       // todo : mqtt_loop();
        break;
      }
    }
  }
}

void CVdmNet::checkNet() 
{
  if (VdmNet.serverIsStarted) {
    VdmNet.netLoop();
  } else {
    // check if net is connected
    VdmNet.setup();
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

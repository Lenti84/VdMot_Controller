/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano, Lenti84

  Comments:

  Version :

  Modifcations :


***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE DEVELOPER OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller

*END************************************************************************/


#define USING_CORE_ESP32_CORE_V200_PLUS     true
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
#include <AsyncWebServer_WT32_ETH01.h>
#include "web.h"
#include "tfs.h"

#include "time.h"

extern "C" {
  #include "tfs_data.h"
}
#include "VdmTask.h"

#include <AsyncJson.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

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

void CVdmNet::postSetValve (JsonObject doc)
{
  UART_DBG.println("postSetValue");
  uint8_t index;
  if (doc["valve"]) {
    index=(doc["valve"].as<uint8_t>())-1;
    if (doc["value"]) actuators[index].target_position = doc["value"];
  }
}

void valvesCalib()
{
  // todo
}

int8_t checkEntry (String url) 
{
  int8_t i = 0;
  TFS_DIR_ENTRY *entry = (TFS_DIR_ENTRY*) tfs_data;
  String thisUrl=url;
  if (url=="/") thisUrl = "/index.html";
 
  while (entry->NAME != NULL)
  {
      if (thisUrl == String(entry->NAME))
      {
        return(i);
      }
      entry++;
      i++;
  }
  
  return(-1);
}

void handleRoot(AsyncWebServerRequest *request) 
{
  int8_t index;
  
  index=checkEntry(request->url());
  if (index>=0) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", 
                                      tfs_data[index].DATA, tfs_data[index].SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  } else {
    String message = "File "+request->url()+" Not Found\n\n"; 
    request->send(404, "text/plain", message);
  }
}

void handleNotFound(AsyncWebServerRequest *request)
{
  if (checkEntry(request->url())>=0) {
    handleRoot(request); 
  } else {

    String message = "File Not Found\n\n";

    message += "URI: ";
    //message += server.uri();
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";

    for (uint8_t i = 0; i < request->args(); i++)
    {
      message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }

    request->send(404, "text/plain", message);
  }
}

void handleValves(AsyncWebServerRequest *request)
{
  request->send(200,aj,getValvesStatus());
}

void handleTemps(AsyncWebServerRequest *request)
{
  request->send(200,aj,getTempsStatus(VdmConfig.configFlash.tempsConfig));
}

void handleNetInfo(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,getNetInfo(ETH,VdmConfig.configFlash.netConfig));
}

void handleNetConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,getNetConfig(VdmConfig.configFlash.netConfig));
}

void handleProtConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,getProtConfig(VdmConfig.configFlash.protConfig));
}
void handleValvesConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,getValvesConfig (VdmConfig.configFlash.valvesConfig));
}

void handleTempsConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,getTempsConfig (VdmConfig.configFlash.tempsConfig));
}

void handleSysInfo(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,getSysInfo());
}

void handleCmd(JsonObject doc) 
{ 
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
}




CVdmNet::CVdmNet()
{
}


void CVdmNet::init()
{
  serverIsStarted = false;
  wifiState = wifiIdle;
  ethState = ethIdle;
  dataBrokerIsStarted = false;
}



void CVdmNet::setup() {
  #ifdef netDebug
    UART_DBG.print("Interface type ");
    UART_DBG.println(VdmConfig.configFlash.netConfig.eth_wifi);
  #endif
 
  VdmConfig.configFlash.netConfig.eth_wifi=0;
  switch (VdmConfig.configFlash.netConfig.eth_wifi) {
    case interfaceAuto :
      {
        setupEth();
        setupWifi();
        break;
      }
      case interfaceEth :
      {
        setupEth();
        break;
      }
      case interfaceWifi :
      {
        setupWifi();
        break;
      }
  }
}

void CVdmNet::setupEth() {
  #ifdef netDebug
    UART_DBG.print("Setup Eth ");
    UART_DBG.println (ethState);
    UART_DBG.print("Server started ");
    UART_DBG.println (serverIsStarted);
  #endif
  if (!serverIsStarted) {
    switch (ethState) {
      case wifiIdle :
      {  
        ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);
        // todo
        // Static IP, leave without this line to get IP via DHCP
        //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
        //ETH.config(myIP, myGW, mySN, myDNS);

        WT32_ETH01_onEvent();
        ethState=ethIsStarting;
        break;
      }
      case ethIsStarting :
      {
        if (WT32_ETH01_isConnected()) {
          initServer();
          setupNtp();
          UART_DBG.println(ETH.localIP());
          serverIsStarted=true; 
          ethState=ethConnected;
          interfaceType=currentInterfaceIsEth;
          wifiState=wifiDisabled; 
          WiFi.disconnect(); 
        }
        break;
      }
      case ethConnected : break;
      case ethDisabled : break;
    }
  }  
}


void CVdmNet::setupWifi() {
  #ifdef netDebug
  UART_DBG.print("Setup Wifi ");
  UART_DBG.println (wifiState);
  #endif

  if (!serverIsStarted) {
    switch (wifiState) {
      case wifiIdle :
      {
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(VdmConfig.configFlash.netConfig.ssid, VdmConfig.configFlash.netConfig.pwd);
        wifiState=wifiIsStarting;
        break;
      }
      case wifiIsStarting :
      {
        if (WiFi.status() == WL_CONNECTED) {
          initServer();
          setupNtp();
          UART_DBG.println(WiFi.localIP());
          serverIsStarted=true; 
          wifiState=wifiConnected;
          interfaceType=currentInterfaceIsWifi;
        }
        break;
      }
      case wifiConnected : break;
      case wifiDisabled : break;
    }
  }
}

void CVdmNet::setupNtp() {
  // Init and get the time
  configTime(VdmConfig.configFlash.netConfig.timeOffset, 
             VdmConfig.configFlash.netConfig.daylightOffset, 
             VdmConfig.configFlash.netConfig.timeServer);
}

void  CVdmNet::initServer() {
  // define on events
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {handleRoot(request);});
  server.onNotFound(handleNotFound);
  server.on("/valves",HTTP_GET,[](AsyncWebServerRequest * request) {handleValves(request);});
  server.on("/temps",HTTP_GET,[](AsyncWebServerRequest * request) {handleTemps(request);});
  server.on("/netinfo",HTTP_GET,[](AsyncWebServerRequest * request) {handleNetInfo(request);});
  server.on("/netconfig",HTTP_GET,[](AsyncWebServerRequest * request) {handleNetConfig(request);});
  server.on("/protconfig",HTTP_GET,[](AsyncWebServerRequest * request) {handleProtConfig(request);});
  server.on("/valvesconfig",HTTP_GET,[](AsyncWebServerRequest * request) {handleValvesConfig(request);});
  server.on("/tempsconfig",HTTP_GET,[](AsyncWebServerRequest * request) {handleTempsConfig(request);});
  server.on("/sysinfo",HTTP_GET,[](AsyncWebServerRequest * request) {handleSysInfo(request);});
 
  AsyncCallbackJsonWebHandler* setValveHandler = new AsyncCallbackJsonWebHandler("/setValve", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmNet.postSetValve (jsonObj);
      request->send(200, "text/plain", "ok");
    } else request->send(400, "text/plain", "Not an object");
  });
  server.addHandler(setValveHandler);

  AsyncCallbackJsonWebHandler* netCfgHandler = new AsyncCallbackJsonWebHandler("/netconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postNetCfg (jsonObj);
      request->send(200, "text/plain", "ok");
    } else request->send(400, "text/plain", "Not an object");
  });
  server.addHandler(netCfgHandler);
  
  AsyncCallbackJsonWebHandler* protCfgHandler = new AsyncCallbackJsonWebHandler("/protconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postProtCfg (jsonObj);
      request->send(200, "text/plain", "ok");
    } else request->send(400, "text/plain", "Not an object");
  });
  server.addHandler(protCfgHandler);

  AsyncCallbackJsonWebHandler* valvesCfgHandler = new AsyncCallbackJsonWebHandler("/valvesconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postValvesCfg (jsonObj);
      request->send(200, "text/plain", "ok");
    } else request->send(400, "text/plain", "Not an object");
  });
  server.addHandler(valvesCfgHandler);
  
  AsyncCallbackJsonWebHandler* tempsCfgHandler = new AsyncCallbackJsonWebHandler("/tempsconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postTempsCfg (jsonObj);
      request->send(200, "text/plain", "ok");
    } else request->send(400, "text/plain", "Not an object");
  });
  server.addHandler(tempsCfgHandler);

  AsyncCallbackJsonWebHandler* cmdHandler = new AsyncCallbackJsonWebHandler("/cmd", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      handleCmd (jsonObj);
      request->send(200, "text/plain", "ok");
    } else request->send(400, "text/plain", "Not an object");
  });
  server.addHandler(cmdHandler);

  server.begin();
}

void CVdmNet::startBroker()
{
  switch (VdmConfig.configFlash.protConfig.dataProtocol) {
    case mqttProtocol:
    {
      mqtt_setup(VdmConfig.configFlash.protConfig.brokerIp,VdmConfig.configFlash.protConfig.brokerPort);
      VdmTask.startMqtt();
      dataBrokerIsStarted=true;
      break;
    }
  }
}

void CVdmNet::mqttBroker()
{
   mqtt_loop();
}

void CVdmNet::checkNet() 
{
  if (serverIsStarted) {
    VdmTask.deleteTask(VdmTask.taskIdCheckNet);
    if (VdmConfig.configFlash.protConfig.dataProtocol!=noneProtocol)
    {
      startBroker();
    }
    VdmTask.startApp();
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

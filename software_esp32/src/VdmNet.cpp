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
#include "WT32AsyncOTA.h"

#include "web.h"
#include "tfs.h"

#include "time.h"

#include "stm32.h"
#include "stm32ota.h"
#include <ESPmDNS.h>
#include "telnet.h"
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <FS.h>

#include "Logger.h"

extern "C" {
  #include "tfs_data.h"
}
#include "VdmTask.h"
#include "VdmSystem.h"
#include "mqtt.h"

#include <AsyncJson.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

CVdmNet VdmNet;

#define aj  "application/json"
#define tp  "text/plain"
#define gz  "Content-Encoding : gzip"


 // Create a new empty syslog instance
 Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

// server handles --------------------------------------------------

void CVdmNet::postSetValve (JsonObject doc)
{
  #ifdef netDebug
  UART_DBG.println("postSetValue");
  #endif

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

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
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
    AsyncWebServerResponse *response = request->beginResponse_P(200, getContentType (tfs_data[index].NAME), 
                                      tfs_data[index].DATA, tfs_data[index].SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  } else {
    String message = "File "+request->url()+" Not Found\n\n"; 
    request->send(404, "text/plain", message);
  }
}


void handleWebPageStmUpdate(AsyncWebServerRequest *request) 
{
  int8_t index;
  String thisUrl = request->url();
  if (!thisUrl.endsWith(".html")) thisUrl+=".html";
  index=checkEntry(thisUrl);
  if (index>=0) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, getContentType (tfs_data[index].NAME), 
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
  request->send(200,aj,Web.getValvesStatus());
}

void handleTemps(AsyncWebServerRequest *request)
{
  request->send(200,aj,Web.getTempsStatus(VdmConfig.configFlash.tempsConfig));
}

void handleNetInfo(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getNetInfo(VdmNet.networkInfo));
}

void handleNetConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,Web.getNetConfig(VdmConfig.configFlash.netConfig));
}

void handleProtConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,Web.getProtConfig(VdmConfig.configFlash.protConfig));
}
void handleValvesConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,Web.getValvesConfig (VdmConfig.configFlash.valvesConfig));
}

void handleTempsConfig(AsyncWebServerRequest *request)
{
  request->send(200,aj,Web.getTempsConfig (VdmConfig.configFlash.tempsConfig));
}

void handleSysInfo(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getSysInfo());
}

void handleGetFSDir(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getSysDynInfo());
}

void handleSysDynInfo(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getFSDir());
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
    ESP.restart(); 
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
  if (doc["startStmUpdate"]==1) {
    // todo stmUpdate
  }
}

void handleUploadSTM32(AsyncWebServerRequest *request, const String& filename, size_t index, 
            uint8_t *data, size_t len, bool final)
{
  if(!index){
   // logOutput((String)"UploadStart: " + filename);
    // open the file on first call and store the file handle in the request object
    String thisFile = filename;
    if (!filename.startsWith("/stm/")) thisFile = "/stm/" + filename;
    request->_tempFile = SPIFFS.open(thisFile, "w");
  }
  if(len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data,len);
  }
  if(final){
   // logOutput((String)"UploadEnd: " + filename + "," + index+len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    size_t uploadsize = request->_tempFile.size();
    request->send(200, "text/plain", "File Uploaded !"); //redirect("/fsdir");//
  }
}
 

void handleGetLogData (AsyncWebServerRequest *request) 
{
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
  request->send(200, "text/html", data);
}

CVdmNet::CVdmNet()
{
}

void CVdmNet::UpdateSTM32(String updateFileName)
{
  //FlashMode();
  //STM32_begin();  // contains STM32ota_begin and clear STM32 buffer
   /*   
  if (stm32Erase() == STM32ACK) {
  } else {
    return;
  }

  uint8_t cflag;
  File fsUploadFile;
  int lastbuf = 0;
  int bini = 0;
  String flashwr;
  uint8_t binread[256];

  fsUploadFile = SPIFFS.open(updateFileName, "r");
  if (fsUploadFile) {
     bini = fsUploadFile.size() / 256;
     lastbuf = fsUploadFile.size() % 256;
     flashwr = String(bini) + "-" + String(lastbuf) + "<br>";
     for (int i = 0; i < bini; i++) {
       fsUploadFile.read(binread, 256);
       stm32SendCommand(STM32WR);
       while (!UART_STM32.available()) ;
       cflag = UART_STM32.read();
       if (cflag == STM32ACK)
         if (stm32Address(STM32STADDR + (256 * i)) == STM32ACK) {
           if (stm32SendData(binread, 255) == STM32ACK)
             flashwr += ".";
           else flashwr = "Error";
         }
     }
     fsUploadFile.read(binread, lastbuf);
     stm32SendCommand(STM32WR);
     //todo ; handle in task
     while (!UART_STM32.available()) ;
     cflag = UART_STM32.read();
     if (cflag == STM32ACK)
       if (stm32Address(STM32STADDR + (256 * bini)) == STM32ACK) {
         if (stm32SendData(binread, lastbuf) == STM32ACK)
           flashwr += "<br>Finished<br>";
         else flashwr = "Error";
       }
     fsUploadFile.close();
 }*/
}

void CVdmNet::init()
{
  serverIsStarted = false;
  wifiState = wifiIdle;
  ethState = ethIdle;
  dataBrokerIsStarted = false;
  if (!SPIFFS.begin(true)) {
    UART_DBG.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  UART_DBG.println("SPIFFS booted");

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
        if (VdmConfig.configFlash.netConfig.dhcpEnabled==0) 
        {
          ETH.config(VdmConfig.configFlash.netConfig.staticIp, 
          VdmConfig.configFlash.netConfig.gateway, 
          VdmConfig.configFlash.netConfig.mask,VdmConfig.configFlash.netConfig.dnsIp);
        }
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
          networkInfo.interfaceType=currentInterfaceIsEth;
          networkInfo.dhcpEnabled=VdmConfig.configFlash.netConfig.dhcpEnabled;
          networkInfo.ip=ETH.localIP();
          networkInfo.gateway=ETH.gatewayIP();
          networkInfo.dnsIp=ETH.dnsIP();
          networkInfo.mask=ETH.subnetMask();
          networkInfo.mac=ETH.macAddress();
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
        if (VdmConfig.configFlash.netConfig.dhcpEnabled==0) 
        {
          WiFi.config(VdmConfig.configFlash.netConfig.staticIp, 
          VdmConfig.configFlash.netConfig.gateway, 
          VdmConfig.configFlash.netConfig.mask,VdmConfig.configFlash.netConfig.dnsIp);
        }
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
          networkInfo.interfaceType=currentInterfaceIsWifi;
          networkInfo.dhcpEnabled=VdmConfig.configFlash.netConfig.dhcpEnabled;
          networkInfo.ip=WiFi.localIP();
          networkInfo.gateway=WiFi.gatewayIP();
          networkInfo.dnsIp=WiFi.dnsIP();
          networkInfo.mask=WiFi.subnetMask();
          networkInfo.mac=WiFi.macAddress();
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
  server.on("/sysdyninfo",HTTP_GET,[](AsyncWebServerRequest * request) {handleSysDynInfo(request);});
  server.on("/fsdir",HTTP_GET,[](AsyncWebServerRequest * request) {handleSysDynInfo(request);});
  server.on("/logdata",HTTP_GET,[](AsyncWebServerRequest * request) {handleGetLogData(request);});
  server.on("/stmupdate", HTTP_GET, [](AsyncWebServerRequest * request) {handleWebPageStmUpdate(request);});

  server.on("/uploadstm", HTTP_POST, [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                    size_t len, bool final) {handleUploadSTM32(request, filename, index, data, len, final);}
  );

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
  WT32AsyncOTA.begin(&server);    // Start WT32OTA
  server.begin();
}

void CVdmNet::startBroker()
{
  switch (VdmConfig.configFlash.protConfig.dataProtocol) {
    case mqttProtocol:
    {
      Mqtt.mqtt_setup(VdmConfig.configFlash.protConfig.brokerIp,VdmConfig.configFlash.protConfig.brokerPort);
      VdmTask.startMqtt();
      dataBrokerIsStarted=true;
      break;
    }
  }
}

void CVdmNet::mqttBroker()
{
   Mqtt.mqtt_loop();
}

void CVdmNet::checkNet() 
{
  if (serverIsStarted) {
    VdmTask.deleteTask(VdmTask.taskIdCheckNet);

    VdmSystem.getSystemInfo();
    if (VdmConfig.configFlash.protConfig.dataProtocol!=noneProtocol)
    {
      startBroker();
    }
    // todo STM32_Start();

    if (MDNS.begin("esp32")) {
      UART_DBG.println("MDNS responder started");
    }

    // todo   
    //telnet_setup();

    // prepare syslog configuration here (can be anywhere before first call of 
    // log/logf method)
    if (VdmConfig.configFlash.netConfig.syslogLevel>0)
    {
     
      syslog.server(IPAddress(VdmConfig.configFlash.netConfig.syslogIp), VdmConfig.configFlash.netConfig.syslogPort);
      syslog.deviceHostname(DEVICE_HOSTNAME);
      syslog.appName(APP_NAME);
      syslog.defaultPriority(LOG_KERN);
    }


    VdmTask.startApp();
    VdmTask.startServices();
  } else {
    // check if net is connected
    VdmNet.setup();
  }
}


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


#include <stdint.h>
#include <VdmNet.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "globals.h"
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
#include "ServerServices.h"
#include "VdmSystem.h"
#include "VdmTask.h"

extern "C" {
  #include "tfs_data.h"
}


#include <AsyncJson.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

CServerServices ServerServices;

#define aj  "application/json"
#define tp  "text/plain"
#define th  "text/html"
#define gz  "Content-Encoding : gzip"

#define resOk "{\"res\":\"ok\"}"

// server handles --------------------------------------------------

void valvesCalib()
{
  // todo
}

void restart ()
{  
  ESP.restart();
}

void writeConfig ()
{  
  VdmConfig.writeConfig();
  ESP.restart();
}

void resetConfig ()
{  
  VdmConfig.clearConfig(); 
  VdmConfig.writeConfig();
  ESP.restart();
}

void restoreConfig ()
{  
  VdmConfig.setDefault();
  VdmConfig.writeConfig();
  ESP.restart();
}

void clearFS ()
{  
  VdmSystem.clearFS();
}

void CServerServices::postSetValve (JsonObject doc)
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

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return th;
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return tp;
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
    request->send(404, tp, message);
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
    request->send(404, tp, message);
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

    request->send(404, tp, message);
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

void handleSysDynInfo(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getSysDynInfo());
}

void handleGetFSDir(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getFSDir());
}

void handleStmUpdStatus(AsyncWebServerRequest *request) 
{ 
  request->send(200,aj,Web.getStmUpdStatus());
}


void handleCmd(JsonObject doc) 
{ 
  typedef void (*fp)();
  fp  fpList[] = {&valvesCalib,&restart,&writeConfig,&resetConfig,&restoreConfig,&clearFS} ;

  char const *names[]=  {"vCalib", "reboot", "saveCfg","resetCfg","restoreCfg","clearFS", NULL};
  char const **p;

  if (!doc["action"].isNull()) {
    const char* d=doc["action"].as<const char*>();
    uint8_t i=0;
  
    for (p=names; *p!=NULL; p++)
    {
      if (strcmp(d, *p)==0)
      {
        fpList[i]();
        break;
      }
      i++;
    }
  }
}

void handleUploadFile(AsyncWebServerRequest *request, const String& filename, size_t index, 
            uint8_t *data, size_t len, bool final)
{
  if(!index){
    String thisFileName = filename;
    // open the file on first call and store the file handle in the request object
    UART_DBG.println("file has arguments : "+String(request->args()));
    if (request->args()) {
        UART_DBG.println("file has argument "+request->arg("dir"));
        thisFileName = "/"+request->arg("dir")+"/"+filename; 
    }

     UART_DBG.println("filename : "+thisFileName);
    request->_tempFile = SPIFFS.open(thisFileName, "w");
  }
  if(len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data,len);
  //  UART_DBG.println("file chunk");
  }
  if(final){
    // close the file handle as the upload is now done
    request->_tempFile.close();
    request->send(200, aj, Web.getFSDir()); 
    UART_DBG.println("upload finished");
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
  request->send(200, th, data);
}

CServerServices::CServerServices()
{
}

void CServerServices::stmDoUpdate(JsonObject doc)
{

  if (!doc["file"].isNull()) {
    if (!doc["cmd"].isNull()) {
    String thisFileName = doc["file"].as<const char*>();
    if (!thisFileName.startsWith("/")) thisFileName = "/"+thisFileName;
    uint8_t command = doc["cmd"];
    UART_DBG.println("file : "+thisFileName + "Comand "+ String(command));
    if (command==0) VdmTask.startStm32Ota(STM32OTA_START,thisFileName);
    if (command==1) VdmTask.startStm32Ota(STM32OTA_STARTBLANK,thisFileName);
  }
  }
}

void  CServerServices::initServer() {
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
  server.on("/fsdir",HTTP_GET,[](AsyncWebServerRequest * request) {handleGetFSDir(request);});
  server.on("/logdata",HTTP_GET,[](AsyncWebServerRequest * request) {handleGetLogData(request);});
  server.on("/stmupdate", HTTP_GET, [](AsyncWebServerRequest * request) {handleWebPageStmUpdate(request);});
  server.on("/stmupdstatus", HTTP_GET, [](AsyncWebServerRequest * request) {handleStmUpdStatus(request);});

  server.on("/fupload", HTTP_POST, [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                    size_t len, bool final) {handleUploadFile(request, filename, index, data, len, final);}
  );

  AsyncCallbackJsonWebHandler* stmDoUpdateHandler = new AsyncCallbackJsonWebHandler("/stmdoupdate", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      ServerServices.stmDoUpdate (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(stmDoUpdateHandler);

  AsyncCallbackJsonWebHandler* setValveHandler = new AsyncCallbackJsonWebHandler("/setvalve", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      ServerServices.postSetValve (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(setValveHandler);

  AsyncCallbackJsonWebHandler* netCfgHandler = new AsyncCallbackJsonWebHandler("/netconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postNetCfg (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(netCfgHandler);
  
  AsyncCallbackJsonWebHandler* protCfgHandler = new AsyncCallbackJsonWebHandler("/protconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postProtCfg (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(protCfgHandler);

  AsyncCallbackJsonWebHandler* valvesCfgHandler = new AsyncCallbackJsonWebHandler("/valvesconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postValvesCfg (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(valvesCfgHandler);
  
  AsyncCallbackJsonWebHandler* tempsCfgHandler = new AsyncCallbackJsonWebHandler("/tempsconfig", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      VdmConfig.postTempsCfg (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(tempsCfgHandler);

  AsyncCallbackJsonWebHandler* tempsAuthHandler = new AsyncCallbackJsonWebHandler("/auth", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      String auth=VdmConfig.handleAuth (jsonObj);
      request->send(200, aj, auth);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(tempsAuthHandler);

  AsyncCallbackJsonWebHandler* cmdHandler = new AsyncCallbackJsonWebHandler("/cmd", [](AsyncWebServerRequest *request, JsonVariant &json) {
    if (json.is<JsonObject>()) {
      JsonObject&& jsonObj = json.as<JsonObject>();
      handleCmd (jsonObj);
      request->send(200, aj, resOk);
    } else request->send(400, tp, "Not an object");
  });
  server.addHandler(cmdHandler);
  WT32AsyncOTA.begin(&server);    // Start WT32OTA
  server.begin();
}



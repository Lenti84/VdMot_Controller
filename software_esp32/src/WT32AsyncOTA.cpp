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
/*
    derived from  AsyncElegantOTA using WT32-ETH
    https://github.com/ayushsharma82/AsyncElegantOTA

*/

#include "Arduino.h"
#include "stdlib_noniso.h"

#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include "ESPAsyncTCP.h"
    #include "flash_hal.h"
    #include "FS.h"
#elif defined(ESP32)
    #include "WiFi.h"
    #include "AsyncTCP.h"
    #include "Update.h"
    #include "esp_int_wdt.h"
    #include "esp_task_wdt.h"
    #include "Services.h"
#endif

#include "Hash.h"
#include <AsyncWebServer_WT32_ETH01.h>
#include "WT32AsyncOTA.h"
#include "FS.h"



CWT32AsyncOTA WT32AsyncOTA;

CWT32AsyncOTA::CWT32AsyncOTA()
{
    _id = getID();
    _userName = "";
    _pwd = "";
    _authRequired = false;
}

void CWT32AsyncOTA::setID(const char* id)
{
    _id = id;
}

void CWT32AsyncOTA::begin(AsyncWebServer *server, const char* userName, const char* pwd)
{
    _server = server;

    if (strlen(userName) > 0) {
        _authRequired = true;
        _userName = userName;
        _pwd = pwd;
    } else {
        _authRequired = false;
        _userName = "";
        _pwd = "";
    }

    _server->on("/update/identity", HTTP_GET, [&](AsyncWebServerRequest *request){
        if(_authRequired){
            if(!request->authenticate(_userName.c_str(), _pwd.c_str())){
                return request->requestAuthentication();
            }
        }
        #if defined(ESP8266)
            request->send(200, "application/json", "{\"id\": \""+_id+"\", \"hardware\": \"ESP8266\"}");
        #elif defined(ESP32)
            request->send(200, "application/json", "{\"id\": \""+_id+"\", \"hardware\": \"ESP32\"}");
        #endif
    });

    _server->on("/update", HTTP_GET, [&](AsyncWebServerRequest *request){
        if(_authRequired){
            if(!request->authenticate(_userName.c_str(), _pwd.c_str())){
                return request->requestAuthentication();
            }
        }
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", WT32Upate_HTML, WT32Upate_HTML_SIZE);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->on("/update", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if(_authRequired){
            if(!request->authenticate(_userName.c_str(), _pwd.c_str())){
                return request->requestAuthentication();
            }
        }
        // the request handler is triggered after the upload has finished... 
        // create the response, add header, and send response
        AsyncWebServerResponse *response = request->beginResponse((Update.hasError())?500:200, "text/plain", (Update.hasError())?"FAIL":"OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        restart();
    }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        //Upload handler chunks in data
        if(_authRequired){
            if(!request->authenticate(_userName.c_str(), _pwd.c_str())){
                return request->requestAuthentication();
        }
    }

    if (!index) {
        if(!request->hasParam("MD5", true)) {
            return request->send(400, "text/plain", "MD5 parameter missing");
        }

        if(!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
            return request->send(400, "text/plain", "MD5 parameter invalid");
        }

        #if defined(ESP8266)
            int cmd = (filename == "filesystem") ? U_FS : U_FLASH;
            Update.runAsync(true);
            size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin((cmd == U_FS)?fsSize:maxSketchSpace, cmd)){ // Start with max available size
        #elif defined(ESP32)
            int cmd = (filename == "filesystem") ? U_SPIFFS : U_FLASH;
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { // Start with max available size
        #endif
            Update.printError(Serial);
            return request->send(400, "text/plain", "OTA could not begin");
            }
        }

        // Write chunked data to the free sketch space
        if (len) {
            if (Update.write(data, len) != len) {
                return request->send(400, "text/plain", "OTA could not begin");
            }
        }
            
        if (final) { // if the final flag is set then this is the last frame of data
            if (!Update.end(true)) { //true to set the size to the current progress
                Update.printError(Serial);
                return request->send(400, "text/plain", "Could not end OTA");
            }
        }else{
            return;
        }
    });
}

       
void  CWT32AsyncOTA::restart() 
{
    ESP.restart();
}

   
String  CWT32AsyncOTA::getID()
{
    String id = "";
    #if defined(ESP8266)
        id = String(ESP.getChipId());
    #elif defined(ESP32)
        id = String((uint32_t)ESP.getEfuseMac(), HEX);
    #endif
    id.toUpperCase();
    return id;
}



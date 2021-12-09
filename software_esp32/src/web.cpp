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



#include "globals.h"
#include "web.h"
#include "stm32.h"
#include "stm32ota.h"
#include "app.h"
#include <SPIFFS.h>
#include <FS.h>
#include "TypedQueue.h"
#include "VdmNet.h"
#include "VdmConfig.h"


uint8_t binread[256];
int lastbuf = 0;
int bini = 0;

String stringtmp;
int rdtmp;
int stm32ver;
bool initflag = 0;
bool Runflag = 0;

File fsUploadFile;


//const char* serverIndex = "<h1>Upload STM32 BinFile</h1><h2><br><br><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Upload'></form></h2>";

String GetTop();
String GetBottom();
String GetNavigation();
String GetValveStatus();


const char on_log[] PROGMEM = ""
"<script>\n"
"  function sendCommand() {\n"
"    var cmd = document.getElementById('commandText').value;\n"
"    var request = new XMLHttpRequest();\n"
"    request.open('GET', 'command?cmd=' + encodeURIComponent(cmd), true);\n"
"    request.send();\n"
"  };\n"
"  function clearList(what) {\n"
"    document.getElementById(what + 'Div').innerHTML = '';\n"
"    filter(what);\n"
"  };\n"
"  function filter(what) {\n"
"    var el = document.getElementById(what + 'DivFilter');\n"
"    var text0 = el.value.toLowerCase();\n"
"    var elements = document.getElementsByClassName(what + 'Line');\n"
"    var names = '';\n"
"    var ct = 0;\n"
"    for (var i = 0; i < elements.length; i++) {\n"
"      if (elements[i].innerHTML.toLowerCase().indexOf(text0) == -1) {\n"
"        elements[i].style.display = 'none';\n"
"      }\n"
"      else {\n"
"        elements[i].style.display = 'block';\n"
"        ct++;\n"
"      }\n"
"    }\n"
"    document.getElementById(what + 'RowCount').innerHTML = ct + \" rows\";\n"
"  };\n"
"  function run() {\n"
"    var el = document.getElementById('logDivFilter');\n"
"    el.onkeyup = function (evt) {\n"
"      filter('log');\n"
"    };\n"
"    var el = document.getElementById('dataDivFilter');\n"
"    el.onkeyup = function (evt) {\n"
"      filter('data');\n"
"    };\n"
"    getLogData();\n"
"  };\n"
"  function getLogData() {\n"
"    if (document.getElementById('enabled').checked == true) {\n"
"      var request = new XMLHttpRequest();\n"
"      request.onreadystatechange = function () {\n"
"        if (this.readyState == 4 && this.status == 200 && this.responseText != null && this.responseText != '') {\n"
"          var lines = this.responseText.split('\\n');\n"
"          for (var i = 0; i < lines.length; i++) {\n"
"            var txt = lines[i];\n"
"            if (txt != '') {\n"
"              if (txt == 'SYS: ***CLEARLOG***') {\n"
"                clearList('data');\n"
"                clearList('log');\n"
"              } else {\n"
"                var targetDiv = 'logDiv';\n"
"                var scrollCheckBox = 'scrollLogDiv';\n"
"                var prefix = 'log';\n"
"                if (txt.startsWith('DATA:')) {\n"
"                  prefix = 'data';\n"
"                  targetDiv = 'dataDiv';\n"
"                  scrollCheckBox = 'scrollDataDiv';\n"
"                  txt = txt.substring(5);\n"
"                }\n"
"                if (txt.startsWith('SYS:')) {\n"
"                  txt = txt.substring(4);\n"
"                }\n"
"                txt = new Date().toLocaleTimeString('de-DE') + ': ' + txt;\n"
"                document.getElementById(targetDiv).innerHTML += \"<div class='\" + prefix + \"Line'>\" + txt + '</div>';\n"
"                filter(prefix);\n"
"                if (document.getElementById(scrollCheckBox).checked == true) {\n"
"                  var objDiv = document.getElementById(targetDiv);\n"
"                  objDiv.scrollTop = objDiv.scrollHeight;\n"
"                }\n"
"              }\n"
"            }\n"
"          }\n"
"        }\n"
"      };\n"
"      request.open('GET', 'getLogData?nc=' + Math.random(), true);\n"
"      request.send();\n"
"    }\n"
"    setTimeout('getLogData()', 500);\n"
"  };\n"
"</script>\n"
"<body text=#ffffff bgcolor=#0DCBDB align=\"center\" onload='run()'>\n"
"  Command: <input id='commandText' size='100' onkeydown=\"if (event.keyCode == 13) sendCommand()\">\n"
"  <button type='button' onclick=\"sendCommand();\">Send</button>\n"
"  &nbsp;&nbsp;&nbsp;\n"
"  <input type='checkbox' id='enabled' value='true' checked>Enable logging\n"
"  <br><br>\n"
"  <i>ESP <-> STM32:</i>\n"
"  <input type='checkbox' id='scrollDataDiv' value='true' checked> Scroll\n"
"  <button type='button' onclick=\"clearList('data');\">Clear</button>\n"
"  Filter:\n"
"  <input id='dataDivFilter'>\n"
"  <span id='dataRowCount'></span>\n"
"  <div id='dataDiv' style='height: 250px; border:1px solid black; overflow:scroll;'></div>\n"
"  <br><i>Debug log:</i>\n"
"  <input type='checkbox' id='scrollLogDiv' value='true' checked> Scroll\n"
"  <button type='button' onclick=\"clearList('log');\">Clear</button>\n"
"  Filter:\n"
"  <input text='text' id='logDivFilter'>\n"
"  <span id='logRowCount'></span>\n"
"  <div id='logDiv' style='height: 250px; border:1px solid black; overflow:scroll;'></div>\n"
"</body>\n"
;

String ip2String (IPAddress ipv4addr)
{
  return ipv4addr.toString();
}

String getNetConfig (VDM_NETWORK_CONFIG netConfig)
{
  String result = "{\"ethWifi\":"+String(netConfig.eth_wifi)+","+
                  "\"dhcp\":"+String(netConfig.dhcpEnabled)+","+
                  "\"ip\":\""+ip2String(netConfig.staticIp)+"\","+
                  "\"mask\":\""+ip2String(netConfig.mask)+"\","+
                  "\"gw\":\""+ip2String(netConfig.gateway)+"\","+
                  "\"dns\":\""+ip2String(netConfig.dnsIp)+"\","+
                  "\"userName\":\""+String(netConfig.userName)+"\","+
                  "\"ssid\":\""+String(netConfig.ssid)+"\","+
                  "\"timeServer\":\""+String(netConfig.timeServer)+"\","+
                  "\"timeOffset\":"+String(netConfig.timeOffset)+","+
                  "\"timeDST\":"+String(netConfig.daylightOffset)+
                  "}";  
  return result;  
}

String getNetInfo(ETHClass ETH,VDM_NETWORK_CONFIG netConfig)
{
  String result = "{\"ethWifi\":"+String(VdmNet.interfaceType)+","+
                  "\"dhcp\":\""+String(netConfig.dhcpEnabled)+"\","+
                  "\"ip\":\""+ETH.localIP().toString()+"\","+
                  "\"mac\":\""+ETH.macAddress()+"\","+
                  "\"mask\":\""+ETH.subnetMask().toString()+"\","+
                  "\"gw\":\""+ETH.gatewayIP().toString()+"\","+
                  "\"dns\":\""+ETH.dnsIP().toString()+"\"}";
  return result;  
}

String getProtConfig (VDM_PROTOCOL_CONFIG protConfig)
{
  String result = "{\"prot\":"+String(protConfig.dataProtocol)+","+
                    "\"mqttIp\":\""+ip2String(protConfig.brokerIp)+"\","+
                    "\"mqttPort\":"+String(protConfig.brokerPort)+"}";  
  return result;  
}

String getValvesConfig (VDM_VALVES_CONFIG valvesConfig)
{
  String result = "{\"calib\":{\"dayOfCalib\":"+String(valvesConfig.dayOfCalib) + "," +
                  "\"hourOfCalib\":"+String(valvesConfig.hourOfCalib) + "},\"valves\":[" ;

  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    result += "{\"name\":\""+String(valvesConfig.valveConfig[x].name) + "\"," +
              "\"active\":"+String(valvesConfig.valveConfig[x].active) + "}";
    if (x<ACTUATOR_COUNT-1) result += ",";
  }  
  result += "]}"; 
  return result;  
}

String getTempsConfig (VDM_TEMPS_CONFIG tempsConfig)
{
  String result = "[";
  
  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    result += "{\"name\":\""+String(tempsConfig.tempConfig[x].name) + "\"," +
              "\"active\":"+String(tempsConfig.tempConfig[x].active) + "," +
              "\"offset\":"+String(((float)tempsConfig.tempConfig[x].offset)/10,1) + "}";
    if (x<ACTUATOR_COUNT-1) result += ",";
  }  
  result += "]"; 
  return result;  
}


String getSysInfo()
{
  struct tm timeinfo;
  char buf[50];
  String time;
 
  if(!getLocalTime(&timeinfo)){
    time = "Failed to obtain time";
  } else {
    strftime (buf, sizeof(buf), "%A, %B %d.%Y %H:%M:%S", &timeinfo);
    time = String(buf);
  }
  String result = "{\"wt32version\":\""+String(MAJORVERSION)+"."+String(MINORVERSION)+"\","+
                  "\"locTime\":\""+time+"\"}";
  return result;  
}

String getValvesStatus() 
{
  String result = "[";
  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    
    result += "{\"pos\":"+String(actuators[x].actual_position) + ","+
              "\"meanCur\":" + String(actuators[x].meancurrent) + ","+
              "\"targetPos\":" + String(actuators[x].target_position)+"}";      
    if (x<ACTUATOR_COUNT-1) result += ",";
  }  
  result += "]";
  return result;
}


String getTempsStatus(VDM_TEMPS_CONFIG tempsConfig) 
{
  String result = "[";
  int temperature;
  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    temperature = actuators[x].temperature+tempsConfig.tempConfig[x].offset;
    result += "{\"temp\":" + String(((float)temperature)/10,1)+"}";
    if (x<ACTUATOR_COUNT-1) result += ",";
  }  
  result += "]";
  return result;
}


// void handleFileUpload()
// {
//   if (server.uri() != "/upload") return;
//   HTTPUpload& upload = server.upload();
//   if (upload.status == UPLOAD_FILE_START) {
//     String filename = upload.filename;
//     if (!filename.startsWith("/")) filename = "/" + filename;
//     fsUploadFile = SPIFFS.open(filename, "w");
//     filename = String();
//   } else if (upload.status == UPLOAD_FILE_WRITE) {
//     if (fsUploadFile)
//       fsUploadFile.write(upload.buf, upload.currentSize);
//   } else if (upload.status == UPLOAD_FILE_END) {
//     if (fsUploadFile)
//       fsUploadFile.close();
//   }
// }


// void handleFlash()
// {
//   String FileName, flashwr;
//   uint8_t cflag, fnum = 256;
//   File dir = SPIFFS.open("/");
//   File file = dir.openNextFile();
//   while (file)
//   {
//     FileName = file.name();
//     file = dir.openNextFile();
//   }
//   fsUploadFile = SPIFFS.open(FileName, "r");
//   if (fsUploadFile) {
//     bini = fsUploadFile.size() / 256;
//     lastbuf = fsUploadFile.size() % 256;
//     flashwr = String(bini) + "-" + String(lastbuf) + "<br>";
//     for (int i = 0; i < bini; i++) {
//       fsUploadFile.read(binread, 256);
//       stm32SendCommand(STM32WR);
//       while (!UART_STM32.available()) ;
//       cflag = UART_STM32.read();
//       if (cflag == STM32ACK)
//         if (stm32Address(STM32STADDR + (256 * i)) == STM32ACK) {
//           if (stm32SendData(binread, 255) == STM32ACK)
//             flashwr += ".";
//           else flashwr = "Error";
//         }
//     }
//     fsUploadFile.read(binread, lastbuf);
//     stm32SendCommand(STM32WR);
//     while (!UART_STM32.available()) ;
//     cflag = UART_STM32.read();
//     if (cflag == STM32ACK)
//       if (stm32Address(STM32STADDR + (256 * bini)) == STM32ACK) {
//         if (stm32SendData(binread, lastbuf) == STM32ACK)
//           flashwr += "<br>Finished<br>";
//         else flashwr = "Error";
//       }
//     //flashwr += String(binread[0]) + "," + String(binread[lastbuf - 1]) + "<br>";
//     fsUploadFile.close();
//     String flashhtml = "<h1>Programming</h1><h2>" + flashwr +  "<br><br><a style=\"color:white\" href=\"/up\">Upload STM32 BinFile</a><br><br><a style=\"color:white\" href=\"/list\">List STM32 BinFile</a></h2>";
//     server.send(200, "text/html", makePage("Flash Page", flashhtml));
//   }
// }


// void handleFileDelete() {
//   int binhigh = 0;
//   String FileList = "File: ";
//   String FName;
//   File dir = SPIFFS.open("/");
//   File file = dir.openNextFile();
//   while (file) {
//     FName = file.name();
//     file = dir.openNextFile();
//   }
//   FileList += FName;
//   if (SPIFFS.exists(FName)) {
//     server.send(200, "text/html", makePage("Deleted", "<h2>" + FileList + " be deleted!<br><br><a style=\"color:white\" href=\"/list\">Return </a></h2>"));
//     SPIFFS.remove(FName);
//   }
//   else
//     return server.send(404, "text/html", makePage("File Not found", "404"));
// }


// void handleListFiles()
// {
//   String FileList = "Bootloader Ver: ";
//   String Listcode;
//   char blversion = 0;
//   File dir = SPIFFS.open("/");
//   blversion = stm32Version();
//   FileList += String((blversion >> 4) & 0x0F) + "." + String(blversion & 0x0F) + "<br> MCU: ";
//   FileList += STM32_CHIPNAME[stm32GetId()];
//   FileList += "<br><br> File: ";
//   File file = dir.openNextFile();
//   while (file)
//   {
//     String FileName = file.name();
//     File f = SPIFFS.open(file.name());
//     String FileSize = String(f.size());
//     int whsp = 6 - FileSize.length();
//     while (whsp-- > 0)
//     {
//       FileList += " ";
//     }
//     FileList +=  FileName + "   Size:" + FileSize;
//     file = dir.openNextFile();
//   }
//   Listcode = "<h1>List STM32 BinFile</h1><h2>" + FileList + "<br><br><a style=\"color:white\" href=\"/flash\">Flash Menu</a><br><br><a style=\"color:white\" href=\"/delete\">Delete BinFile </a><br><br><a style=\"color:white\" href=\"/up\">Upload BinFile</a></h2>";
//   server.send(200, "text/html", makePage("FileList", Listcode));
// }
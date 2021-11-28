
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

String int2String (uint8_t x, uint8_t l)
{
  char result[10];
  memset (result,0,sizeof(result));
  itoa(x,result,l);
  return result;
}

String int2_OnOFF (uint8_t x)
{
  return (x ? "On" : "Off"); 
}

String getNetConfig (VDM_NETWORK_CONFIG netConfig)
{
  String result = "{\"ethWifi\":"+int2String(netConfig.netConfigFlags.eth_wifi,2)+","+
                  "\"dhcp\":"+int2String(netConfig.netConfigFlags.dhcpEnabled,2)+","+
                  "\"ip\":\""+ip2String(netConfig.staticIp)+"\","+
                  "\"mask\":\""+ip2String(netConfig.mask)+"\","+
                  "\"gw\":\""+ip2String(netConfig.gateway)+"\","+
                  "\"dns\":\""+ip2String(netConfig.dnsIp)+"\"}";  
  return result;  
}

String getNetInfo(ETHClass ETH,VDM_NETWORK_CONFIG netConfig)
{
  String result = "{\"ethWifi\":"+int2String(netConfig.netConfigFlags.eth_wifi,2)+","+
                  "\"dhcp\":\""+int2String(netConfig.netConfigFlags.dhcpEnabled,2)+"\","+
                  "\"ip\":\""+ETH.localIP().toString()+"\","+
                  "\"mac\":\""+ETH.macAddress()+"\","+
                  "\"mask\":\""+ETH.subnetMask().toString()+"\","+
                  "\"gw\":\""+ETH.gatewayIP().toString()+"\","+
                  "\"dns\":\""+ETH.dnsIP().toString()+"\"}";
  return result;  
}

String getProtConfig (VDM_PROTOCOL_CONFIG protConfig)
{
  String result = "{\"prot\":"+int2String(protConfig.dataProtocol,2)+
                    "\"mqttIp\":\""+ip2String(protConfig.brokerIp)+"\""+
                    "\"mqttPort\":\""+int2String(protConfig.brokerPort,4)+"\"}";  
  return result;  
}


String getSysInfo()
{
  String result = "{\"wt32version\":\""+String(MAJORVERSION)+"."+String(MINORVERSION)+"\"}";
  return result;  
}


void postValvePos () 
{

}

String getValveStatus() 
{
  String result = "[";
  uint8_t x;
  int temperature;

  for (x=0;x<ACTUATOR_COUNT;x++) {
    result += "{";
    result += "\"pos\":"+String(actuators[x].actual_position) + ",";
    result += "\"meanCur\": " + String(actuators[x].meancurrent) + ",";
    temperature = actuators[x].temperature;
    result += "\"temp\": " + String(temperature/10) + "." + String(temperature%10);
    result += "}";
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
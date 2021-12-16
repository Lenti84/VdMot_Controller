#include "globals.h"
#include "web.h"
#include "stm32.h"
#include "stm32ota.h"
#include "app.h"
#include <SPIFFS.h>
#include <FS.h>
#include <WebServer.h>
#include "TypedQueue.h"



uint8_t binread[256];
int lastbuf = 0;
int bini = 0;

String stringtmp;
int rdtmp;
int stm32ver;
bool initflag = 0;
bool Runflag = 0;

File fsUploadFile;
WebServer server(80);

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


void webserver_loop() {
    server.handleClient();
}



void webserver_setup() {

    // server.on("/up", []() {
    //   server.send(200, "text/html", makePage("Select file", serverIndex));
    // });

    // server.on("/inline", []() {
    //   server.send(200, "text/plain", "this works as well");
    // });

    // server.on("/list", handleListFiles);

    // server.on("/programm", handleFlash);

    // server.on("/run", []() {
    //   String Runstate = "STM32 Restart and runing!<br><br> you can reflash MCU (click 1.FlashMode before return Home) <br><br> Or close Browser";
    //   // stm32Run();
    //   if (Runflag == 0) {
    //     RunMode();
    //     Runflag = 1;
    //     UART_STM32.begin(115200, SERIAL_8N1, STM32_RX, STM32_TX, false, 20000UL);
    //     while(UART_STM32.available()) UART_STM32.read();
    //   }
    //   else {
    //     FlashMode();
    //     STM32ota_begin();
    //     while(UART_STM32.available()) UART_STM32.read();
    //     delay(100);
    //     initflag = 0;
    //     Runflag = 0;
    //   }
    //   server.send(200, "text/html", makePage("Run", "<h2>" + Runstate + "<br><br><a style=\"color:white\" href=\"/run\">1.FlashMode </a><br><br><a style=\"color:white\" href=\"/\">2.Home </a></h2>"));
    // });

    // server.on("/erase", []() {
    //   if (stm32Erase() == STM32ACK)
    //     stringtmp = "<h1>Erase OK</h1><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>";
    //   else if (stm32Erasen() == STM32ACK)
    //     stringtmp = "<h1>Erase OK</h1><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>";
    //   else
    //     stringtmp = "<h1>Erase failure</h1><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>";
    //   server.send(200, "text/html", makePage("Erase page", stringtmp));
    // });

    // server.on("/flash", []() {
    //   stringtmp = "<h1>FLASH MENU</h1><h2><a style=\"color:white\" href=\"/programm\">Flash STM32</a><br><br><a style=\"color:white\" href=\"/erase\">Erase STM32</a><br><br><a style=\"color:white\" href=\"/run\">Run STM32</a><br><br><a style=\"color:white\" href=\"/list\">Return </a></h2>";
    //   server.send(200, "text/html", makePage("Flash page", stringtmp));
    // });

    // server.on("/delete", handleFileDelete);

    // server.onFileUpload(handleFileUpload);

    // server.on("/upload", []() {
    //   server.send(200, "text/html", makePage("FileList", "<h1> Uploaded OK </h1><br><br><h2><a style=\"color:white\" href=\"/list\">Return </a></h2>"));
    // });

    // server.on("/stm32", []() {
    //   STM32ota_begin();
    //   if (Runflag == 1) {        
    //     FlashMode();
    //     STM32ota_begin();
    //     //while(UART_STM32.available()) UART_STM32.read();
    //     delay(100);
    //     Runflag = 0;
    //   }
    //   //if (initflag == 0)
    //   //{
    //     delay(100);
    //     UART_STM32.write(STM32INIT);
    //     delay(10);
    //     if (UART_STM32.available() > 0);
    //     rdtmp = UART_STM32.read();
    //     if (rdtmp == STM32ACK )   {
    //       //initflag = 1;
    //       stringtmp = STM32_CHIPNAME[stm32GetId()];
    //     }
    //     else if (rdtmp == STM32NACK) {
    //       UART_STM32.write(STM32INIT);
    //       delay(10);
    //       if (UART_STM32.available() > 0);
    //       rdtmp = UART_STM32.read();
    //       if (rdtmp == STM32ACK)   {
    //         //initflag = 1;
    //         stringtmp = STM32_CHIPNAME[stm32GetId()];
    //       }
    //     }
    //     else
    //       stringtmp = "Error";


    //   //You have to keep below code "<h2>Version 1.0 by <a style=\"color:white\" href=\"https://github.com/csnol/1CHIP-Programmers\">CSNOL" in your sketch.
    //   String starthtml = "<h1>STM32-OTA</h1><h2>Version 1.0 by <a style=\"color:white\" href=\"https://github.com/csnol/1CHIP-Programmers\">CSNOL<br><br><a style=\"color:white\" href=\"/up\">Upload STM32 BinFile </a><br><br><a style=\"color:white\" href=\"/list\">List STM32 BinFile</a></h2>";
    //   server.send(200, "text/html", makePage("Start Page", starthtml + "- Init MCU -<br> " + stringtmp));
    // });

    server.on("/status", []() {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");
      result += GetValveStatus();
      result += GetBottom(); 

      server.send(200, "text/html", result);
    });


    server.on("/log", []() {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");
      result += FPSTR(on_log);
      result += GetBottom(); 

      server.send(200, "text/html", result);
    });

    server.on("/getLogData", []() {
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

      server.send(200, "text/html", data);
    });

    server.on("/command", []() {
      //if (m_commandCallback != NULL) {
        String command = server.arg("cmd");
        logger.println("Command from frontend: '" + command + "'");
        if(command.startsWith("stm ")) {
          UART_STM32.println(command.substring(4));
          
          UART_DBG.print("send to stm:");
          UART_DBG.println(command.substring(4));
        }
        //m_commandCallback(command);
        server.send(200, "text/html", "OK");
      //}
    });

    server.on("/credits", []() {

      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");            
      result += "VdMot Controller was created with help and by borrowings from:<br><ul>";
      result += "<li>parts of WebFrontend: https://wiki.fhem.de/wiki/LaCrosseGateway_V1.x</li>";
      result += "<li>ideas for STM32 flash support: https://github.com/csnol/1CHIP-Programmers</li>";
      result += "</ul>";
      result += GetBottom();

      server.send(200, "text/html", makePage("VdMot Controller Credits Page", result));
    });

    server.on("/", []() {
      
      String starthtml = "<h1>VdMot Controller</h1><h2>Version ";
      starthtml += MAJORVERSION "." MINORVERSION;
      starthtml += "</h2><br>";
      starthtml += GetNavigation();
      server.send(200, "text/html", makePage("VdMot Controller Start Page", starthtml));
    });
    

    Serial.println("Setup webserver finished");

    server.begin();
}

String makePage(String title, String contents) {
  Serial.println("Make Page");
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title >";
  s += title;
  s += "</title></head><body text=#ffffff bgcolor=#0DCBDB align=\"center\">";
  s += contents;
  s += "</body></html>";
  return s;
}


String GetTop() {
  String result;
  result += F("<!DOCTYPE HTML><html>");
  result += F("<meta charset='utf-8'/>");
  result += "<head><title>";
  result += "VdMot Controller";
  result += "</title></head><body text=#ffffff bgcolor=#0DCBDB align=\"center\">";
  result += F("<p>VdMot Controller");
  result += "&nbsp;&nbsp;&nbsp;";
  result += F("</p>");
  return result;
}

String GetBottom() {
  String result;
  result += F("</body></html>");
  return result;
}


String GetNavigation() {
  String result = "";
  result += F("<a href='/'>Home</a>&nbsp;&nbsp;");
  result += F("<a href='status'>Status</a>&nbsp;&nbsp;");
  result += F("<a href='log'>Log</a>&nbsp;&nbsp;");
  result += F("<a href='credits'>Credits</a>&nbsp;&nbsp;");
  result += F("<br>");
  
  return result;
}

String GetValveStatus() {
  String result = "";
  uint8_t x;
  int temperature;

  for (x=0;x<ACTUATOR_COUNT;x++) {
    result += "Valve " + String(x + 1) + ": ";
    result += "pos: " + String(actuators[x].actual_position) + " %; ";
    result += "mean cur: " + String(actuators[x].meancurrent) + " mA; ";
    temperature = actuators[x].temperature;
    result += "temperature: " + String(temperature/10) + "." + String(temperature%10) + " °C";
    result += "<br>";
  }  
  
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
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
#include "VdmSystem.h"
#include "VdmNet.h"
#include "Messenger.h"
#include "esp_spi_flash.h" 
#include "helper.h"
#include <esp_task_wdt.h>

#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LittleFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif 

String ResetReason[] =  {
    "UNKNOWN",    //!< Reset reason can not be determined
    "POWERON",    //!< Reset due to power-on event
    "EXT",        //!< Reset by external pin (not applicable for ESP32)
    "SW",         //!< Software reset via esp_restart
    "PANIC",      //!< Software reset due to exception/panic
    "INT_WDT",    //!< Reset (software or hardware) due to interrupt watchdog
    "TASK_WDT",   //!< Reset due to task watchdog
    "WDT",        //!< Reset due to other watchdogs
    "DEEPSLEEP",  //!< Reset after exiting deep sleep mode
    "BROWNOUT",   //!< Brownout reset (software or hardware)
    "SDIO"       //!< Reset over SDIO
};


CVdmSystem VdmSystem;

CVdmSystem::CVdmSystem()
{
  spiffsStarted=false;
  numfiles  = 0;
  stmBuild = 0;
  systemMessage="";
  systemState = systemStateOK;
  getFSInProgress = false;
}

void CVdmSystem::getSystemInfo()
{   
    esp_chip_info(&chip_info);      
}

String CVdmSystem::getChipModel()
{  
  getSystemInfo();
  switch (chip_info.model) {
      case 1:
        return "ESP32";
      case 2:
        return "ESP32-S2";
      case 9:
        return "ESP32-S3";
      case 5:
        return "ESP32-C3";
      case 6:
        return "ESP32-H2";
      default:
        return "ESP32 (Unknown)";
  }
}

String CVdmSystem::localTime() {
  struct tm timeinfo;
  char buf[50];
  String sTime="Failed to obtain time";
  String upTime;
  String sLastCalib;
  if (VdmNet.sntpActive) {
    if(getLocalTime(&timeinfo)) {
      strftime (buf, sizeof(buf), "%A, %B %d.%Y %H:%M:%S", &timeinfo);
      sTime = String(buf);
    }
  }
  return (sTime);
}

bool CVdmSystem::getLocalTime(struct tm * info)
{
    uint32_t start = millis();
    uint32_t ms=10;
    time_t now;
   // while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            return true;
        }
   //     delay(10);
   // }
    return false;
}


String CVdmSystem::getUpTime() {
    char buf[50];
    int64_t upTimeUS = esp_timer_get_time(); // in microseconds
    int64_t seconds = upTimeUS/1000000;
    uint32_t days = (uint32_t)seconds/86400;
    uint32_t hr=(uint32_t)seconds % 86400 /  3600;
    uint32_t min=(uint32_t)seconds %  3600 / 60;
    uint32_t sec=(uint32_t)seconds % 60;
    snprintf (buf,sizeof(buf),"%dd %d:%02d:%02d", days, hr, min, sec);
    return String(buf);
}


void CVdmSystem::sendResetReason() {
  String ResetMsg = systemMsgReset+':'+getLastResetReason();
  VdmSystem.setSystemState(systemStateInfo,ResetMsg);
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
      syslog.log(LOG_DEBUG, ResetMsg);
  }   
  if (VdmConfig.configFlash.messengerConfig.reason.reasonFlags.reset) {
    uint8_t rr=esp_reset_reason();
    if (rr>10) rr=0;
    if ((rr>=4) && (rr<=7) || (rr==9)) {
      if (VdmConfig.configFlash.messengerConfig.activeFlags.pushOver) {
        Messenger.sendMessage(systemMsgReset,getLastResetReason().c_str());
      }
    }
  }
}

void CVdmSystem::getFSDirectory() 
{
  getFSInProgress = true;
  if (!spiffsStarted) SPIFFS.begin(true);
  spiffsStarted=true;
  numfiles  = 0; // Reset number of FS files counter
  File root = SPIFFS.open("/");
  if (root) {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file) { // Now get all the filenames, file types and sizes
      Filenames[numfiles].filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
      Filenames[numfiles].ftype    = (file.isDirectory() ? "Dir" : "File");
      Filenames[numfiles].fsize    = ConvBinUnits(file.size(), 1);
      #ifdef EnvDevelop
        UART_DBG.print("get file : ");
        UART_DBG.println(file.name());
      #endif
      file = root.openNextFile();
      numfiles++;
      if (numfiles>maxFiles) break;
    }
    root.close();
  }
  getFSInProgress = false;
}

void CVdmSystem::clearFS() 
{
  if (!spiffsStarted) SPIFFS.begin(true);
  spiffsStarted=true;
  //esp_task_wdt_init(30, false);
  bool formatSuccess = SPIFFS.format();
  #ifdef EnvDevelop
    UART_DBG.print("Format success: ");
    UART_DBG.println(formatSuccess);
  #endif
  getFSDirectory();
}

void CVdmSystem::fileDelete (String fileName)
{  
  if (!spiffsStarted) SPIFFS.begin(true);
  spiffsStarted=true;
  String thisFileName=fileName;
  if (!fileName.startsWith("/")) thisFileName="/"+fileName;
  SPIFFS.remove(thisFileName);
}

void CVdmSystem::setSystemState(uint8_t thisSystemState,String thisSystemMsg)
{
  systemState=thisSystemState; 
  systemMessage= String(thisSystemMsg);
  
  UART_DBG.print("SystemMsg: ");
  UART_DBG.println(systemMessage);
}

String CVdmSystem::getLastResetReason()
{
  uint8_t rr=esp_reset_reason();
  if (rr>10) rr=0;
  return (ResetReason[rr]);
}
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


#include "VdmSystem.h"
#include <SPIFFS.h> 
#include "esp_spi_flash.h" 
#include "helper.h"
#include <esp_task_wdt.h>

CVdmSystem VdmSystem;

CVdmSystem::CVdmSystem()
{
  spiffsStarted=false;
  numfiles  = 0;
  stmBuild = 0;
  memset (systemMessage,0,sizeof(systemMessage));
  systemState = systemStateOK;
  getFSInProgress = false;
}

void CVdmSystem::getSystemInfo()
{   
    esp_chip_info(&chip_info);      
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
      UART_DBG.print("get file : ");
      UART_DBG.println(file.name());
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

  UART_DBG.print("Format success: ");
  UART_DBG.println(formatSuccess);
  getFSDirectory();

  //ESP.restart();
}

void CVdmSystem::fileDelete (String fileName)
{  
  if (!spiffsStarted) SPIFFS.begin(true);
  spiffsStarted=true;
  String thisFileName=fileName;
  if (!fileName.startsWith("/")) thisFileName="/"+fileName;
  SPIFFS.remove(thisFileName);
}

void CVdmSystem::setSystemState(uint8_t thisSystemState,char const *thisSystemMsg)
{
  systemState=thisSystemState; 
  strncpy (systemMessage,thisSystemMsg,sizeof(systemMessage));
  UART_DBG.print("SystemMsg: ");
  UART_DBG.println(systemMessage);
}
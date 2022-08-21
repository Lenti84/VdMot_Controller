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

}

void CVdmSystem::getSystemInfo()
{   
    esp_chip_info(&chip_info);      
}

void CVdmSystem::getFSDirectory() 
{
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
      file = root.openNextFile();
      numfiles++;
      if (numfiles>maxFiles) break;
    }
    root.close();
  }
}

void CVdmSystem::clearFS() 
{
  if (!spiffsStarted) SPIFFS.begin(true);
  spiffsStarted=true;
  File root = SPIFFS.open("/");
  if (root) {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file) {
      SPIFFS.remove(String(file.name()));
      file = root.openNextFile();
    }
    root.close();
  }
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
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



#pragma once

#include "globals.h"
#include "VdmConfig.h" 
#include "esp_system.h"
#include "helper.h"
#include <FS.h>
#include <CRC32.h>
#include <LittleFS.h>

#define systemMsgSTMReset   "stm not working. reset stm"
#define systemMsgReset      "reset system by "
#define FS_READ_MODE        'r'
#define FS_WRITE_MODE       'w'

#define systemStateOK       0
#define systemStateInfo     1 
#define systemStateError    2 

#define  maxFiles  100

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

class CVdmSystem
{
public:
  CVdmSystem();
  void getSystemInfo();
  String getChipModel();
  void setGetFS();
  void getFSDirectory();
  void fileDelete(String fName);
  void setClearFS();
  void clearFS();
  void setSystemState(uint8_t thisSystemState,String thisSystemMsg);
  String getUpTime();
  String localTime();
  bool getLocalTime(struct tm * info);
  String getLastResetReason();
  void sendResetReason();
  void openFile (String fName,char mode);
  void writelnToFile (String line);
  String readlnFromFile ();
  int fileAvailable ();
  void closeFile ();

  esp_chip_info_t chip_info;
  fileinfo Filenames[maxFiles]; // Enough for most purposes!
  uint8_t numfiles;
  String stmVersion;
  uint32_t stmNRevision;
  uint32_t stmMinRequired;
  bool stmVersionFalse;
  uint32_t stmID;
  time_t stmBuild;
  uint8_t systemState;
  String systemMessage;
  bool getFSInProgress;
  uint32_t stackSize;

  
private:
  bool spiffsStarted;
  File dir;
  File fsfile;
};

extern CVdmSystem VdmSystem;

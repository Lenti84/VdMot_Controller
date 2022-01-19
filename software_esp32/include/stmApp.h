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

#pragma once

typedef struct  {
	unsigned char actual_position;      // from controller
  unsigned char target_position;      // to controller
  unsigned char state;                // from controller
  unsigned int  meancurrent;          // from controller
  uint8_t       tIdx1;          // Index of temp1 in temp list
  int           temp1;          // temperature of assigned sensor
  uint8_t       tIdx2;          // Index of temp2 in temp list
  int           temp2;          // temperature of assigned sensor
} ACTUATOR_STRUC;

typedef struct  {
  int16_t   temperature;          // temperature of assigned sensor
  char id[25];
} TEMP_STRUC;

typedef struct  {
  char id[25];
} TEMPID_STRUC;


#define APP_PRE_SETTXENA			      "stxen"   // not used in stm
#define APP_PRE_GETSUPPLYSENS		    "gspst"   // not used in stm
#define APP_PRE_SETSENSORINDEX		  "stsnx"   // not used in stm


#define APP_PRE_SETTARGETPOS       	"stgtp"			
#define APP_PRE_GETONEWIRECNT		    "gonec"			
#define APP_PRE_GETONEWIREDATA		  "goned"	
#define APP_PRE_SETONEWIRESEARCH    "stons"		
#define APP_PRE_SET1STSENSORINDEX	  "stsnx"   // not used	in stm		
#define APP_PRE_SET2NDSENSORINDEX	  "stsny"		// not used	in stm	
#define APP_PRE_SETALLVLVOPEN		    "staop"			
#define APP_PRE_GETONEWIRESETT		  "gvlon"	  // not used	in stm	
#define APP_PRE_GETVLVDATA			    "gvlvd"			
#define APP_PRE_SETVLLEARN          "staln"   // one valve

#define APP_PRE_GETVERSION			    "gvers"
#define APP_PRE_GETTARGETPOS       	"gtgtp"			

#define APP_PRE_SETMOTCHARS         "smotc"
#define APP_PRE_GETMOTCHARS         "gmotc"

#define APP_PRE_GETACTUALPOS       	"gactp"   // not used in stm
#define APP_PRE_GETMEANCURR        	"gmenc"   // not used in stm
#define APP_PRE_GETSTATUS          	"gstat"   // not used in stm

#define VLV_STATE_IDLE      0x01 // nothing to do
#define VLV_STATE_OPENING   0x02 // opens
#define VLV_STATE_CLOSING   0x03 // closes
#define VLV_STATE_BLOCKS    0x04 // valve is blocked
#define VLV_STATE_UNKNOWN   0x05 // initial state
#define VLV_STATE_OPENCIR   0x06 // open circuit detected
#define VLV_STATE_FULLOPEN  0x07 // go directly full open

#define COMM_ALIVE_CYCLE      30        // send "Alive" cycle in 100 ms cycles

enum COMM_STATE {COMM_IDLE,COMM_ALIVE,COMM_SENDTARGET,COMM_CHECKTARGET,COMM_GETDATA,
                  COMM_GETONEWIRE,COMM_GETONEWIRECOUNT,COMM_HANDLEQUEUE};


class CStmApp
{
public:
  CStmApp();
  void app_setup();
  void app_loop();
  void app_cmd(String command, String args="");
  bool checkCmdIsAvailable (String thisCmd);
  void valvesCalibration();
  void valvesAssembly();
  void scanTemps();

  ACTUATOR_STRUC actuators[ACTUATOR_COUNT];
  TEMP_STRUC temps[TEMP_SENSORS_COUNT];
  TEMPID_STRUC tempsId[TEMP_SENSORS_COUNT];
  uint8_t tempsCount;
private:
  void app_check_data();
  void app_comm_machine();
  void app_alive_check();
  void app_web_cmd_check();
  int8_t findTempID(char* ID);
  
  uint16_t stm32alive;
  bool settarget_check;
  uint8_t target_position_mirror[ACTUATOR_COUNT];

  COMM_STATE commstate;    
  uint8_t tempIndex;
  uint8_t checkTempsCount;
  uint8_t tempsPrivCount;
  String cmd_buffer;

  char buffer[1200];
  char *bufptr;
  uint16_t buflen;
  int16_t availcnt;
  bool found;
  
  char*   cmdptr;
  char*	  cmdptrend;
  char    cmd[50];
  char		arg0[1200];	
  char    arg1[20];		
  char    arg2[20];
  char    arg3[20];
  char    arg4[20];
	char*		arg0ptr;
	char*		arg1ptr;
  char*		arg2ptr;
  char*		arg3ptr;
  char*		arg4ptr;
	uint8_t	argcnt;
};

extern CStmApp StmApp;


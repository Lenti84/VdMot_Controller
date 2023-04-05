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

#include <Arduino.h>
#include "VdmNet.h"
#include "VdmConfig.h"
#include "globals.h"
#include "stmApp.h"

class CWeb
{
public:
  CWeb();
  void handleFileUpload();
  void handleFlash();
  void handleFileDelete();
  void handleListFiles();
  String makePage(String title, String contents);
  void postValvesPos();
  String getValvesStatus();
  String getTempsStatus(VDM_TEMPS_CONFIG tempsConfig);
  String getNetInfo(VDM_NETWORK_INFO networkInfo); 
  String getNetConfig (VDM_NETWORK_CONFIG netConfig);
  String getProtConfig (VDM_PROTOCOL_CONFIG protConfig);
  String getValvesConfig (VDM_VALVES_CONFIG valvesConfig);
  String getMotorConfig (MOTOR_CHARS motorConfig);
  String getValvesControlConfig (VDM_VALVES_CONTROL_CONFIG valvesControlConfig);
  String getTempsConfig (VDM_TEMPS_CONFIG tempsConfig);
  String getTempSensorsID();
  String getSysDynInfo();
  String getSysInfo();
  String getSysConfig (VDM_SYSTEM_CONFIG sysConfig);
  String getMsgConfig (VDM_MSG_CONFIG msgConfig);
  String getFSDir();
  String getStmUpdStatus();
  bool findIdInValve (uint8_t idx);
  bool getControlActive();
};

extern CWeb Web;


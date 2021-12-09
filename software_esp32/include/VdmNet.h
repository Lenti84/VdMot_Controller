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

#include <stdint.h>
#include <ArduinoJson.h>
#include "VdmConfig.h"

#define noneProtocol 0
#define mqttProtocol 1

#define interfaceAuto 0
#define interfaceEth  1
#define interfaceWifi 2

#define currentInterfaceIsEth  0
#define currentInterfaceIsWifi 1



enum TWifiState {wifiIdle,wifiIsStarting,wifiConnected,wifiDisabled};
enum TEthState  {ethIdle,ethIsStarting,ethConnected,ethDisabled};

class CVdmNet
{
public:
  CVdmNet();
  void init();
  void setup();
  void setupEth();
  void setupWifi();
  void initServer();
  void setupNtp();
  void valvesCalib();
  void checkNet();
  void startBroker();
  void mqttBroker();
  

  void postSetValve (JsonObject doc);
  TWifiState wifiState;
  TEthState ethState;
  bool serverIsStarted;
  bool dataBrokerIsStarted;
  uint8_t interfaceType;
};

extern CVdmNet VdmNet;




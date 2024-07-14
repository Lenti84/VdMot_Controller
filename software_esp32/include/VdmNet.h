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

#include <Pushover.h>


#define USING_CORE_ESP32_CORE_V200_PLUS     true
// Debug Level from 0 to 4
#define _ETHERNET_WEBSERVER_LOGLEVEL_       3

#define ETH_ADDR        1
#define ETH_POWER_PIN   16
#define ETH_POWER_PIN_ALTERNATIVE 16
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO0_IN

#include <stdint.h>
#include <ArduinoJson.h>
#include "VdmConfig.h"
#include <Syslog.h>
#include <AsyncWebServer_WT32_ETH01.h>

#define noneProtocol 0
#define mqttProtocol 1

#define interfaceAuto 0
#define interfaceEth  1
#define interfaceWifi 2

#define currentInterfaceIsEth  0
#define currentInterfaceIsWifi 1



enum TWifiState {wifiIdle,wifiIsStarting,wifiConnected,wifiDisabled};
enum TEthState  {ethIdle,ethIsStarting,ethConnected,ethDisabled};


typedef struct {
  uint8_t interfaceType;
  bool dhcpEnabled;
  IPAddress ip;
  IPAddress mask;
  IPAddress gateway;
  IPAddress dnsIp; 
  String mac;       
} VDM_NETWORK_INFO;



class CVdmNet
{
public:
  CVdmNet();
  void init();
  void setup();
  void setupEth();
  void setupWifi();
  void setupNtp();
  void checkNet();
  void checkWifi();
  void startBroker();
  void mqttBroker();
  void startSysLog();
  bool checkSntpReachable();
   
  TWifiState wifiState;
  TEthState ethState;
  VDM_NETWORK_INFO networkInfo;
  bool serverIsStarted;
  bool dataBrokerIsStarted;
  struct tm startTimeinfo;
  bool syslogStarted;
  bool sntpActive;
  bool sntpReachable;
private :
};

extern CVdmNet VdmNet;

extern Syslog syslog;


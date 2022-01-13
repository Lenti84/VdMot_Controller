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
#include <Preferences.h>
#include <ArduinoJson.h>

enum protType  {protTypeNone=0,protTypeMqtt=1};

typedef struct {
  uint8_t eth_wifi;
  bool dhcpEnabled;
  uint32_t staticIp;
  uint32_t mask;
  uint32_t gateway;
  uint32_t dnsIp;          
  char ssid[65];
  char pwd[65];
  char userName[65];
  char userPwd[65];
  char timeServer[65];
  long timeOffset;
  int daylightOffset; 
  uint8_t syslogLevel;
  uint32_t syslogIp;
  uint32_t syslogPort;

} VDM_NETWORK_CONFIG;

typedef struct {
  uint8_t  dataProtocol; // 0 = no protocol , 1 = mqtt
  uint32_t brokerIp;
  uint16_t brokerPort;
  uint32_t brokerInterval;
  char userName[65];
  char userPwd[65];
} VDM_PROTOCOL_CONFIG;

typedef struct {
  char  name[11];
  bool active;
} VDM_VALVE_CONFIG;

typedef struct {
  VDM_VALVE_CONFIG valveConfig[ACTUATOR_COUNT];
  uint8_t dayOfCalib;
  uint8_t hourOfCalib;
} VDM_VALVES_CONFIG;

typedef struct {
  char name[11];
  bool active;
  int offset;
  char ID[25];
} VDM_TEMP_CONFIG;

typedef struct {
  VDM_TEMP_CONFIG tempConfig[TEMP_SENSORS_COUNT];
} VDM_TEMPS_CONFIG;

typedef struct 
{
  VDM_NETWORK_CONFIG netConfig;
  VDM_PROTOCOL_CONFIG protConfig;
  VDM_VALVES_CONFIG valvesConfig;
  VDM_TEMPS_CONFIG tempsConfig;
} CONFIG_FLASH;


#define nvsNetCfg         "netCfg"
#define nvsNetEthwifi     "ethwifi"
#define nvsNetDhcp        "dhcp"
#define nvsNetStaticIp    "staticIp"
#define nvsNetMask        "mask"
#define nvsNetGW          "gw"
#define nvsNetDnsIp       "dnsIp"       
#define nvsNetSsid        "ssid"
#define nvsNetPwd         "pwd"
#define nvsNetUserName    "userName"
#define nvsNetUserPwd     "userPwd"
#define nvsNetTimeServer  "timeServer"
#define nvsNetTimeOffset  "timeOffset"
#define nvsNetDayLightOffset "dstOffset"
#define nvsNetSysLogEnable  "syslogEnable"
#define nvsNetSysLogIp      "sysLogIp"
#define nvsNetSysLogPort    "sysLogPort"

#define nvsProtCfg        "protCfg"
#define nvsProtDataProt   "dataProt"
#define nvsProtBrokerIp   "brokerIp"
#define nvsProtBrokerPort "brokerPort"
#define nvsProtBrokerInterval "brokerInterval"
#define nvsProtBrokerUser "brokerUser"
#define nvsProtBrokerPwd  "brokerPwd"

#define nvsValvesCfg      "valvesCfg"
#define nvsValves         "valves"

#define nvsTempsCfg      "tempsCfg"
#define nvsTemps         "temps"
#define nvsDayOfCalib    "dayOfCalib"
#define nvsHourOfCalib   "hourOfCalib"


class CVdmConfig
{
public:
  CVdmConfig();
  void init();
  void setDefault();
  void clearConfig();
  void readConfig();
  void writeConfig(bool reboot=false);
  void resetConfig (bool reboot=false);
  void restoreConfig (bool reboot=false);
  void postNetCfg (JsonObject doc);
  void postProtCfg (JsonObject doc);
  void postValvesCfg (JsonObject doc);
  void postTempsCfg (JsonObject doc);
  String handleAuth (JsonObject doc);
  uint32_t doc2IPAddress(String id);

  Preferences prefs;

  CONFIG_FLASH configFlash;
};

extern CVdmConfig VdmConfig;

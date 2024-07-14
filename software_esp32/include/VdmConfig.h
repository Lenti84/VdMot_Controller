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

#define pinSetFactoryCfg  2

enum protType  {protTypeNone=0,protTypeMqtt=1};

#define allowHeatingCooling 0
#define allowHeating 1
#define allowCooling 2

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
  char tz[50];
  char tzCode[50];
} VDM_SYSTEM_TIME_ZONE_CONFIG;


typedef struct {
  uint8_t celsiusFahrenheit;
  char stationName[21];
} VDM_SYSTEM_CONFIG;

typedef struct  {
  uint8_t publishSeparate : 1 ;
  uint8_t publishAllTemps : 1;
  uint8_t publishPathAsRoot : 1 ;
  uint8_t publishUpTime : 1;
  uint8_t publishOnChange : 1;
  uint8_t publishRetained : 1;
  uint8_t publishPlainText : 1;
  uint8_t publishDiag : 1;
} VDM_PROTOCOL_CONFIG_FLAGS;

typedef struct  {
  uint8_t timeoutTSActive : 1 ;
  uint8_t timeoutDSActive : 1 ;
} VDM_PROTOCOL_MQTT_CONFIG_FLAGS;


typedef struct  {
  VDM_PROTOCOL_MQTT_CONFIG_FLAGS flags;
  uint32_t timeOut;
  uint8_t toPos;
} VDM_PROTOCOL_MQTT_CONFIG;


typedef struct {
  uint8_t  dataProtocol; // 0 = no protocol , 1 = mqtt
  uint32_t brokerIp;
  uint16_t brokerPort;
  uint32_t brokerInterval;
  uint32_t publishInterval;
  char userName[65];
  char userPwd[65];
  VDM_PROTOCOL_CONFIG_FLAGS protocolFlags;
  uint16_t keepAliveTime;
  uint32_t minBrokerDelay;
  VDM_PROTOCOL_MQTT_CONFIG mqttConfig;
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

typedef struct  {
  uint8_t active : 1 ;
  uint8_t allow : 3;
  uint8_t windowInstalled : 1;
} VDM_VALVE_CONTROL_CONFIG_FLAGS;


typedef struct {
  VDM_VALVE_CONTROL_CONFIG_FLAGS controlFlags;
  uint8_t link;
  uint8_t valueSource;
  uint8_t targetSource;
  uint16_t xp;
  int8_t offset;
  uint16_t ti;
  uint16_t ts;
  float ki;
  uint8_t scheme;
  uint8_t startActiveZone;
  uint8_t endActiveZone;
} VDM_VALVE_CONTROL_CONFIG;

typedef struct {
  float alpha;
} VDM_VALVE_CONTROL_CONFIG_ALPHA;

typedef struct {
  uint8_t usePrediction : 1; 
} VDM_VALVES_CONTROL_CONFIG_FLAGS;

typedef struct {
  VDM_VALVE_CONTROL_CONFIG valveControlConfig[ACTUATOR_COUNT];
  uint8_t heatControl;
  uint8_t parkingPosition;
} VDM_VALVES_CONTROL_CONFIG;


typedef struct {
  char name[11];
  bool active;
  int offset;
  char ID[25];
} VDM_TEMP_CONFIG;

typedef struct {
  VDM_TEMP_CONFIG tempConfig[TEMP_SENSORS_COUNT];
} VDM_TEMPS_CONFIG;

typedef struct  {
  uint8_t pushOver : 1 ;
  uint8_t email : 1 ;
} VDM_MSG_ACTIVE_CONFIG_FLAGS;

typedef struct  {
  uint8_t valveFailed : 1 ;
  uint8_t notDetect : 1;
  uint8_t mqttTimeOut : 1;
  uint8_t reset : 1;
  uint8_t ds18Failed : 1;
  uint8_t tValueFailed : 1;
  uint8_t valveBlocked : 1;
} VDM_MSG_REASON_CONFIG_FLAGS;

typedef struct  {
  VDM_MSG_REASON_CONFIG_FLAGS reasonFlags;
  uint16_t mqttTimeOutTime;
} VDM_MSG_REASON_CONFIG;

typedef struct {
  // pushover
  char userToken[31];
  char appToken[31];
  char title[31];
} VDM_MSG_PUSHOVER_CONFIG;

typedef struct {
  // email
  char user[65];
  char pwd[65];
  char host[65]; 
  uint16_t port;
  char recipient[65];
  char title[31];
} VDM_MSG_EMAIL_CONFIG;

typedef struct {
  VDM_MSG_ACTIVE_CONFIG_FLAGS activeFlags;
  VDM_MSG_REASON_CONFIG reason;
  VDM_MSG_PUSHOVER_CONFIG pushover;
  VDM_MSG_EMAIL_CONFIG email;
} VDM_MSG_CONFIG;

typedef struct 
{
  VDM_NETWORK_CONFIG netConfig;
  VDM_PROTOCOL_CONFIG protConfig;
  VDM_VALVES_CONFIG valvesConfig;
  VDM_TEMPS_CONFIG tempsConfig;
  VDM_SYSTEM_CONFIG systemConfig;
  VDM_VALVES_CONTROL_CONFIG valvesControlConfig;
  VDM_SYSTEM_TIME_ZONE_CONFIG timeZoneConfig;
  VDM_MSG_CONFIG messengerConfig;
} CONFIG_FLASH;

typedef struct {
  time_t lastCalib; 
} MISC_VALUES;

typedef struct {
  uint8_t heatControl;
  uint8_t parkPosition; 
} HEATCFG_VALUES;

#define nvsNetCfg                   "netCfg"
#define nvsNetEthwifi               "ethwifi"
#define nvsNetDhcp                  "dhcp"
#define nvsNetStaticIp              "staticIp"
#define nvsNetMask                  "mask"
#define nvsNetGW                    "gw"
#define nvsNetDnsIp                 "dnsIp"       
#define nvsNetSsid                  "ssid"
#define nvsNetPwd                   "pwd"
#define nvsNetUserName              "userName"
#define nvsNetUserPwd               "userPwd"
#define nvsNetTimeServer            "timeServer"
#define nvsNetTimeOffset            "timeOffset"
#define nvsNetDayLightOffset        "dstOffset"
#define nvsNetSysLogEnable          "syslogEnable"
#define nvsNetSysLogIp              "sysLogIp"
#define nvsNetSysLogPort            "sysLogPort"

#define nvsTZCfg                    "tZCfg"
#define nvsTZ                       "tZ"
#define nvsTZCode                   "tZCode"

#define nvsProtCfg                  "protCfg"
#define nvsProtDataProt             "dataProt"
#define nvsProtBrokerIp             "brokerIp"
#define nvsProtBrokerPort           "brokerPort"
#define nvsProtBrokerInterval       "brokerInterval"
#define nvsProtPublishInterval      "publishInterval"
#define nvsProtBrokerUser           "brokerUser"
#define nvsProtBrokerPwd            "brokerPwd"
#define nvsProtBrokerPublishFlags   "brokerPF"
#define nvsProtBrokerKeepAliveTime  "brokerKAT"
#define nvsProtBrokerMinBrokerDelay "brokerMD"
#define nvsProtBrokerMQTTFlags      "brokerMQF"
#define nvsProtBrokerMQTTTimeOut    "brokerMQTO"
#define nvsProtBrokerMQTTToPos      "brokerMQToPos"

#define nvsValvesCfg                "valvesCfg"
#define nvsValvesControlCfg         "valvesCtrlCfg"
#define nvsValves                   "valves"
#define nvsValvesControl            "valvesCtrl"
#define nvsValvesControlAlpha       "vCtrlAlpha"
#define nvsValvesControlHeatControl "vCtrlHeat"
#define nvsValvesControlParkPos     "vCtrlParkPos"
#define nvsValvesControlFlags       "vCtrlFlags"

#define nvsTempsCfg                 "tempsCfg"
#define nvsTemps                    "temps"

#define nvsDayOfCalib               "dayOfCalib"
#define nvsHourOfCalib              "hourOfCalib"
#define nvsMovCalib                 "movCalib"

#define nvsMotorCfg                 "motorCfg"
#define nvsMotorMinC                "motorMinC"
#define nvsMotorMaxC                "motorMaxC"

#define nvsSystemCfg                "sysCfg"
#define nvsSystemCelsiusFahrenheit  "CF"
#define nvsSystemStationName        "stName"

#define nvsMsgCfg                   "msgCfg"
#define nvsMsgCfgFlags              "msgFlags"
#define nvsMsgCfgReason             "msgReas"
#define nvsMsgCfgMqttTimeout        "msgMQTO"
#define nvsMsgCfgPOAppToken         "POAppTk"
#define nvsMsgCfgPOUserToken        "POUserTk"
#define nvsMsgCfgPOTitle            "POTitle"

#define nvsMsgCfgEmailUser          "EUser"
#define nvsMsgCfgEmailPwd           "EPwd"
#define nvsMsgCfgEMailHost          "EHost"
#define nvsMsgCfgEmailPort          "EPort"
#define nvsMsgCfgEmailRecipient     "ERep"
#define nvsMsgCfgEmailTitle         "ETitle"

#define nvsMisc                     "Misc"
#define nvsMiscLastCalib            "MiscLC"

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
  void writeValvesControlConfig(bool reboot=false, bool restartTask=true);
  void writeSysLogValues();
  void postNetCfg (JsonObject doc);
  void postSysLogCfg (JsonObject doc);
  void postProtCfg (JsonObject doc);
  void postValvesCfg (JsonObject doc);
  void postValvesControlCfg (JsonObject doc);
  void postTempsCfg (JsonObject doc);
  void postSysCfg (JsonObject doc);
  void postMessengerCfg (JsonObject doc);
  String handleAuth (JsonObject doc);
  uint32_t doc2IPAddress(String id);
  void checkToResetCfg();
  void writeMiscValues();

  Preferences prefs;

  CONFIG_FLASH configFlash;

  MISC_VALUES miscValues;
  HEATCFG_VALUES heatValues;
};

extern CVdmConfig VdmConfig;

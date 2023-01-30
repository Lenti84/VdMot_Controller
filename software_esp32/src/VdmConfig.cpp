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


#include <stdint.h>
#include "VdmConfig.h"
#include "VdmTask.h"
#include "web.h"
#include "Services.h"
#include "stmApp.h"

CVdmConfig VdmConfig;


CVdmConfig::CVdmConfig()
{
  
}

void CVdmConfig::init()
{
  setDefault();
  readConfig();
}

void CVdmConfig::resetConfig (bool reboot)
{  
  clearConfig(); 
  writeConfig();
  writeValvesControlConfig();
  if (reboot) Services.restartSystem(hard,false);
}

void CVdmConfig::restoreConfig (bool reboot)
{  
  setDefault();
  writeConfig();
  writeValvesControlConfig();
  if (reboot) Services.restartSystem(hard,false);
}

void CVdmConfig::setDefault()
{
  configFlash.netConfig.eth_wifi = 0;	      // 0 = auto (default)
  configFlash.netConfig.dhcpEnabled = 1;    // 0 = no DHCP, 1 = use DHCP
  configFlash.netConfig.staticIp = 0;
  configFlash.netConfig.mask = 0;
  configFlash.netConfig.gateway = 0;
  configFlash.netConfig.dnsIp = 0;
  memset (configFlash.netConfig.ssid,0,sizeof(configFlash.netConfig.ssid));
  memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.pwd));
  memset (configFlash.netConfig.userName,0,sizeof(configFlash.netConfig.userName));
  memset (configFlash.netConfig.userPwd,0,sizeof(configFlash.netConfig.userPwd));
  memset (configFlash.timeZoneConfig.tz,0,sizeof(configFlash.timeZoneConfig.tz));
  memset (configFlash.timeZoneConfig.tzCode,0,sizeof(configFlash.timeZoneConfig.tzCode));
  clearConfig();
}

void CVdmConfig::clearConfig()
{ 
  configFlash.protConfig.dataProtocol = 0;
  configFlash.protConfig.brokerIp = 0;
  configFlash.protConfig.brokerPort = 0;
  configFlash.protConfig.brokerInterval = 2000;
  memset (configFlash.protConfig.userName,0,sizeof(configFlash.protConfig.userName));
  memset (configFlash.protConfig.userPwd,0,sizeof(configFlash.protConfig.userPwd));
  configFlash.protConfig.publishTarget = false;
  configFlash.protConfig.publishAllTemps = false;
  configFlash.protConfig.publishPathAsRoot = false;
  
  for (uint8_t i=0; i<ACTUATOR_COUNT; i++) {
    configFlash.valvesConfig.valveConfig[i].active = false;
    memset (configFlash.valvesConfig.valveConfig[i].name,0,sizeof(configFlash.valvesConfig.valveConfig[i].name));
  }

  for (uint8_t i=0; i<ACTUATOR_COUNT; i++) {
    configFlash.valvesControlConfig.valveControlConfig[i].active = false;
    configFlash.valvesControlConfig.valveControlConfig[i].xp=20;
    configFlash.valvesControlConfig.valveControlConfig[i].link=0;
    configFlash.valvesControlConfig.valveControlConfig[i].offset=0;
    configFlash.valvesControlConfig.valveControlConfig[i].targetSource=0;
    configFlash.valvesControlConfig.valveControlConfig[i].valueSource=0;
    configFlash.valvesControlConfig.valveControlConfig[i].ti=3600;
    configFlash.valvesControlConfig.valveControlConfig[i].ts=900;
    configFlash.valvesControlConfig.valveControlConfig[i].scheme=0;
    configFlash.valvesControlConfig.valveControlConfig[i].ki=0.01;
    configFlash.valvesControlConfig.valveControlConfig[i].startActiveZone=0;
    configFlash.valvesControlConfig.valveControlConfig[i].endActiveZone=100;
  }
  configFlash.valvesControlConfig.heatControl=0;
  configFlash.valvesControlConfig.parkingPosition=10;

  for (uint8_t i=0; i<TEMP_SENSORS_COUNT; i++) {
    configFlash.tempsConfig.tempConfig[i].active = false;
    configFlash.tempsConfig.tempConfig[i].offset = 0;
    memset (configFlash.tempsConfig.tempConfig[i].name,0,sizeof(configFlash.tempsConfig.tempConfig[i].name));
    memset (configFlash.tempsConfig.tempConfig[i].ID,0,sizeof(configFlash.tempsConfig.tempConfig[i].ID));
  }
  configFlash.systemConfig.celsiusFahrenheit=0;
  memset (configFlash.systemConfig.stationName,0,sizeof(configFlash.systemConfig.stationName));
  strncpy(configFlash.systemConfig.stationName,DEVICE_HOSTNAME,sizeof(configFlash.systemConfig.stationName));

  configFlash.valvesConfig.dayOfCalib=9;
  configFlash.valvesConfig.hourOfCalib=0;
  memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.timeServer));
  strncpy(configFlash.netConfig.timeServer,"pool.ntp.org",sizeof(configFlash.netConfig.timeServer));
  strncpy(configFlash.timeZoneConfig.tz,"Europe/Berlin",sizeof(configFlash.timeZoneConfig.tz));
  strncpy(configFlash.timeZoneConfig.tzCode,"CET-1CEST,M3.5.0,M10.5.0/3",sizeof(configFlash.timeZoneConfig.tzCode));

  configFlash.netConfig.syslogLevel=0;
  configFlash.netConfig.syslogIp=0;
  configFlash.netConfig.syslogPort=0; 
}

void CVdmConfig::readConfig()
{
  if (prefs.begin(nvsSystemCfg,false)) {
    configFlash.systemConfig.celsiusFahrenheit=prefs.getUChar(nvsSystemCelsiusFahrenheit);
    if (prefs.isKey(nvsSystemStationName))
      prefs.getString(nvsSystemStationName,(char*) configFlash.systemConfig.stationName,sizeof(configFlash.systemConfig.stationName));
    prefs.end();
  }

  if (prefs.begin(nvsNetCfg,false)) {
    configFlash.netConfig.eth_wifi=prefs.getUChar(nvsNetEthwifi);
    configFlash.netConfig.dhcpEnabled=prefs.getUChar(nvsNetDhcp,1);
    configFlash.netConfig.staticIp=prefs.getULong(nvsNetStaticIp);
    configFlash.netConfig.mask=prefs.getULong(nvsNetMask);
    configFlash.netConfig.gateway=prefs.getULong(nvsNetGW);
    configFlash.netConfig.dnsIp=prefs.getULong(nvsNetDnsIp); 
    if (prefs.isKey(nvsNetSsid))
      prefs.getString(nvsNetSsid,(char*) configFlash.netConfig.ssid,sizeof(configFlash.netConfig.ssid));
    if (prefs.isKey(nvsNetPwd))
      prefs.getString(nvsNetPwd,(char*) configFlash.netConfig.pwd,sizeof(configFlash.netConfig.pwd));
    if (prefs.isKey(nvsNetUserName))
      prefs.getString(nvsNetUserName,(char*) configFlash.netConfig.userName,sizeof(configFlash.netConfig.userName));
    if (prefs.isKey(nvsNetUserPwd))
      prefs.getString(nvsNetUserPwd,(char*) configFlash.netConfig.userPwd,sizeof(configFlash.netConfig.userPwd));
    if (prefs.isKey(nvsNetTimeServer))
      prefs.getString(nvsNetTimeServer,(char*) configFlash.netConfig.timeServer,sizeof(configFlash.netConfig.timeServer));
    
    if (strlen(configFlash.netConfig.timeServer) == 0) {
      memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.timeServer));
      strncpy(configFlash.netConfig.timeServer,"pool.ntp.org",sizeof(configFlash.netConfig.timeServer));
    }
    
    configFlash.netConfig.syslogLevel=prefs.getUChar(nvsNetSysLogEnable);
    configFlash.netConfig.syslogIp=prefs.getULong(nvsNetSysLogIp);
    configFlash.netConfig.syslogPort=prefs.getUShort(nvsNetSysLogPort);
    prefs.end();
  }
  if (prefs.begin(nvsProtCfg,false)) {
    configFlash.protConfig.dataProtocol = prefs.getUChar(nvsProtDataProt);
    configFlash.protConfig.brokerIp = prefs.getULong(nvsProtBrokerIp);
    configFlash.protConfig.brokerPort = prefs.getUShort(nvsProtBrokerPort);
    configFlash.protConfig.brokerInterval = prefs.getULong(nvsProtBrokerInterval,2000);
    if (prefs.isKey(nvsProtBrokerUser))
      prefs.getString(nvsProtBrokerUser,(char*) configFlash.protConfig.userName,sizeof(configFlash.protConfig.userName));
    if (prefs.isKey(nvsProtBrokerPwd))
      prefs.getString(nvsProtBrokerPwd,(char*) configFlash.protConfig.userPwd,sizeof(configFlash.protConfig.userPwd));
    configFlash.protConfig.publishTarget = prefs.getUChar(nvsProtBrokerPublishTarget);
    configFlash.protConfig.publishAllTemps = prefs.getUChar(nvsProtBrokerPublishAllTemps);
    configFlash.protConfig.publishPathAsRoot = prefs.getUChar(nvsProtBrokerPublishPathAsRoot);
    prefs.end();
  }

  if (prefs.begin(nvsValvesCfg,false)) {
    if (prefs.isKey(nvsValves))
      prefs.getBytes(nvsValves, (void *) configFlash.valvesConfig.valveConfig, sizeof(configFlash.valvesConfig.valveConfig));
    configFlash.valvesConfig.dayOfCalib=prefs.getUChar(nvsDayOfCalib,9);
    configFlash.valvesConfig.hourOfCalib=prefs.getUChar(nvsHourOfCalib); 
    prefs.end();
  }

  if (prefs.begin(nvsValvesControlCfg,false)) {
    if (prefs.isKey(nvsValvesControl)) {
      prefs.getBytes(nvsValvesControl, (void *) configFlash.valvesControlConfig.valveControlConfig, sizeof(configFlash.valvesControlConfig.valveControlConfig));
      configFlash.valvesControlConfig.heatControl=prefs.getUChar (nvsValvesControlHeatControl,0);
      configFlash.valvesControlConfig.parkingPosition=prefs.getUChar (nvsValvesControlParkPos,10);
    }
    prefs.end();
  }
 
 if (prefs.begin(nvsTempsCfg,false)) {
  if (prefs.isKey(nvsTemps))
    prefs.getBytes(nvsTemps,(void *) configFlash.tempsConfig.tempConfig, sizeof(configFlash.tempsConfig.tempConfig));
  prefs.end();
 }

  if (prefs.begin(nvsTZCfg,false)) {
    if (prefs.isKey(nvsTZ))
      prefs.getString(nvsTZ,(char*) configFlash.timeZoneConfig.tz,sizeof(configFlash.timeZoneConfig.tz));
    if (prefs.isKey(nvsTZCode))
      prefs.getString(nvsTZCode,(char*) configFlash.timeZoneConfig.tzCode,sizeof(configFlash.timeZoneConfig.tzCode));
    prefs.end();
  }
}

void CVdmConfig::writeConfig(bool reboot)
{

  prefs.begin(nvsSystemCfg,false);
  prefs.clear();
  prefs.putUChar(nvsSystemCelsiusFahrenheit,configFlash.systemConfig.celsiusFahrenheit);
  prefs.putString(nvsSystemStationName,configFlash.systemConfig.stationName);
  prefs.end();
 
  prefs.begin(nvsNetCfg,false);
  prefs.clear();
  prefs.putUChar(nvsNetEthwifi,configFlash.netConfig.eth_wifi);
  prefs.putUChar(nvsNetDhcp,configFlash.netConfig.dhcpEnabled);
  prefs.putULong(nvsNetStaticIp,configFlash.netConfig.staticIp);
  prefs.putULong(nvsNetMask,configFlash.netConfig.mask);
  prefs.putULong(nvsNetGW,configFlash.netConfig.gateway);
  prefs.putULong(nvsNetDnsIp,configFlash.netConfig.dnsIp); 
  prefs.putString(nvsNetSsid,configFlash.netConfig.ssid);
  prefs.putString(nvsNetPwd,configFlash.netConfig.pwd);
  prefs.putString(nvsNetUserName,configFlash.netConfig.userName);
  prefs.putString(nvsNetUserPwd,configFlash.netConfig.userPwd);
  prefs.putString(nvsNetTimeServer,configFlash.netConfig.timeServer);
  prefs.putUChar(nvsNetSysLogEnable,configFlash.netConfig.syslogLevel);
  prefs.putULong(nvsNetSysLogIp,configFlash.netConfig.syslogIp);
  prefs.putUShort(nvsNetSysLogPort,configFlash.netConfig.syslogPort);
  prefs.end();

  prefs.begin(nvsProtCfg,false);
  prefs.clear();
  prefs.putUChar(nvsProtDataProt,configFlash.protConfig.dataProtocol);
  if (configFlash.protConfig.dataProtocol==protTypeMqtt) {
    prefs.putULong(nvsProtBrokerIp,configFlash.protConfig.brokerIp);
    prefs.putUShort(nvsProtBrokerPort,configFlash.protConfig.brokerPort);
    prefs.putULong(nvsProtBrokerInterval,configFlash.protConfig.brokerInterval);
    prefs.putString(nvsProtBrokerUser,configFlash.protConfig.userName);
    prefs.putString(nvsProtBrokerPwd,configFlash.protConfig.userPwd);
    prefs.putUChar(nvsProtBrokerPublishTarget,configFlash.protConfig.publishTarget);
    prefs.putUChar(nvsProtBrokerPublishAllTemps,configFlash.protConfig.publishAllTemps);
    prefs.putUChar(nvsProtBrokerPublishPathAsRoot,configFlash.protConfig.publishPathAsRoot);
  }
  prefs.end();
 
  prefs.begin(nvsValvesCfg,false);
  prefs.clear();
  prefs.putBytes(nvsValves, (void *) configFlash.valvesConfig.valveConfig, sizeof(configFlash.valvesConfig.valveConfig));
  prefs.putUChar(nvsDayOfCalib,configFlash.valvesConfig.dayOfCalib);
  prefs.putUChar(nvsHourOfCalib,configFlash.valvesConfig.hourOfCalib);
  prefs.end();

  prefs.begin(nvsTempsCfg,false);
  prefs.clear();
  prefs.putBytes(nvsTemps, (void *) configFlash.tempsConfig.tempConfig, sizeof(configFlash.tempsConfig.tempConfig));
  prefs.end();
  
  prefs.begin(nvsTZCfg,false);
  prefs.clear();
  prefs.putString(nvsTZ,configFlash.timeZoneConfig.tz);
  prefs.putString(nvsTZCode,configFlash.timeZoneConfig.tzCode);
  prefs.end();

  if (reboot) Services.restartSystem();
}


void CVdmConfig::writeValvesControlConfig(bool reboot)
{
  prefs.begin(nvsValvesControlCfg,false);
  prefs.clear();
  prefs.putBytes(nvsValvesControl, (void *) configFlash.valvesControlConfig.valveControlConfig, sizeof(configFlash.valvesControlConfig.valveControlConfig));
  prefs.putUChar (nvsValvesControlHeatControl,configFlash.valvesControlConfig.heatControl);
  prefs.putUChar (nvsValvesControlParkPos,configFlash.valvesControlConfig.parkingPosition);
  prefs.end();
  if (reboot) Services.restartSystem();
}

uint32_t CVdmConfig::doc2IPAddress(String id)
{
  IPAddress c1;
  bool b;
  b=c1.fromString(id) ; 
  if (b) return (uint32_t) c1; else return 0;
}

void CVdmConfig::postNetCfg (JsonObject doc)
{
  if (!doc["ethWifi"].isNull()) configFlash.netConfig.eth_wifi=doc["ethWifi"];
  if (!doc["dhcp"].isNull()) configFlash.netConfig.dhcpEnabled=doc["dhcp"];
  if (!doc["ip"].isNull()) configFlash.netConfig.staticIp=doc2IPAddress(doc["ip"]);
  if (!doc["mask"].isNull()) configFlash.netConfig.mask=doc2IPAddress(doc["mask"]);
  if (!doc["gw"].isNull()) configFlash.netConfig.gateway=doc2IPAddress(doc["gw"]);
  if (!doc["dns"].isNull()) configFlash.netConfig.dnsIp=doc2IPAddress(doc["dns"]);
  if (!doc["ssid"].isNull()) strncpy(configFlash.netConfig.ssid,doc["ssid"].as<const char*>(),sizeof(configFlash.netConfig.ssid));
  if (!doc["pwd"].isNull()) strncpy(configFlash.netConfig.pwd,doc["pwd"].as<const char*>(),sizeof(configFlash.netConfig.pwd));
  if (!doc["userName"].isNull()) strncpy(configFlash.netConfig.userName,doc["userName"].as<const char*>(),sizeof(configFlash.netConfig.userName));
  if (!doc["userPwd"].isNull()) strncpy(configFlash.netConfig.userPwd,doc["userPwd"].as<const char*>(),sizeof(configFlash.netConfig.userPwd));
  if (!doc["timeServer"].isNull()) strncpy(configFlash.netConfig.timeServer,doc["timeServer"].as<const char*>(),sizeof(configFlash.netConfig.timeServer));
  if (!doc["syslogLevel"].isNull()) configFlash.netConfig.syslogLevel=doc["syslogLevel"];
  if (!doc["syslogIp"].isNull()) configFlash.netConfig.syslogIp=doc2IPAddress(doc["syslogIp"]);
  if (!doc["syslogPort"].isNull()) configFlash.netConfig.syslogPort=doc["syslogPort"];
  if (!doc["tz"].isNull()) strncpy(configFlash.timeZoneConfig.tz,doc["tz"].as<const char*>(),sizeof(configFlash.timeZoneConfig.tz));
  if (!doc["tzCode"].isNull()) strncpy(configFlash.timeZoneConfig.tzCode,doc["tzCode"].as<const char*>(),sizeof(configFlash.timeZoneConfig.tzCode));

}

void CVdmConfig::postProtCfg (JsonObject doc)
{
  if (!doc["prot"].isNull()) configFlash.protConfig.dataProtocol = doc["prot"];
  if (!doc["ip"].isNull()) configFlash.protConfig.brokerIp = doc2IPAddress(doc["ip"]);
  if (!doc["port"].isNull()) configFlash.protConfig.brokerPort = doc["port"];
  if (!doc["interval"].isNull()) configFlash.protConfig.brokerInterval = doc["interval"];
  if (!doc["user"].isNull()) strncpy(configFlash.protConfig.userName,doc["user"].as<const char*>(),sizeof(configFlash.netConfig.userName));
  if (!doc["pwd"].isNull()) strncpy(configFlash.protConfig.userPwd,doc["pwd"].as<const char*>(),sizeof(configFlash.netConfig.userPwd));
  if (!doc["pubTarget"].isNull()) configFlash.protConfig.publishTarget = doc["pubTarget"];
  if (!doc["pubAllTemps"].isNull()) configFlash.protConfig.publishAllTemps = doc["pubAllTemps"];
  if (!doc["pubPathAsRoot"].isNull()) configFlash.protConfig.publishPathAsRoot = doc["pubPathAsRoot"];
}

void CVdmConfig::postValvesCfg (JsonObject doc)
{
  uint8_t chunkStart=doc["chunkStart"];
  uint8_t chunkEnd=doc["chunkEnd"];
  uint8_t idx=0;

  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
    syslog.log(LOG_DEBUG,"VdmConfig: post valve cfg - chunk start: "+String(chunkStart)+" chunk end: "+String(chunkEnd));
  }

  if (!doc["calib"]["dayOfCalib"].isNull()) configFlash.valvesConfig.dayOfCalib=doc["calib"]["dayOfCalib"];
  if (!doc["calib"]["hourOfCalib"].isNull()) configFlash.valvesConfig.hourOfCalib=doc["calib"]["hourOfCalib"];
 
  
  for (uint8_t i=chunkStart-1; i<chunkEnd; i++) {
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
      syslog.log(LOG_DEBUG,"VdmConfig: post valve cfg - idx: "+String(idx)+" i:"+String(i));
    }

    if (!doc["valves"][idx]["name"].isNull()) strncpy(configFlash.valvesConfig.valveConfig[i].name,doc["valves"][idx]["name"].as<const char*>(),sizeof(configFlash.valvesConfig.valveConfig[i].name));
    if (!doc["valves"][idx]["active"].isNull()) configFlash.valvesConfig.valveConfig[i].active=doc["valves"][idx]["active"];
    if (!doc["valves"][idx]["tIdx1"].isNull()) {
      StmApp.actuators[i].tIdx1=doc["valves"][idx]["tIdx1"];
      StmApp.setTempIdxActive=true;
      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
        syslog.log(LOG_DEBUG,"VdmConfig: post valve cfg - idx1 val: "+String(StmApp.actuators[i].tIdx1));
      }
    }
    if (!doc["valves"][idx]["tIdx2"].isNull()) {
      StmApp.actuators[i].tIdx2=doc["valves"][idx]["tIdx2"];
      StmApp.setTempIdxActive=true;
    }
    idx++;
  }
  
  if (!doc["calib"]["cycles"].isNull()) {
    StmApp.learnAfterMovements=doc["calib"]["cycles"];
    StmApp.setLearnAfterMovements();
  }
  if (!doc["motor"]["lowC"].isNull()) {
    StmApp.motorChars.maxLowCurrent=doc["motor"]["lowC"];
    StmApp.setMotorCharsActive=true;
  }
  if (!doc["motor"]["highC"].isNull()) {
    StmApp.motorChars.maxHighCurrent=doc["motor"]["highC"];
    StmApp.setMotorCharsActive=true;
  }
  if (!doc["motor"]["startOnPower"].isNull()) {
    StmApp.motorChars.startOnPower=doc["motor"]["startOnPower"];
    StmApp.setMotorCharsActive=true;
  }
  
  if (StmApp.setMotorCharsActive) {
    StmApp.setMotorChars();
    StmApp.setMotorCharsActive=false;
  }
 
  if ((StmApp.setTempIdxActive) && (chunkEnd==12)) {
    StmApp.setTempIdx();
    Services.restartSTM=true;
    //StmApp.matchSensors();
    //StmApp.matchSensorRequest = true;
    StmApp.setTempIdxActive=false;
  }
}

void CVdmConfig::postValvesControlCfg (JsonObject doc)
{
  
  uint8_t chunkStart=doc["chunkStart"];
  uint8_t chunkEnd=doc["chunkEnd"];
  uint8_t idx=0;
   
  for (uint8_t i=chunkStart-1; i<chunkEnd; i++) {
    if (!doc["valves"][idx]["active"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].active=doc["valves"][idx]["active"];
    if (!doc["valves"][idx]["link"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].link=doc["valves"][idx]["link"];
    if (!doc["valves"][idx]["vSource"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].valueSource=doc["valves"][idx]["vSource"];
    if (!doc["valves"][idx]["tSource"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].targetSource=doc["valves"][idx]["tSource"];
    if (!doc["valves"][idx]["xp"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].xp=doc["valves"][idx]["xp"];
    if (!doc["valves"][idx]["offset"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].offset=doc["valves"][idx]["offset"];
    if (!doc["valves"][idx]["ti"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].ti=doc["valves"][idx]["ti"];
    if (!doc["valves"][idx]["ts"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].ts=doc["valves"][idx]["ts"];  
    if (!doc["valves"][idx]["ki"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].ki=doc["valves"][idx]["ki"];  
    if (!doc["valves"][idx]["scheme"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].scheme=doc["valves"][idx]["scheme"];  
    if (!doc["valves"][idx]["startAZ"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].startActiveZone=doc["valves"][idx]["startAZ"];  
    if (!doc["valves"][idx]["endAZ"].isNull()) configFlash.valvesControlConfig.valveControlConfig[i].endActiveZone=doc["valves"][idx]["endAZ"];  
    idx++;
  } 

  if (!doc["common"]["heatControl"].isNull()) {
    configFlash.valvesControlConfig.heatControl=doc["common"]["heatControl"];
  } 
  if (!doc["common"]["parkPosition"].isNull()) {
    configFlash.valvesControlConfig.parkingPosition=doc["common"]["parkPosition"];
  } 
  
}

void CVdmConfig::postTempsCfg (JsonObject doc)
{
  uint8_t chunkStart=doc["chunkStart"];
  uint8_t chunkEnd=doc["chunkEnd"];
  uint8_t idx=0;
 
  for (uint8_t i=chunkStart-1; i<chunkEnd; i++) {
    if (!doc["temps"][idx]["name"].isNull()) strncpy(configFlash.tempsConfig.tempConfig[i].name,doc["temps"][idx]["name"].as<const char*>(),sizeof(configFlash.tempsConfig.tempConfig[i].name));
    if (!doc["temps"][idx]["id"].isNull()) strncpy(configFlash.tempsConfig.tempConfig[i].ID,doc["temps"][idx]["id"].as<const char*>(),sizeof(configFlash.tempsConfig.tempConfig[i].ID));
    if (!doc["temps"][idx]["active"].isNull()) configFlash.tempsConfig.tempConfig[i].active=doc["temps"][idx]["active"];
    if (!doc["temps"][idx]["offset"].isNull()) configFlash.tempsConfig.tempConfig[i].offset=10*(doc["temps"][idx]["offset"].as<float>()) ;
    idx++;
  }
}

void CVdmConfig::postSysCfg (JsonObject doc)
{
  if (!doc["CF"].isNull()) configFlash.systemConfig.celsiusFahrenheit = doc["CF"];
  if (!doc["station"].isNull()) strncpy(configFlash.systemConfig.stationName,doc["station"].as<const char*>(),sizeof(configFlash.systemConfig.stationName));
 
}

String CVdmConfig::handleAuth (JsonObject doc)
{
  bool userCheck=false;
  bool pwdCheck=false;
  uint8_t result=0;
  if ((strlen(configFlash.netConfig.userName)>0) && (strlen(configFlash.netConfig.userPwd)>0)) {
    if (!(doc["user"].isNull() || doc["pwd"].isNull())) {
      const char* du=doc["user"].as<const char*>();
      const char* dp=doc["pwd"].as<const char*>();
      userCheck=(strncmp (configFlash.netConfig.userName,du,sizeof(configFlash.netConfig.userName)))==0;
      pwdCheck=(strncmp (configFlash.netConfig.userPwd,dp,sizeof(configFlash.netConfig.userPwd)))==0;
    }
    if (userCheck) result+=1;  // userName compares
    if (pwdCheck) result+=2;  // pwdName compares
  } else result=3;
  return ("{\"auth\":"+String(result)+"}");
}


int8_t CVdmConfig::findTempID (char* tempId)
{
  uint8_t idx=0;
  for (uint8_t i=0;i<TEMP_SENSORS_COUNT;i++) {
    if (strncmp (tempId,configFlash.tempsConfig.tempConfig[i].ID,sizeof(configFlash.tempsConfig.tempConfig[i].ID)) == 0) { 
      return (idx);
    }
    idx++;
  }
  return (-1);
}

void CVdmConfig::checkToResetCfg() {
  pinMode(pinSetFactoryCfg, INPUT_PULLUP);
  if (digitalRead(pinSetFactoryCfg)==0) {  
    UART_DBG.println("Entry reset config");
    delay(1000);
    if (digitalRead(pinSetFactoryCfg)==0) {
        UART_DBG.println("Reset config, remove shortcut");
        while (digitalRead(pinSetFactoryCfg)==0) {
          delay(100);
        }
        UART_DBG.println("Reset config now");
        VdmConfig.restoreConfig(true);
    }
  }
}
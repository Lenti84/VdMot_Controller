/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano

  Comments:


***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************

*END************************************************************************/


#include <stdint.h>
#include "VdmConfig.h"
#include "web.h"

CVdmConfig VdmConfig;


CVdmConfig::CVdmConfig()
{
}

void CVdmConfig::init()
{
  setDefault();
  readConfig();
}


void CVdmConfig::setDefault()
{
  configFlash.netConfig.eth_wifi = 0;	      // 0 = eth (default)
  configFlash.netConfig.dhcpEnabled = 1;    // 0 = no DHCP, 1 = use DHCP
  configFlash.netConfig.staticIp = 0;
  configFlash.netConfig.mask = 0;
  configFlash.netConfig.gateway = 0;
  configFlash.netConfig.dnsIp = 0;
  memset (configFlash.netConfig.ssid,0,sizeof(configFlash.netConfig.ssid));
  memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.pwd));
  memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.userName));
  memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.userPwd));
  clearConfig();
}

void CVdmConfig::clearConfig()
{ 
  configFlash.netConfig.timeOffset=3600;
  configFlash.netConfig.daylightOffset=3600; 
  configFlash.protConfig.dataProtocol = 0;
  configFlash.protConfig.brokerIp = 0;
  configFlash.protConfig.brokerPort = 0;

  for (uint8_t i=0; i<ACTUATOR_COUNT; i++){
    configFlash.valvesConfig.valveConfig[i].active = false;
    memset (configFlash.valvesConfig.valveConfig[i].name,0,sizeof(configFlash.valvesConfig.valveConfig[i].name));
  }
  configFlash.valvesConfig.dayOfCalib=0;
  configFlash.valvesConfig.hourOfCalib=0;
  memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.timeServer));
  strncpy(configFlash.netConfig.timeServer,"pool.ntp.org",sizeof(configFlash.netConfig.timeServer));
}



void CVdmConfig::readConfig()
{
  prefs.begin(nvsNetCfg,false);
  configFlash.netConfig.eth_wifi=prefs.getUChar(nvsNetEthwifi);
  configFlash.netConfig.dhcpEnabled=prefs.getUChar(nvsNetDhcp,1);
  configFlash.netConfig.staticIp=prefs.getULong(nvsNetStaticIp);
  configFlash.netConfig.mask=prefs.getULong(nvsNetMask);
  configFlash.netConfig.gateway=prefs.getULong(nvsNetGW);
  configFlash.netConfig.dnsIp=prefs.getULong(nvsNetDnsIp); 
  prefs.getString(nvsNetSsid,(char*) configFlash.netConfig.ssid,sizeof(configFlash.netConfig.ssid));
  prefs.getString(nvsNetPwd,(char*) configFlash.netConfig.pwd,sizeof(configFlash.netConfig.pwd));
  prefs.getString(nvsNetUserName,(char*) configFlash.netConfig.userName,sizeof(configFlash.netConfig.userName));
  prefs.getString(nvsNetUserPwd,(char*) configFlash.netConfig.userPwd,sizeof(configFlash.netConfig.userPwd));
  prefs.getString(nvsNetTimeServer,(char*) configFlash.netConfig.timeServer,sizeof(configFlash.netConfig.timeServer));
  if (strlen(configFlash.netConfig.timeServer) == 0) {
    memset (configFlash.netConfig.pwd,0,sizeof(configFlash.netConfig.timeServer));
    strncpy(configFlash.netConfig.timeServer,"pool.ntp.org",sizeof(configFlash.netConfig.timeServer));
  }
  configFlash.netConfig.timeOffset=prefs.getLong(nvsNetTimeOffset,3600);
  configFlash.netConfig.daylightOffset=prefs.getInt(nvsNetDayLightOffset,3600);
  prefs.end();

  prefs.begin(nvsProtCfg,false);
  configFlash.protConfig.dataProtocol = prefs.getUChar(nvsProtDataProt);
  configFlash.protConfig.brokerIp = prefs.getULong(nvsProtBrokerIp);
  configFlash.protConfig.brokerPort = prefs.getUShort(nvsProtBrokerPort);
  prefs.end();

  prefs.begin(nvsValvesCfg,false);
  prefs.getBytes(nvsValves, (void *) configFlash.valvesConfig.valveConfig, sizeof(configFlash.valvesConfig.valveConfig));
  configFlash.valvesConfig.dayOfCalib=prefs.getUChar(nvsDayOfCalib);
  configFlash.valvesConfig.hourOfCalib=prefs.getUChar(nvsHourOfCalib); 
  prefs.end();

  prefs.begin(nvsTempsCfg,false);
  prefs.getBytes(nvsTemps,(void *) configFlash.tempsConfig.tempConfig, sizeof(configFlash.tempsConfig.tempConfig));
  prefs.end();
}

void CVdmConfig::writeConfig()
{
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
  prefs.putLong(nvsNetTimeOffset,configFlash.netConfig.timeOffset);
  prefs.putInt(nvsNetDayLightOffset,configFlash.netConfig.daylightOffset);
  prefs.end();

  prefs.begin(nvsProtCfg,false);
  prefs.clear();
  prefs.putUChar(nvsProtDataProt,configFlash.protConfig.dataProtocol);
  if (configFlash.protConfig.dataProtocol==protTypeMqtt) {
    prefs.putULong(nvsProtBrokerIp,configFlash.protConfig.brokerIp);
    prefs.putUShort(nvsProtBrokerPort,configFlash.protConfig.brokerPort);
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
}

uint32_t CVdmConfig::doc2IPAddress(String id)
{
  IPAddress c1;
  bool b;
  b=c1.fromString(id) ; 
 // if (b) UART_DBG.println("b = true"); else UART_DBG.println("b = false");
  return (uint32_t) c1 ;
}

void CVdmConfig::postNetCfg (String payload)
{
  UART_DBG.println("post net cfg");
  UART_DBG.println(payload);
  const size_t capacity = JSON_ARRAY_SIZE(20) + 1000;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, payload);
  configFlash.netConfig.eth_wifi=doc["ethWifi"];
  configFlash.netConfig.dhcpEnabled=doc["dhcp"];
  configFlash.netConfig.staticIp=doc2IPAddress(doc["ip"]);
  configFlash.netConfig.mask=doc2IPAddress(doc["mask"]);
  configFlash.netConfig.gateway=doc2IPAddress(doc["gw"]);
  configFlash.netConfig.dnsIp=doc2IPAddress(doc["dns"]);
  strncpy(configFlash.netConfig.ssid,doc["ssid"].as<const char*>(),sizeof(configFlash.netConfig.ssid));
  strncpy(configFlash.netConfig.userName,doc["userName"].as<const char*>(),sizeof(configFlash.netConfig.userName));
  strncpy(configFlash.netConfig.timeServer,doc["timeServer"].as<const char*>(),sizeof(configFlash.netConfig.timeServer));
  configFlash.netConfig.timeOffset = doc["timeOffset"];
  configFlash.netConfig.daylightOffset = doc["timeDST"];
  doc.clear();
  
}

void CVdmConfig::postProtCfg (String payload)
{
  UART_DBG.println("post prot cfg");
  UART_DBG.println(payload);
  const size_t capacity = JSON_ARRAY_SIZE(10) + 500;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, payload);
  configFlash.protConfig.dataProtocol = doc["prot"];
  configFlash.protConfig.brokerIp = doc2IPAddress(doc["mqttIp"]);
  configFlash.protConfig.brokerPort = doc["mqttPort"];
  doc.clear(); 
}

void CVdmConfig::postValvesCfg (String payload)
{
  UART_DBG.println("post valves cfg");
  UART_DBG.println(payload);
  const size_t capacity = JSON_ARRAY_SIZE(50) + 2000;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, payload);
  //doc["vlv"][x]["tar"]
  //configFlash.tempsConfig.tempConfig[i].name = doc["name"][i];
  configFlash.valvesConfig.dayOfCalib=doc["calib"]["dayOfCalib"];
  configFlash.valvesConfig.hourOfCalib=doc["calib"]["hourOfCalib"];
  for (uint8_t i=0; i<ACTUATOR_COUNT; i++) {
    strncpy(configFlash.valvesConfig.valveConfig[i].name,doc["valves"][i]["name"].as<const char*>(),sizeof(configFlash.valvesConfig.valveConfig[i].name));
    configFlash.valvesConfig.valveConfig[i].active=doc["valves"][i]["active"];
  }
  doc.clear();
}

void CVdmConfig::postTempsCfg (String payload)
{
  UART_DBG.println("post temps cfg");
  UART_DBG.println(payload);
  const size_t capacity = JSON_ARRAY_SIZE(50) + 2000;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, payload);
  for (uint8_t i=0; i<ACTUATOR_COUNT; i++) {
    strncpy(configFlash.tempsConfig.tempConfig[i].name,doc[i]["name"].as<const char*>(),sizeof(configFlash.tempsConfig.tempConfig[i].name));
    configFlash.tempsConfig.tempConfig[i].active=doc[i]["active"];
    configFlash.tempsConfig.tempConfig[i].offset=10*(doc[i]["offset"].as<float>()) ;
  }
  doc.clear();
}
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
#include "web.h"
#include "stmApp.h"
#include "VdmNet.h"
#include "VdmConfig.h"
#include "VdmSystem.h"
#include "PIControl.h"
#include "ServerServices.h"
#include "WiFi.h"
#include "helper.h"
#include "stm32.h"
#include "stmApp.h"
#include "stm32ota.h"
#include "mqtt.h"
#include "compile_time.h"
#include "esp_system.h"
 
CWeb Web;

CWeb::CWeb()
{
}

String CWeb::getSysConfig (VDM_SYSTEM_CONFIG sysConfig)
{
  String result = "{\"CF\":"+String(sysConfig.celsiusFahrenheit)+","+
                  "\"station\":\""+String(sysConfig.stationName)+"\""+
                  "}";  
  return result;  
}

String CWeb::getMsgConfig (VDM_MSG_CONFIG msgConfig)
{
  String result = "{\"reason\":{\"valveFailed\":"+String(msgConfig.reason.reasonFlags.valveFailed)+","+
                  "\"valveBlocked\":"+String(msgConfig.reason.reasonFlags.valveBlocked)+","+
                  "\"notDetect\":"+String(msgConfig.reason.reasonFlags.notDetect)+","+
                  "\"reset\":"+String(msgConfig.reason.reasonFlags.reset)+","+
                  "\"ds18Failed\":"+String(msgConfig.reason.reasonFlags.ds18Failed)+","+
                  "\"tValueFailed\":"+String(msgConfig.reason.reasonFlags.tValueFailed)+","+
                  "\"mqttTimeOut\":"+String(msgConfig.reason.reasonFlags.mqttTimeOut)+","+
                  "\"mqttTimeOutTime\":"+String(msgConfig.reason.mqttTimeOutTime)+"},"+
                  "\"PO\":{\"active\":"+String(msgConfig.activeFlags.pushOver)+","+
                  "\"appToken\":\""+String(msgConfig.pushover.appToken)+"\","+
                  "\"userToken\":\""+String(msgConfig.pushover.userToken)+"\","+
                  "\"title\":\""+String(msgConfig.pushover.title)+"\"},"+
                  "\"Email\":{\"active\":"+String(msgConfig.activeFlags.email)+","+
                  "\"user\":\""+String(msgConfig.email.user)+"\","+
                  "\"pwd\":\""+String(msgConfig.email.pwd)+"\","+
                  "\"host\":\""+String(msgConfig.email.host)+"\","+
                  "\"port\":\""+String(msgConfig.email.port)+"\","+
                  "\"recipient\":\""+String(msgConfig.email.recipient)+"\","+
                  "\"title\":\""+String(msgConfig.email.title)+"\"}"+
                  "}";  
  return result;  
}

String CWeb::getNetConfig (VDM_NETWORK_CONFIG netConfig)
{
  String result = "{\"ethWifi\":"+String(netConfig.eth_wifi)+","+
                  "\"dhcp\":"+String(netConfig.dhcpEnabled)+","+
                  "\"ip\":\""+ip2String(netConfig.staticIp)+"\","+
                  "\"mask\":\""+ip2String(netConfig.mask)+"\","+
                  "\"gw\":\""+ip2String(netConfig.gateway)+"\","+
                  "\"dns\":\""+ip2String(netConfig.dnsIp)+"\","+
                  "\"userName\":\""+String(netConfig.userName)+"\","+
                  "\"ssid\":\""+String(netConfig.ssid)+"\","+
                  "\"timeServer\":\""+String(netConfig.timeServer)+"\","+
                  "\"tz\":\""+String(VdmConfig.configFlash.timeZoneConfig.tz)+"\","+
                  "\"tzCode\":\""+String(VdmConfig.configFlash.timeZoneConfig.tzCode)+"\","+
                  "\"syslogLevel\":"+String(netConfig.syslogLevel)+","+
                  "\"syslogIp\":\""+ip2String(netConfig.syslogIp)+"\","+
                  "\"syslogPort\":"+String(netConfig.syslogPort)+  
                  "}";  
  return result;  
}

String CWeb::getNetInfo(VDM_NETWORK_INFO networkInfo)
{
  String result = "{\"ethWifi\":"+String(networkInfo.interfaceType)+","+
                  "\"dhcp\":"+String(networkInfo.dhcpEnabled)+","+
                   "\"ip\":\""+networkInfo.ip.toString()+"\","+
                  "\"mac\":\""+networkInfo.mac+"\","+
                  "\"mask\":\""+networkInfo.mask.toString()+"\","+
                  "\"gw\":\""+networkInfo.gateway.toString()+"\","+
                  "\"dns\":\""+networkInfo.dnsIp.toString()+"\"}";
  return result;  
}

String CWeb::getProtConfig (VDM_PROTOCOL_CONFIG protConfig)
{
  String result = "{\"prot\":"+String(protConfig.dataProtocol)+","+
                    "\"ip\":\""+ip2String(protConfig.brokerIp)+"\","+
                    "\"port\":\""+String(protConfig.brokerPort)+"\","+
                    "\"interval\":"+String(protConfig.brokerInterval)+","+
                    "\"mqttTOTSActive\":"+String(protConfig.mqttConfig.flags.timeoutTSActive)+","+
                    "\"mqttTODSActive\":"+String(protConfig.mqttConfig.flags.timeoutDSActive)+","+
                    "\"mqttTO\":"+String(protConfig.mqttConfig.timeOut)+","+
                    "\"mqttToPos\":"+String(protConfig.mqttConfig.toPos)+","+
                    "\"publish\":"+String(protConfig.publishInterval)+","+
                    "\"pubMinDelay\":"+String(protConfig.minBrokerDelay)+","+
                    "\"pubOnChange\":"+String(protConfig.protocolFlags.publishOnChange)+","+
                    "\"pubRetained\":"+String(protConfig.protocolFlags.publishRetained)+","+
                    "\"pubSeparate\":"+String(protConfig.protocolFlags.publishSeparate)+","+
                    "\"pubPlainText\":"+String(protConfig.protocolFlags.publishPlainText)+","+
                    "\"pubAllTemps\":"+String(protConfig.protocolFlags.publishAllTemps)+","+
                    "\"pubPathAsRoot\":"+String(protConfig.protocolFlags.publishPathAsRoot)+","+
                    "\"pubUpTime\":"+String(protConfig.protocolFlags.publishUpTime)+","+
                    "\"pubDiag\":"+String(protConfig.protocolFlags.publishDiag)+","+
                    "\"keepAliveTime\":"+String(protConfig.keepAliveTime)+","+
                    "\"user\":\""+String(protConfig.userName)+"\"}";  
  return result;  
}

String CWeb::getValvesConfig (VDM_VALVES_CONFIG valvesConfig)
{
  String result = "{\"calib\":{\"dayOfCalib\":"+String(valvesConfig.dayOfCalib) + "," +
                   "\"hourOfCalib\":"+String(valvesConfig.hourOfCalib)+ "}," +
                   "\"valves\":[" ;

  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    result += "{\"name\":\""+String(valvesConfig.valveConfig[x].name) + "\"," +
              "\"active\":"+String(valvesConfig.valveConfig[x].active) + ","+
              "\"tIdx1\":"+String(StmApp.actuators[x].tIdx1) + ","+
              "\"tIdx2\":"+String(StmApp.actuators[x].tIdx2) + "}";
    if (x<ACTUATOR_COUNT-1) result += ",";
  }  
  result += "]}"; 
  return result;  
}


String CWeb::getMotorConfig (MOTOR_CHARS motorConfig)
{
  String result =   "{\"calib\":{";
        result +=   "\"cycles\":"+String(StmApp.learnAfterMovements)+ "}," +
                    "\"motor\":{\"lowC\":"+String(motorConfig.maxLowCurrent) + "," +
                    "\"highC\":"+String(motorConfig.maxHighCurrent)+ "," +
                    "\"noOfMinCount\":"+String(motorConfig.noOfMinCount)+ "," +
                    "\"maxCalReps\":"+String(motorConfig.maxCalReps)+ "," +
                    "\"startOnPower\":"+String(motorConfig.startOnPower)+ "}}";
  return result;  
}

String CWeb::getValvesControlConfig (VDM_VALVES_CONTROL_CONFIG valvesControlConfig)
{
  String result = "{\"common\":{\"heatControl\":"+String(valvesControlConfig.heatControl) + "," +
                    "\"parkPosition\":"+String(valvesControlConfig.parkingPosition) + 
                     "}," +
                    "\"valves\":[" ;

  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    result += "{\"name\":\""+String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name) + "\"," +
              "\"active\":"+String(valvesControlConfig.valveControlConfig[x].controlFlags.active) + ","+
              "\"allow\":"+String(valvesControlConfig.valveControlConfig[x].controlFlags.allow) + ","+
              "\"window\":"+String(valvesControlConfig.valveControlConfig[x].controlFlags.windowInstalled) + ","+
              "\"link\":"+String(valvesControlConfig.valveControlConfig[x].link) + ","+
              "\"vSource\":"+String(valvesControlConfig.valveControlConfig[x].valueSource) + ","+
              "\"tSource\":"+String(valvesControlConfig.valveControlConfig[x].targetSource) + ","+
              "\"xp\":"+String(valvesControlConfig.valveControlConfig[x].xp) + ","+
              "\"offset\":"+String(valvesControlConfig.valveControlConfig[x].offset) + ","+
              "\"ti\":"+String(valvesControlConfig.valveControlConfig[x].ti) + ","+
              "\"ts\":"+String(valvesControlConfig.valveControlConfig[x].ts) + ","+
              "\"ki\":"+String(valvesControlConfig.valveControlConfig[x].ki) + ","+
              "\"scheme\":"+String(valvesControlConfig.valveControlConfig[x].scheme) + ","+
              "\"startAZ\":"+String(valvesControlConfig.valveControlConfig[x].startActiveZone) + ","+
              "\"endAZ\":"+String(valvesControlConfig.valveControlConfig[x].endActiveZone) + "}";

    if (x<ACTUATOR_COUNT-1) result += ",";
  }  
  result += "]}"; 
  return result;  
}

String CWeb::getTempsConfig (VDM_TEMPS_CONFIG tempsConfig)
{
  String result = "[";
  for (uint8_t x=0;x<TEMP_SENSORS_COUNT;x++) {
    result += "{\"name\":\""+String(tempsConfig.tempConfig[x].name) + "\"," +
              "\"active\":"+String(tempsConfig.tempConfig[x].active) + "," +
              "\"id\":\""+String(tempsConfig.tempConfig[x].ID) + "\"," +
              "\"offset\":"+String(((float)tempsConfig.tempConfig[x].offset)/10,1) + "}";
    if (x<TEMP_SENSORS_COUNT-1) result += ",";
  }  
  result += "]"; 
  return result;  
}

String CWeb::getTempSensorsID()
{
  String result = "[";
  String id="";
  for (uint8_t x=0;x<StmApp.tempsCount;x++) {
    result += "{\"id\":\""+String(StmApp.tempsId[x].id) + "\"}";
    if (x<StmApp.tempsCount-1) result += ",";
  }  
  result += "]"; 
  return result;  
}

bool CWeb::findIdInValve (uint8_t idx) {
  for (uint8_t i=0;i<ACTUATOR_COUNT;i++) {
    if (StmApp.actuators[i].tIdx1 == idx+1) return (true); 
    if (StmApp.actuators[i].tIdx2 == idx+1) return (true); 
  }
  return (false);
}  

String CWeb::getSysInfo()
{
  String wt32Build="";
  time_t t;
  struct tm *tmp ;
  char buf[50];

  #ifdef FIRMWARE_BUILD 
    t = __TIME_UNIX__; //FIRMWARE_BUILD;
    tmp = gmtime (&t);
    strftime (buf, sizeof(buf), " build %d.%m.%Y %H:%M", tmp);
    wt32Build = String(buf);
  #endif  

  t = VdmSystem.stmBuild;
  String stmBuild="";
  if (t>1) {
    tmp = gmtime (&t);
    strftime (buf, sizeof(buf), " build %d.%m.%Y %H:%M", tmp);
    stmBuild = String (buf);
  }
  float usedFlashProz = 100 * (float) ESP.getSketchSize()/ (float)ESP.getFreeSketchSpace();

  String result = "{\"wt32version\":\""+String(FIRMWARE_VERSION)+wt32Build+"\"," +
                  "\"wt32ChipModel\":\""+VdmSystem.getChipModel()+"\"," +
                  "\"wt32ChipCores\":"+VdmSystem.chip_info.cores+ "," +
                  "\"wt32ChipCoreRev\":"+VdmSystem.chip_info.revision+","+
                  "\"wt32ChipStack\":"+VdmSystem.stackSize+","+
                  "\"wt32Sketch\":"+ESP.getSketchSize()+","+
                  "\"wt32FlashUsed\":"+String(usedFlashProz,1)+","+
                  "\"stm32version\":\""+VdmSystem.stmVersion+stmBuild+"\","+
                  "\"stm32ChipId\":\"0x"+String(VdmSystem.stmID,16)+" "+String(StmOta.checkChipName(VdmSystem.stmID))+"\""+
                  "}";
  return result;  
}

String CWeb::getSysDynInfo()
{
  struct tm timeinfo;
  char buf[50];
  String sLastCalib;
  time_t lastCalib=VdmConfig.miscValues.lastCalib;
  localtime_r(&lastCalib, &timeinfo);
  strftime (buf, sizeof(buf), "%A, %B %d.%Y %H:%M:%S", &timeinfo);
  sLastCalib = String(buf);
  
  String result = "{\"locTime\":\""+VdmSystem.localTime()+"\"," +
                  "\"sntpActive\":"+String(VdmNet.sntpActive)+"," +
                  "\"sntpReachable\":"+String(VdmNet.sntpReachable)+"," +
                  "\"upTime\":\""+VdmSystem.getUpTime()+"\"," +
                  "\"heap\":\""+ConvBinUnits(ESP.getFreeHeap(),1)+ "\"," +
                  "\"minheap\":\""+ConvBinUnits(ESP.getMinFreeHeap(),1)+ "\"," +
                  "\"wifirssi\":"+WiFi.RSSI()+ "," +
                  "\"wifich\":"+WiFi.channel()+ "," +
                  "\"wifiStatus\":"+WiFi.status()+ "," +
                  "\"stmStatus\":"+String(StmApp.stmStatus)+ "," +
                  "\"stmInit\":"+String(StmApp.stmInitState);
                  if (VdmConfig.configFlash.protConfig.dataProtocol>0) {
                    result += ",\"brokerStatus\":"+String(Mqtt.mqttState);
                    result += ",\"brokerConnected\":"+String(Mqtt.mqttConnected);
                  }
                  result += ",\"lastCalib\":\""+String(sLastCalib)+"\"";
                  result += ",\"heatControl\":"+String(VdmConfig.heatValues.heatControl);
                  result +="}";
  return result;  
}

bool CWeb::getControlActive() 
{
  bool active=false;
  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) { 
    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active) active = true;
  }
  return (((VdmConfig.heatValues.heatControl==piControlOnHeating) || (VdmConfig.heatValues.heatControl==piControlOnCooling)) && active);
}

String CWeb::getValvesStatus() 
{
  bool start=false;
  uint8_t valveActive;
  bool controlActive = getControlActive();
  String result = "{";
  result += "\"valves\":[";
  uint8_t thisState;

  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) { 
    #ifdef ValveSimulation
      if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) StmApp.actuators[x].state=VLV_STATE_IDLE;
    #endif
    if ((StmApp.actuators[x].state!=VLV_STATE_OPENCIR) && (StmApp.actuators[x].state!=VLV_STATE_START)) {
      if (PiControl[x].failed) thisState=STATE_FAILED; else thisState=StmApp.actuators[x].state;
      if (start) result += ",";
      result += "{\"idx\":"+String(x+1) + ","+
                 "\"name\":\""+String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name) +"\"," +
                 "\"state\":"+String(thisState) + ","+
                 "\"pos\":"+String(StmApp.actuators[x].actual_position) + ","+
                 "\"meanCur\":" + String(StmApp.actuators[x].meancurrent) + ","+
                 "\"targetPos\":" + String(StmApp.actuators[x].target_position)+ ","+
                 "\"link\":"+String(VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link) + ","+
                 "\"moves\":" + String(StmApp.actuators[x].movements)+ ","+
                 "\"oc\":" + String(StmApp.actuators[x].opening_count)+ ","+
                 "\"cc\":" + String(StmApp.actuators[x].closing_count)+ ","+
                 "\"dc\":" + String(StmApp.actuators[x].deadzone_count)+ ","+
                 "\"cr\":" + String(StmApp.actuators[x].calibRetries);

                 if ((StmApp.stmInitState==STM_INIT_FINISHED) && StmApp.oneWireAllRead) {
                  if (StmApp.actuators[x].tIdx1>0) { 
                    if (StmApp.actuators[x].temp1>-500) {
                        result +=",\"temp1\":" + String(((float)StmApp.actuators[x].temp1)/10,1);
                    } else result +=",\"temp1\":\"failed\"";
                  }
                  if (StmApp.actuators[x].tIdx2>0) { 
                    if (StmApp.actuators[x].temp2>-500) {
                        result +=",\"temp2\":" + String(((float)StmApp.actuators[x].temp2)/10,1);
                    } else result +=",\"temp2\":\"failed\"";
                  }
                 }

                 if (controlActive || Mqtt.valveStates[x].tValueFailed) {
                  if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    result +=",\"tTarget\":" + String(((float)PiControl[x].target),1);
                    result +=",\"tValue\":" + String(((float)PiControl[x].value),1);
                  } else {
                    result +=",\"tTarget\":\"link #"+String(VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link)+"\"";
                    result +=",\"tValue\":\"link #"+String(VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link)+"\"";
                  }
                 }

                 if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.windowInstalled) {
                   result +=",\"window\":"+String(PiControl[x].windowState);
                 }
                 
                 valveActive = 0;
                 if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active==1) valveActive |= 1;
                 if (PiControl[x].controlActive) valveActive |= 2;
                 if (PiControl[x].failed) valveActive |= 4;
                 result +=",\"controlActive\":"+String(valveActive);
              
                 if (StmApp.actuators[x].calibration) {
                  result +=",\"calibration\":"+String(StmApp.actuators[x].calibration); 
                 }

                 if (Mqtt.valveStates[x].tValueFailed) {
                  result +=",\"tValueFailed\":"+String(Mqtt.valveStates[x].tValueFailed); 
                 }
                 result +="}";      
    start = true;
    }
  }  
  result += "]}";
  return result;
}

String CWeb::getTempsStatus(VDM_TEMPS_CONFIG tempsConfig) 
{
  String result = "[";
  int16_t temperature;
  bool start = false;
  String s;

  for (uint8_t i=0;i<StmApp.tempsCount;i++) {
    if ((StmApp.findTempIdxInValve(i)<0) && (VdmConfig.configFlash.tempsConfig.tempConfig[i].active)) {
      if (start) result += ",";
      temperature = StmApp.temps[i].temperature;
      if (StmApp.temps[i].temperature<=-500) s="\"failed\""; else s=String(((float)temperature)/10,1);
      result += "{\"id\":\"" + String(StmApp.temps[i].id) + "\","+
                "\"name\":\"" + String(tempsConfig.tempConfig[i].name) + "\","+
                "\"temp\":" +s+"}";
      start = true;
    }
  }  
  result += "]";
  return result;
}

String CWeb::getFSDir() 
{
  String result;
  String item;
  //VdmSystem.getFSDirectory();
  
  if (!VdmSystem.getFSInProgress) {
    if (VdmSystem.numfiles>0) {
      result = "[";
      for (uint8_t x=0; x<VdmSystem.numfiles; x++) {
        item = "{\"fName\":\"" + VdmSystem.Filenames[x].filename+"\","+
                "\"ftype\":\"" + VdmSystem.Filenames[x].ftype+"\","+
                "\"fsize\":\"" + VdmSystem.Filenames[x].fsize+"\""+
                "}";
        result+=item;
        if (x<VdmSystem.numfiles-1) result += ",";
      }  
      result += "]";
    } else result ="[]";
  }
  return result;
}

String CWeb::getStmUpdStatus()
{
  String result = "{\"percent\":"+String(Stm32.stmUpdPercent)+","+
                    "\"status\":"+String(Stm32.stmUpdateStatus)+","+
                    "\"chipId\":\"0x"+String(StmOta.chipId,HEX)+"\","+
                    "\"chipName\":\""+String(StmOta.chipName)+"\""+"}";
  return result;
}


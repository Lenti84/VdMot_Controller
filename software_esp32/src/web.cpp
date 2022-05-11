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
#include "WiFi.h"
#include "helper.h"
#include "stm32.h"
#include "stmApp.h"
#include "mqtt.h"

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
                  "\"timeOffset\":"+String(netConfig.timeOffset)+","+
                  "\"timeDST\":"+String(netConfig.daylightOffset)+","+
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
                    "\"pubTarget\":"+String(protConfig.publishTarget)+","+
                    "\"user\":\""+String(protConfig.userName)+"\"}";  
  return result;  
}

String CWeb::getValvesConfig (VDM_VALVES_CONFIG valvesConfig,MOTOR_CHARS motorConfig)
{
  String result = "{\"calib\":{\"dayOfCalib\":"+String(valvesConfig.dayOfCalib) + "," +
                   "\"hourOfCalib\":"+String(valvesConfig.hourOfCalib)+ "," +
                    "\"cycles\":"+String(StmApp.learnAfterMovements)+ "}," +
                   "\"motor\":{\"lowC\":"+String(motorConfig.maxLowCurrent) + "," +
                   "\"highC\":"+String(motorConfig.maxHighCurrent)+ "}," +
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


String CWeb::getValvesControlConfig (VDM_VALVES_CONTROL_CONFIG valvesControlConfig)
{
  String result = "{\"common\":{\"heatControl\":"+String(valvesControlConfig.heatControl) + "," +
                    "\"parkPosition\":"+String(valvesControlConfig.parkingPosition)+ "}," +
                   "\"valves\":[" ;

  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) {
    result += "{\"name\":\""+String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name) + "\"," +
              "\"active\":"+String(valvesControlConfig.valveControlConfig[x].active) + ","+
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
              "\"endAZ\":"+String(valvesControlConfig.valveControlConfig[x].endActiveZone)+ "}";

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
    t = FIRMWARE_BUILD;
    tmp = gmtime (&t);
    strftime (buf, sizeof(buf), " Build %d.%m.%Y %H:%M", tmp);
    wt32Build = String(buf);
  #endif

  t = VdmSystem.stmBuild;
  String stmBuild="";
  if (t>0) {
    tmp = gmtime (&t);
    strftime (buf, sizeof(buf), " Build %d.%m.%Y %H:%M", tmp);
    stmBuild = String (buf);
  }

  String result = "{\"wt32version\":\""+String(FIRMWARE_VERSION)+wt32Build+"\"," +
                  "\"wt32cores\":"+VdmSystem.chip_info.cores+ "," +
                  "\"wt32coreRev\":"+VdmSystem.chip_info.revision+","+
                  "\"stm32version\":\""+VdmSystem.stmVersion+stmBuild+"\""+
                  "}";
  return result;  
}

String CWeb::getSysDynInfo()
{
  struct tm timeinfo;
  char buf[50];
  String time;
  String upTime;

  if(!getLocalTime(&timeinfo)) {
    time = "Failed to obtain time";
  } else {
    strftime (buf, sizeof(buf), "%A, %B %d.%Y %H:%M:%S", &timeinfo);
    time = String(buf);
    uint64_t seconds=difftime(mktime(&timeinfo),mktime(&VdmNet.startTimeinfo));
    int hr=(int)(seconds/3600);
    int min=((int)(seconds/60))%60;
    int sec=(int)(seconds%60);
    String sMin = String(min);
    if (min<10) sMin = "0"+sMin;
    String sSec = String(sec);
    if (sec<10) sSec = "0"+sSec;
    upTime=String(hr)+":"+sMin+":"+sSec;
  }
  String result = "{\"locTime\":\""+time+"\"," +
                  "\"upTime\":\""+upTime+"\"," +
                  "\"heap\":\""+ConvBinUnits(ESP.getFreeHeap(),1)+ "\"," +
                  "\"minheap\":\""+ConvBinUnits(ESP.getMinFreeHeap(),1)+ "\"," +
                  "\"wifirssi\":"+WiFi.RSSI()+ "," +
                  "\"wifich\":"+WiFi.channel()+ "," +
                  "\"stmStatus\":"+String(StmApp.stmStatus);
                  if (VdmConfig.configFlash.protConfig.dataProtocol>0) {
                    result += ",\"brokerStatus\":"+String(Mqtt.mqttState);
                    result += ",\"brokerConnected\":"+String(Mqtt.mqttConnected);
                  }
                  result +="}";
  return result;  
}

String CWeb::getValvesStatus() 
{
  bool start=false;
  String result = "[";
  for (uint8_t x=0;x<ACTUATOR_COUNT;x++) { 
    #ifdef ValveSimulation
      if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) StmApp.actuators[x].state=VLV_STATE_IDLE;
    #endif
    if ((StmApp.actuators[x].state!=VLV_STATE_OPENCIR) && (StmApp.actuators[x].state!=VLV_STATE_START)) {
      if (start) result += ",";
      result += "{\"idx\":"+String(x+1) + ","+
                 "\"name\":\""+String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name) +"\"," +
                 "\"state\":"+String(StmApp.actuators[x].state) + ","+
                 "\"pos\":"+String(StmApp.actuators[x].actual_position) + ","+
                 "\"meanCur\":" + String(StmApp.actuators[x].meancurrent) + ","+
                 "\"targetPos\":" + String(StmApp.actuators[x].target_position);
                 if (StmApp.actuators[x].temp1>-500) {
                    result +=",\"temp1\":" + String(((float)StmApp.actuators[x].temp1)/10,1);
                 }
                 if (StmApp.actuators[x].temp2>-500) {
                    result +=",\"temp2\":" + String(((float)StmApp.actuators[x].temp2)/10,1);
                 }
                 result +="}";      
    start = true;
    }
  }  
  result += "]";
  return result;
}

String CWeb::getTempsStatus(VDM_TEMPS_CONFIG tempsConfig) 
{
  String result = "[";
  int16_t temperature;
  bool start = false;
  for (uint8_t i=0;i<StmApp.tempsCount;i++) {
    if (!findIdInValve(i)) {
      if (start) result += ",";
      temperature = StmApp.temps[i].temperature;
      result += "{\"id\":\"" + String(StmApp.temps[i].id) + "\","+
                "\"name\":\"" + String(tempsConfig.tempConfig[i].name) + "\","+
                "\"temp\":" + String(((float)temperature)/10,1)+"}";
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
  VdmSystem.getFSDirectory();
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
  return result;
}

String CWeb::getStmUpdStatus()
{
  String result = "{\"percent\":"+String(Stm32.stmUpdPercent)+",\"status\":"+String(Stm32.stmUpdateStatus)+"}";
  return result;
}


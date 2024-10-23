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



#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Syslog.h>
#include <string.h>
#include "mqtt.h"
#include "globals.h"
#include "stmApp.h"
#include "VdmNet.h"
#include "VdmConfig.h"
#include "VdmSystem.h"
#include "VdmTask.h"
#include "helper.h"
#include "web.h"
#include "PIControl.h"
#include "Messenger.h"


CMqtt Mqtt;

void mcallback(char* topic, byte* payload, unsigned int length) 
{
    Mqtt.callback(topic, payload, length);
}


WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void CMqtt::mqtt_setup(IPAddress brokerIP,uint16_t brokerPort) 
{
    memset(lastValveValues,0x0,sizeof(lastValveValues)); 
    for (uint8_t i=0;i<TEMP_SENSORS_COUNT;i++) {
        lastTempValues[i].temperature=0;
        memset(lastTempValues[i].id,0x0,sizeof(lastTempValues[i].id));
        lastTempValues[i].publishNow=false;
    }
    messengerSend=false;
    mqttReceived=false;
    mqtt_client.setServer(brokerIP, brokerPort);
    mqtt_client.setCallback(mcallback);
   
    hadState=HAD_IDLE;
    memset (mqtt_mainTopic,0,sizeof(mqtt_mainTopic));
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishPathAsRoot) strncpy(mqtt_mainTopic,"/",sizeof(mqtt_mainTopic));

    if (strlen(VdmConfig.configFlash.systemConfig.stationName)>0) {
        strncat(mqtt_mainTopic, VdmConfig.configFlash.systemConfig.stationName,sizeof(mqtt_mainTopic) - strlen (mqtt_mainTopic) - 1);
        strncat(mqtt_mainTopic, "/",sizeof(mqtt_mainTopic) - strlen (mqtt_mainTopic) - 1);
    } else  {
        strncat(mqtt_mainTopic, DEFAULT_MAINTOPIC,sizeof(mqtt_mainTopic) - strlen (mqtt_mainTopic) - 1);
    }

    strncpy(mqtt_commonTopic, mqtt_mainTopic, sizeof(mqtt_commonTopic));
    strncat(mqtt_commonTopic, DEFAULT_COMMONTOPIC,sizeof(mqtt_commonTopic) - strlen (mqtt_commonTopic) - 1);
    strncpy(mqtt_valvesTopic, mqtt_mainTopic,sizeof(mqtt_commonTopic));
    strncat(mqtt_valvesTopic, DEFAULT_VALVESTOPIC,sizeof(mqtt_valvesTopic)- strlen (mqtt_valvesTopic) - 1);
    strncpy(mqtt_tempsTopic, mqtt_mainTopic,sizeof(mqtt_commonTopic));
    strncat(mqtt_tempsTopic, DEFAULT_TEMPSTOPIC,sizeof(mqtt_voltsTopic)- strlen (mqtt_voltsTopic) - 1);
    strncpy(mqtt_voltsTopic, mqtt_mainTopic,sizeof(mqtt_commonTopic));
    strncat(mqtt_voltsTopic, DEFAULT_VOLTSTOPIC,sizeof(mqtt_voltsTopic)- strlen (mqtt_voltsTopic) - 1);

    if (strlen(VdmConfig.configFlash.systemConfig.stationName)>0) {
        strncpy(stationName, VdmConfig.configFlash.systemConfig.stationName,sizeof(stationName));
    } else strncpy(stationName, DEVICE_HOSTNAME,sizeof(stationName));
    
    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        lastValveValues[x].lasttValuets=millis();
    }
}

CMqtt::CMqtt()
{
       
}

uint8_t CMqtt::checkForPublish() 
{
    uint8_t result=publishNothing;
    uint8_t i;

    forcePublish = true;
    if (firstPublish) {
        return publishAll; 
    }
    if (!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) {
        if ((millis()-tsPublish)>(1000*VdmConfig.configFlash.protConfig.publishInterval)) {
            tsPublish = millis();
            return publishAll;   
        }
    } else {
        if ((millis()-tsForcePublish)>(1000*VdmConfig.configFlash.protConfig.publishInterval)) {
            #ifdef EnvDevelop
                 UART_DBG.println("force publish");
            #endif
            tsForcePublish = millis();
            return publishAll;     
        }
        forcePublish = false;
        if ((millis()-tsPublish)>(1000*VdmConfig.configFlash.protConfig.minBrokerDelay)) {
            tsPublish = millis();
            // common
            if (VdmSystem.systemMessage.length()>0) result = publishCommon; 
            if (lastCommonValues.heatControl!=VdmConfig.heatValues.heatControl) result = publishCommon; 
            if (lastCommonValues.parkingPosition!=VdmConfig.heatValues.parkPosition) result = publishCommon; 
            if (lastCommonValues.systemState!=VdmSystem.systemState) result = publishCommon;
            String upTime = VdmSystem.getUpTime();
            if (lastCommonValues.upTime!=upTime) result = publishCommon;
        }
        // valves
        for (i=0;i<ACTUATOR_COUNT;i++) {
            lastValveValues[i].publishNow=false;
            if (VdmConfig.configFlash.valvesConfig.valveConfig[i].active) {
                if ((millis()-lastValveValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.minBrokerDelay)) {
                    if ((lastValveValues[i].position!=StmApp.actuators[i].actual_position) || 
                        (lastValveValues[i].target!=StmApp.actuators[i].target_position) ||
                        (lastValveValues[i].state!=StmApp.actuators[i].state) ||
                        (lastValveValues[i].meanCurrent!=StmApp.actuators[i].meancurrent) ||
                        (lastValveValues[i].temp1!=StmApp.actuators[i].temp1) ||
                        (lastValveValues[i].temp2!=StmApp.actuators[i].temp2) ||
                        ((millis()-lastValveValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.publishInterval))) {
                            result|=publishValves;
                            lastValveValues[i].publishNow=true;
                            if ((millis()-lastValveValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.publishInterval))
                                lastValveValues[i].publishTimeOut=true;
                    }
                }
            }
        } 
        // temps
        for (i=0;i<TEMP_SENSORS_COUNT;i++) {
            lastTempValues[i].publishNow=false;
            if (VdmConfig.configFlash.tempsConfig.tempConfig[i].active) {
                if ((millis()-lastTempValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.minBrokerDelay)) {
                    if ((lastTempValues[i].temperature!=StmApp.temps[i].temperature) ||
                        ((millis()-lastTempValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.publishInterval))) {
                            result|=publishTemps;
                            lastTempValues[i].publishNow=true;
                    } 
                }
            }
        }
        // volts
        for (i=0;i<VOLT_SENSORS_COUNT;i++) {
            lastVoltValues[i].publishNow=false;
         //   if (VdmConfig.configFlash.tempsConfig.tempConfig[i].active) {
                if ((millis()-lastVoltValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.minBrokerDelay)) {
                    if ((lastVoltValues[i].vad!=StmApp.volts[i].vad) ||
                        ((millis()-lastVoltValues[i].ts)>(1000*VdmConfig.configFlash.protConfig.publishInterval))) {
                            result|=publishVolts;
                            lastVoltValues[i].publishNow=true;
                    } 
                }
           // }
        }
    }
    return result;
}

void CMqtt::mqtt_loop() 
{
    if (!mqtt_client.connected()) {
        firstPublish=true;
        reconnect();      
    }
    if (mqtt_client.connected()) {
        mqtt_client.loop();
        if (VdmConfig.configFlash.protConfig.publishInterval<2) VdmConfig.configFlash.protConfig.publishInterval=2;
        uint8_t check=checkForPublish();

        switch (hadState) {
            case HAD_IDLE : {
                if (check!=0) { 
                    firstPublish=false;
                    publish_all(check);
                }   
                break; 
            }
            case HAD_STARTED : {
                hadState = HAD_INPROGRSS;
                executeDiscoveryHA();
                break;
            }
            case HAD_INPROGRSS : {
                break;
            }
            case HAD_FINISHED : {
                hadState = HAD_IDLE;
                break;
            }
        }
       
        messengerSend=false;
        connectTimeout=millis();
    } else {
        if ((VdmConfig.configFlash.messengerConfig.reason.reasonFlags.mqttTimeOut) && (!messengerSend)){
            if ((millis()-connectTimeout)>((uint32_t)60*1000*VdmConfig.configFlash.messengerConfig.reason.mqttTimeOutTime)) {
                String title = String(VdmConfig.configFlash.systemConfig.stationName) + " : MQTT" ;
                String s = "MQTT connect failed after "+String(VdmConfig.configFlash.messengerConfig.reason.mqttTimeOutTime)+" minutes";
                Messenger.sendMessage (title.c_str(),s.c_str());
                messengerSend=true;
            }  
        }
    }
        
    mqttConnected=mqtt_client.connected();
    mqttState=mqtt_client.state();
    /*
        -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
        -3 : MQTT_CONNECTION_LOST - the network connection was broken
        -2 : MQTT_CONNECT_FAILED - the network connection failed
        -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
         0 : MQTT_CONNECTED - the client is connected
         1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
         2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
         3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
         4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
         5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
    */
   checktValueTimeOut();
}

void CMqtt::disconnect() {
     mqtt_client.disconnect();
} 

void CMqtt::subscribe (char* topicstr, char* thisTopic, uint8_t size, bool publishValue)
{
    strncat(topicstr, thisTopic,size - strlen (topicstr) - 1);
    uint8_t len = strlen(topicstr);
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishSeparate) {
        strncat(topicstr, "/set",size - strlen (topicstr) - 1);   
    } 
    mqtt_client.subscribe(topicstr);  
    //UART_DBG.println("subscribe "+String(topicstr));
}

void CMqtt::reconnect() 
{
    topicsReceived=false;
    char topicstr[MAINTOPIC_LEN+80];
    char nrstr[21];
    char* mqttUser = NULL;
    char* mqttPwd = NULL;
    uint8_t len;

    tsPublish = millis();
    tsForcePublish = millis();
    if ((strlen(VdmConfig.configFlash.protConfig.userName)>0) && (strlen(VdmConfig.configFlash.protConfig.userPwd)>0)) {
        mqttUser = VdmConfig.configFlash.protConfig.userName;
        mqttPwd = VdmConfig.configFlash.protConfig.userPwd;
    }
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
        syslog.log(LOG_DEBUG, "MQTT reconnecting ...");
    }
    #ifdef EnvDevelop
        UART_DBG.println("Reconnecting MQTT...");
    #endif
    
    mqtt_client.setKeepAlive(VdmConfig.configFlash.protConfig.keepAliveTime);
    mqtt_client.setSocketTimeout(VdmConfig.configFlash.protConfig.keepAliveTime);

    if (!mqtt_client.connect(stationName,mqttUser,mqttPwd)) {
        #ifdef EnvDevelop
            UART_DBG.print("failed, rc=");
            UART_DBG.print(mqtt_client.state());
            UART_DBG.println(" retrying");
        #endif
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
            syslog.log(LOG_ERR, "MQTT failed rc="+String(mqtt_client.state())+String(" retrying"));
        }
        mqttState=mqtt_client.state();
        VdmTask.yieldTask(5000);
        return;
    }
    
    mqtt_client.setKeepAlive(VdmConfig.configFlash.protConfig.keepAliveTime);
    mqtt_client.setSocketTimeout(VdmConfig.configFlash.protConfig.keepAliveTime);

    // make some subscriptions
    memset(topicstr,0x0,sizeof(topicstr));
    strncat(topicstr,mqtt_commonTopic,sizeof(topicstr) - strlen (topicstr) - 1);
    len = strlen(topicstr);
    subscribe (topicstr, (char*) "heatControl",sizeof(topicstr),true);   
    
    topicstr[len] = '\0';
    subscribe (topicstr, (char*) "parkPosition",sizeof(topicstr),true);
  
    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        lastValveValues[x].ts=millis();
        lastValveValues[x].publishNow=false;
        lastValveValues[x].publishTimeOut=false;
        if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) {
            memset(topicstr,0x0,sizeof(topicstr));
            memset(nrstr,0x0,sizeof(nrstr));
            itoa((x+1), nrstr, 10);
            if (strlen(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)>0) {
                strncpy(nrstr,VdmConfig.configFlash.valvesConfig.valveConfig[x].name,sizeof(nrstr));
                replace (nrstr,strlen(nrstr),' ','_');
            }
            // prepare prefix
            strncat(topicstr, mqtt_valvesTopic,sizeof(topicstr) - strlen (topicstr) - 1);
            strncat(topicstr, nrstr,sizeof(topicstr) - strlen (topicstr) - 1);      
            len = strlen(topicstr);

            // target value
            subscribe (topicstr, (char*) "/target",sizeof(topicstr),true);
          
            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active) {
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].valueSource==0) {
                        // temp value
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/tValue",sizeof(topicstr));  
                    } 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].targetSource==0) {
                        // temp target
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/tTarget",sizeof(topicstr));
                    } 
                    // temp dyn offset
                    topicstr[len] = '\0';
                    subscribe (topicstr, (char*) "/control/dynOffs",sizeof(topicstr));
                }
                // window state
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.windowInstalled) {
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/window/state",sizeof(topicstr));
                        // window target (%)
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/window/target",sizeof(topicstr));
                    }
                }
                 // control active
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    topicstr[len] = '\0';
                    subscribe (topicstr, (char*) "/control/mode",sizeof(topicstr));              
                }
            }
        }
        lastValveValues[x].lasttValuets=millis();
    }
    for (uint8_t x = 0;x<TEMP_SENSORS_COUNT;x++) {
        lastTempValues[x].ts=millis();
        lastTempValues[x].publishNow=false;
    }

    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
        syslog.log(LOG_DEBUG, "MQTT Connected...");
    }
}

bool CMqtt::checkTopicName(char* topic,char* ref,bool set)
{
    char refSet[100] = {0};
    char* topic1 = topic;
    char* ref1 = ref;
    uint8_t len;
    if (topic1[0]=='/') topic1++;
    if (ref1[0]=='/') ref1++;
    
    if (strlen(topic1)==0) return false;
    
    strncpy(refSet,ref1, sizeof(refSet));
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishSeparate) {
        strncat(refSet, "/set",sizeof(refSet) - strlen (refSet) - 1);    
    }
    
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        syslog.log(LOG_DEBUG, "MQTT: checkTopic "+String(topic1)+" : "+String(refSet));
    }

    len = (strlen(topic1)>=strlen(refSet)) ? strlen(topic1) : strlen(refSet);
    if (strncmp(topic1,refSet,len)==0) {
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
            syslog.log(LOG_DEBUG, "MQTT: checkTopic found");
        }
        return true; 
    }

    // add /set for iobroker if separate topics used
    strncat(refSet, "/set",sizeof(refSet) - strlen (refSet) - 1);
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        syslog.log(LOG_DEBUG, "MQTT: checkTopic (/set) "+String(topic1)+" : "+String(refSet));
    }

    len = (strlen(topic1)>=strlen(refSet)) ? strlen(topic1) : strlen(refSet);
    if (strncmp(topic1,refSet,len)==0) {
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
            syslog.log(LOG_DEBUG, "MQTT: checkTopic (/set) found");
        }
        return true;   
    }    
    
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                syslog.log(LOG_DEBUG, "MQTT: checkTopic not found");
    }
    return (false);
}

bool CMqtt::checkTopicPath(char* topic,char* ref)
{
    char* topic1 = topic;
    char* ref1 = ref;
    uint8_t len;

    if (topic1[0]=='/') topic1++;
    if (ref1[0]=='/') ref1++;
    return (memcmp((const char*)topic1,(const char*) ref1, strlen(ref1))==0);
}

void CMqtt::callback(char* topic, byte* payload, unsigned int length) 
{
    bool found;
    char item[50];
    char* pt;
    char* pRef;
    uint8_t i;
    uint8_t idx;
    uint8_t val8;
    bool topicFound = false;
    char value[32] = {0};
    char rbName[11] = {0};

    // local zero terminated copy of payload
    memcpy(value, payload, std::min<size_t>(sizeof(value) - 1, length));
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
               syslog.log(LOG_DEBUG, "MQTT: callback "+String(topic)+" : "+String(value));
    }
    // UART_DBG.println("callback "+String (topic) +":"+String(value));
  
    if (length>0) {
        if (topic[0]=='/') {
            topic++;        // adjust topic 
        }
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
            syslog.log(LOG_DEBUG, "MQTT: check callback common "+String(topic)+" : "+String(mqtt_commonTopic));
        }
        if (checkTopicPath(topic,mqtt_commonTopic)) {
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                syslog.log(LOG_DEBUG, "MQTT: callback common found");
            }
            memset(item,0x0,sizeof(item));
            pt= (char*) topic;
            pRef=mqtt_commonTopic;
            if (pRef[0]=='/') pRef++;
            pt+=strlen(pRef);
            topicFound = checkTopicName(pt,(char*)"/heatControl");
            if (topicFound) {
                val8=VdmConfig.heatValues.heatControl;
                if (isFloat(value)) {
                    val8=atoi(value);
                } else {
                    if (strncmp(value,"manual",sizeof("manual"))==0) val8=0;
                    if (strncmp(value,"heat",sizeof("heat"))==0) val8=1;
                    if (strncmp(value,"cool",sizeof("cool"))==0) val8=2;
                    if (strncmp(value,"off",sizeof("off"))==0) val8=3;
                }
                VdmConfig.heatValues.heatControl=val8;             
            } 
            topicFound = checkTopicName(pt,(char*) "/parkPosition");
            if (topicFound) {
                val8=atoi(value); 
                VdmConfig.heatValues.parkPosition=val8;
            }    
        }
        else { 
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                syslog.log(LOG_DEBUG, "MQTT: check callback valve "+String(topic)+" : "+String(mqtt_valvesTopic));
            }
            if (checkTopicPath(topic,mqtt_valvesTopic)) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "MQTT: callback valves found");
                }
                memset(item,0x0,sizeof(item));
                pt= (char*) topic;
                //pt+= strlen(mqtt_valvesTopic);
                pRef=mqtt_valvesTopic;
                if (pRef[0]=='/') pRef++;
                pt+=strlen(pRef);
                idx=0;
                for (i=strlen(mqtt_valvesTopic);i<strlen(topic);i++) {
                    if (*pt=='/') break;
                    item[idx]=*pt;
                    idx++;
                    pt++;
                } 
                
                // find approbiated valve
                idx=0;
                found = false;
                for (i=0;i<ACTUATOR_COUNT;i++) {
                    strncpy(rbName,VdmConfig.configFlash.valvesConfig.valveConfig[i].name,sizeof(VdmConfig.configFlash.valvesConfig.valveConfig[i].name));
                    replace (rbName,strlen(rbName),' ','_');
                    if (strncmp(rbName,item,sizeof(rbName))==0) {
                        found = true;
                        break;
                    }
                    idx++;    
                }
                if (!found) {
                    if (isNumber(item)) {
                        idx=atoi(item)-1;
                        found=true;
                    }
                }

                if (found) {
                // 'set' is added for iobroker different topic for read and write
                    if (isFloat(value)) {
                        if (checkTopicName(pt,(char*) "/target")) {
                            if (VdmConfig.configFlash.valvesConfig.valveConfig[idx].active) {
                                StmApp.actuators[idx].target_position = atoi(value);
                            }
                        } else if (checkTopicName(pt,(char*) "/tValue")) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.active) {
                                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].valueSource==0) {
                                    PiControl[idx].value=strtof(value, NULL);
                                    PiControl[idx].failed=false;
                                    valveStates[idx].tValueFailed=false;
                                    valveStates[idx].messengerSent=false;
                                    mqttReceived=true;
                                    lastValveValues[idx].lasttValuets=millis();
                                }
                            }
                        } else if (checkTopicName(pt,(char*) "/tTarget")) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.active) {
                                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].targetSource==0) {
                                    PiControl[idx].target=strtof(value, NULL);
                                    mqttReceived=true;
                                }
                            }
                        } else if (checkTopicName(pt,(char*) "/control/dynOffs")) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.active) {
                                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].targetSource==0)
                                    PiControl[idx].dynOffset=atoi(value);
                            }
                        } 
                        else if (checkTopicName(pt,(char*) "/window/target")) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].link==0) {
                                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.active) {
                                        PiControl[idx].windowOpenTarget=atoi(value);
                                }
                            }
                        }
                        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                            syslog.log(LOG_DEBUG, "MQTT: found topic "+String(item)+String(pt)+" : "+String(value));
                        }
                    } else {
                        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                            syslog.log(LOG_DEBUG, "MQTT: found topic, but not a number "+String(item)+String(pt)+" : "+String(value));
                        }  
                    }
                    // handling for plain text
                    if (checkTopicName(pt,(char*) "/control/mode")) { 
                        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].link==0) {
                            if (isFloat(value)) {
                                val8=atoi(value);
                                PiControl[idx].controlActive=bool(val8);
                            } else {
                                if (strncmp(value,"off",3)==0)  PiControl[idx].setControlActive(0); 
                                if (strncmp(value,"auto",4)==0)  PiControl[idx].setControlActive(1);
                            } 
                            if (PiControl[idx].controlActive==0) PiControl[idx].setPosition(0);    
                        }
                    }
                    if (checkTopicName(pt,(char*) "/window/state")) { 
                        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].link==0) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.active) {
                                if (isFloat(value)) {
                                    PiControl[idx].setWindowAction(atoi(value));
                                } else {
                                    if (strncmp(value,"close",5)==0)  PiControl[idx].setWindowAction(0);
                                    if (strncmp(value,"open",4)==0)  PiControl[idx].setWindowAction(1);
                                }       
                            }
                        }
                    }
                } else {
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                        syslog.log(LOG_DEBUG, "MQTT: not found topic "+String(item));
                    }   
                }
            }
        }
    }
    if (!topicsReceived && found) topicsReceived=true; 
}

void CMqtt::publishValue (char* topicstr, char* valstr) 
{
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishSeparate) {
        strncat(topicstr, "/value",sizeof(topicstr) - strlen (topicstr) - 1);
    }
    mqtt_client.publish(topicstr, valstr,VdmConfig.configFlash.protConfig.protocolFlags.publishRetained);
}

void CMqtt::publish_valves () {
    char topicstr[MAINTOPIC_LEN+50];
    char nrstr[21];
    char valstr[50];
    int8_t tempIdx;
    uint8_t len;
    String s;
   
    const char valveStatesStr[10][11] =  {"","idle","opens","closes","failed","unknown","no valve","full open","connected","blocked"};

    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        valveStates[x].thisState=StmApp.actuators[x].state;
        if (valveStates[x].tValueFailed) valveStates[x].thisState = STATE_FAILED;
        if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) {
            if (lastValveValues[x].publishNow || forcePublish) {
                memset(topicstr,0x0,sizeof(topicstr));
                memset(nrstr,0x0,sizeof(nrstr));
                itoa((x+1), nrstr, 10);
                if (strlen(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)>0) {
                    strncpy(nrstr,VdmConfig.configFlash.valvesConfig.valveConfig[x].name,sizeof(nrstr));
                    replace (nrstr,strlen(nrstr),' ','_');
                }
                // prepare prefix
                strncat(topicstr, mqtt_valvesTopic,sizeof(topicstr) - strlen (topicstr) - 1);
                strncat(topicstr, nrstr,sizeof(topicstr) - strlen (topicstr) - 1);
                len = strlen(topicstr);
                // target
                if ((lastValveValues[x].target!=StmApp.actuators[x].target_position) || forcePublish || lastValveValues[x].publishTimeOut) {
                    topicstr[len] = '\0';
                    strncat(topicstr, "/target",sizeof(topicstr) - strlen (topicstr) - 1);
                    itoa(StmApp.actuators[x].target_position, valstr, 10);
                    publishValue(topicstr, valstr);
                    lastValveValues[x].target=StmApp.actuators[x].target_position;
                }
                // state
                if ((lastValveValues[x].state!=valveStates[x].thisState) || forcePublish || lastValveValues[x].publishTimeOut) {
                    topicstr[len] = '\0';
                    strncat(topicstr, "/state",sizeof(topicstr) - strlen (topicstr) - 1);
                    if (VdmConfig.configFlash.protConfig.protocolFlags.publishPlainText) {
                        if (valveStates[x].thisState<10) {
                            strcpy(valstr,valveStatesStr[valveStates[x].thisState]);
                        } else strcpy(valstr,"");
                    } else {
                        itoa(valveStates[x].thisState, valstr, 10);        
                    }
                    publishValue(topicstr, valstr);

                    if (StmApp.actuators[x].calibration) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/calibration/date",sizeof(topicstr) - strlen (topicstr) - 1);
                        publishValue(topicstr, (char*) VdmSystem.localTime().c_str());
                    }

                    if ((VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link>0) && (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link<13)) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/control/link",sizeof(topicstr) - strlen (topicstr) - 1);
                        strcpy(valstr,VdmConfig.configFlash.valvesConfig.valveConfig[VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link-1].name);
                        publishValue(topicstr, valstr);
                    }
                    lastValveValues[x].state=valveStates[x].thisState;
                }
                if (VdmConfig.configFlash.protConfig.protocolFlags.publishSeparate) {
                    // tTarget
                    if ((lastValveValues[x].tTarget!=PiControl[x].target) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/tTarget",sizeof(topicstr) - strlen (topicstr) - 1);
                        publishValue(topicstr, (char*) (String(PiControl[x].target,1)).c_str());
                        lastValveValues[x].tTarget=PiControl[x].target;
                    }
                    // tTarget
                    if ((lastValveValues[x].tValue!=PiControl[x].value) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/tValue",sizeof(topicstr) - strlen (topicstr) - 1);
                        publishValue(topicstr, (char*) (String(PiControl[x].value,1)).c_str());
                        lastValveValues[x].tValue=PiControl[x].value;
                    }
                }
                // calibration repetitions
                if ((lastValveValues[x].calibRetries!=StmApp.actuators[x].calibRetries) || forcePublish || lastValveValues[x].publishTimeOut) {
                    topicstr[len] = '\0';
                    strncat(topicstr, "/calibration/repetitions",sizeof(topicstr) - strlen (topicstr) - 1);
                    itoa(StmApp.actuators[x].calibRetries, valstr, 10);
                    publishValue(topicstr, valstr);
                    lastValveValues[x].calibRetries=StmApp.actuators[x].calibRetries;
                }
                // diag
                if (VdmConfig.configFlash.protConfig.protocolFlags.publishDiag) {
                    // meancurrent
                    if ((lastValveValues[x].meanCurrent!=StmApp.actuators[x].meancurrent) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/diag/meanCurrrent",sizeof(topicstr) - strlen (topicstr) - 1);
                        itoa(StmApp.actuators[x].meancurrent, valstr, 10);
                        publishValue(topicstr, valstr);
                        lastValveValues[x].meanCurrent=StmApp.actuators[x].meancurrent;
                    }
                    // counts
                    if ((lastValveValues[x].openCount!=StmApp.actuators[x].opening_count) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/diag/openCount",sizeof(topicstr) - strlen (topicstr) - 1);
                        itoa(StmApp.actuators[x].opening_count, valstr, 10);
                        publishValue(topicstr, valstr);
                        lastValveValues[x].openCount=StmApp.actuators[x].opening_count;
                    }
                    if ((lastValveValues[x].closeCount!=StmApp.actuators[x].closing_count) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/diag/closeCount",sizeof(topicstr) - strlen (topicstr) - 1);
                        itoa(StmApp.actuators[x].closing_count, valstr, 10);
                        publishValue(topicstr, valstr);
                        lastValveValues[x].closeCount=StmApp.actuators[x].closing_count;
                    }
                    if ((lastValveValues[x].deadzoneCount!=StmApp.actuators[x].deadzone_count) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/diag/deadZoneCount",sizeof(topicstr) - strlen (topicstr) - 1);
                        itoa(StmApp.actuators[x].deadzone_count, valstr, 10);
                        publishValue(topicstr, valstr);
                        lastValveValues[x].deadzoneCount=StmApp.actuators[x].deadzone_count;
                    }
                    // moves
                    if ((lastValveValues[x].movements!=StmApp.actuators[x].movements) || forcePublish || lastValveValues[x].publishTimeOut) {
                        topicstr[len] = '\0';
                        strncat(topicstr, "/diag/moves",sizeof(topicstr) - strlen (topicstr) - 1);
                        itoa(StmApp.actuators[x].movements, valstr, 10);
                        publishValue(topicstr, valstr);
                        lastValveValues[x].movements=StmApp.actuators[x].movements;
                    }
                }

                // temperature 1st sensor
                if ((StmApp.stmInitState==STM_INIT_FINISHED) && StmApp.oneWireAllRead) {
                    if (StmApp.actuators[x].tIdx1>0) { 
                        if ((lastValveValues[x].temp1!=StmApp.actuators[x].temp1) || forcePublish || lastValveValues[x].publishTimeOut) {
                            topicstr[len] = '\0';
                            strncat(topicstr, "/temp1",sizeof(topicstr) - strlen (topicstr) - 1);
                            if (StmApp.actuators[x].temp1>-500) {
                                s = String(((float)StmApp.actuators[x].temp1)/10,1); 
                                if (VdmConfig.configFlash.protConfig.mqttConfig.flags.numFormat==numFormatGer) s.replace('.',',');
                            } else s="failed";
                            publishValue(topicstr, (char*) &s);
                            lastValveValues[x].temp1=StmApp.actuators[x].temp1;
                        }
                    }
                    // temperature 2nd sensor
                    if (StmApp.actuators[x].tIdx2>0) {
                        if ((lastValveValues[x].temp2!=StmApp.actuators[x].temp2) || forcePublish || lastValveValues[x].publishTimeOut) {
                            topicstr[len] = '\0';
                            strncat(topicstr, "/temp2",sizeof(topicstr) - strlen (topicstr) - 1);
                            if (StmApp.actuators[x].temp2>-500) {
                                s = String(((float)StmApp.actuators[x].temp2)/10,1); 
                                if (VdmConfig.configFlash.protConfig.mqttConfig.flags.numFormat==numFormatGer) s.replace('.',',');
                            } else s="failed";
                            publishValue(topicstr, (char*) &s);
                            lastValveValues[x].temp2=StmApp.actuators[x].temp2;
                        }
                    }
                }
                lastValveValues[x].publishNow=false;
                lastValveValues[x].publishTimeOut=false;
                lastValveValues[x].ts=millis();
            }
        }
    }
}

void CMqtt::publish_temps()
{
    char topicstr[MAINTOPIC_LEN+50];
    char nrstr[21];
    int8_t tempIdx;
    uint8_t len;
    String s;
    
    if ((StmApp.stmInitState==STM_INIT_FINISHED) && StmApp.oneWireAllRead) {
        for (uint8_t x = 0;x<StmApp.tempsCount;x++) {
            if (lastTempValues[x].publishNow || forcePublish) {
                tempIdx=StmApp.findTempID(StmApp.temps[x].id);
                if (tempIdx>=0) {
                    if (VdmConfig.configFlash.tempsConfig.tempConfig[tempIdx].active) {
                        if ((StmApp.findTempIdxInValve (tempIdx)<0) || VdmConfig.configFlash.protConfig.protocolFlags.publishAllTemps) {
                            memset(topicstr,0x0,sizeof(topicstr));
                            memset(nrstr,0x0,sizeof(nrstr));
                            itoa((x+1), nrstr, 10);
                            if (strlen(VdmConfig.configFlash.tempsConfig.tempConfig[tempIdx].name)>0) {
                                strncpy(nrstr,VdmConfig.configFlash.tempsConfig.tempConfig[tempIdx].name,sizeof(nrstr));
                                replace (nrstr,strlen(nrstr),' ','_');
                            }
                            // prepare prefix
                            strncat(topicstr, mqtt_tempsTopic,sizeof(topicstr) - strlen (topicstr) - 1);
                            strncat(topicstr, nrstr,sizeof(topicstr) - strlen (topicstr) - 1);
                            len = strlen(topicstr);
                            // id
                            strncat(topicstr, "/id",sizeof(topicstr) - strlen (topicstr) - 1);     
                            publishValue(topicstr, StmApp.temps[x].id);
                            // actual value
                            topicstr[len] = '\0';
                            strncat(topicstr, "/value",sizeof(topicstr) - strlen (topicstr) - 1);
                            
                            if (StmApp.temps[x].temperature<=-500) {
                                s = "failed";  
                            } else {
                                s = String(((float)StmApp.temps[x].temperature)/10,1); 
                                if (VdmConfig.configFlash.protConfig.mqttConfig.flags.numFormat==numFormatGer) s.replace('.',','); 
                            }
                            publishValue(topicstr, (char*) &s);
                        }
                    }
                }
                lastTempValues[x].temperature=StmApp.temps[x].temperature;
                lastTempValues[x].publishNow=false;
                lastTempValues[x].ts=millis();
            }
        }
    }
} 

void CMqtt::publish_volts()
{
    char topicstr[MAINTOPIC_LEN+50];
    char nrstr[21];
    int8_t voltIdx;
    uint8_t len;
    String s;
    
    if ((StmApp.stmInitState==STM_INIT_FINISHED) && StmApp.oneWireAllRead) {
        for (uint8_t x = 0;x<StmApp.voltsCount;x++) {
            if (lastVoltValues[x].publishNow || forcePublish) {
                voltIdx=StmApp.findVoltID(StmApp.volts[x].id);
                if (voltIdx>=0) {
                    memset(topicstr,0x0,sizeof(topicstr));
                    memset(nrstr,0x0,sizeof(nrstr));
                    itoa((x+1), nrstr, 10);
                    if (strlen(VdmConfig.configFlash.voltsConfig.voltConfig[voltIdx].name)>0) {
                        strncpy(nrstr,VdmConfig.configFlash.voltsConfig.voltConfig[voltIdx].name,sizeof(nrstr));
                        replace (nrstr,strlen(nrstr),' ','_');
                    }
                    // prepare prefix
                    strncat(topicstr, mqtt_voltsTopic,sizeof(topicstr) - strlen (topicstr) - 1);
                    strncat(topicstr, nrstr,sizeof(topicstr) - strlen (topicstr) - 1);
                    len = strlen(topicstr);
                    // id
                    strncat(topicstr, "/id",sizeof(topicstr) - strlen (topicstr) - 1);     
                    publishValue(topicstr, StmApp.volts[voltIdx].id);
                    // actual value
                    topicstr[len] = '\0';
                    strncat(topicstr, "/value",sizeof(topicstr) - strlen (topicstr) - 1);
                    if (StmApp.volts[voltIdx].failed) s="failed"; 
                    else
                    s = String(StmApp.volts[voltIdx].value,3); 
                    if (VdmConfig.configFlash.protConfig.mqttConfig.flags.numFormat==numFormatGer) s.replace('.',',');    
                    publishValue(topicstr, (char*) &s);
                    // unit
                    topicstr[len] = '\0';
                    strncat(topicstr, "/unit",sizeof(topicstr) - strlen (topicstr) - 1); 
                    publishValue(topicstr, VdmConfig.configFlash.voltsConfig.voltConfig[voltIdx].unit);
                }
            }
            lastVoltValues[x].vad=StmApp.volts[x].vad;
            lastVoltValues[x].publishNow=false;
            lastVoltValues[x].ts=millis();
        }
    }
} 


void CMqtt::publish_common () 
{
    char topicstr[MAINTOPIC_LEN+50];
    char nrstr[21];
    char valstr[50];
    int8_t tempIdx;
    uint8_t len;
    const char heatControlStates[4][11] =  {"manual","heat","cool","off"};
    const char systemStates[3][11] =  {"ok","info","error"};

    memset(topicstr,0x0,sizeof(topicstr));
    strncat(topicstr,mqtt_commonTopic,sizeof(topicstr) - strlen (topicstr) - 1);
    len = strlen(topicstr);
   
    if ((!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) || forcePublish || (lastCommonValues.heatControl!=VdmConfig.heatValues.heatControl)) {
        strncat(topicstr, "heatControl",sizeof(topicstr) - strlen (topicstr) - 1);
        if (VdmConfig.configFlash.protConfig.protocolFlags.publishPlainText) {
            if (VdmConfig.heatValues.heatControl<4) {
                strcpy(valstr,heatControlStates[VdmConfig.heatValues.heatControl]);
            } else strcpy(valstr,"");
        } else {
            itoa(VdmConfig.heatValues.heatControl, valstr, 10);        
        }
        publishValue(topicstr, valstr);
        lastCommonValues.heatControl=VdmConfig.heatValues.heatControl; 
    }

    topicstr[len] = '\0';
    if ((!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) || forcePublish || (lastCommonValues.parkingPosition!=VdmConfig.heatValues.parkPosition)) {
        strncat(topicstr, "parkPosition",sizeof(topicstr) - strlen (topicstr) - 1);
        itoa(VdmConfig.heatValues.parkPosition, valstr, 10);         
        publishValue(topicstr, valstr);
        lastCommonValues.parkingPosition=VdmConfig.heatValues.parkPosition; 
    }
    topicstr[len] = '\0';
    
    if ((!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) || forcePublish || (lastCommonValues.systemState!=VdmSystem.systemState)) {
        strncat(topicstr, "state",sizeof(topicstr) - strlen (topicstr) - 1);
        if (VdmConfig.configFlash.protConfig.protocolFlags.publishPlainText) {
            if (VdmSystem.systemState<3) {
                strcpy(valstr,systemStates[VdmSystem.systemState]);
            } else strcpy(valstr,"");
        } else {
            itoa(VdmSystem.systemState , valstr, 10);        
        }
        publishValue(topicstr, valstr);
        lastCommonValues.systemState=VdmSystem.systemState;
    }
    topicstr[len] = '\0';
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishUpTime) {
        strncat(topicstr, "uptime",sizeof(topicstr) - strlen (topicstr) - 1);
        String upTime = VdmSystem.getUpTime();
        if ((!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) || forcePublish || (lastCommonValues.upTime!=upTime)) {
            lastCommonValues.upTime = upTime;
            publishValue(topicstr, (char*) upTime.c_str());
        }
    }
    
    topicstr[len] = '\0';
    if ((!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) || forcePublish || (lastCommonValues.systemMessage!=VdmSystem.systemMessage)) {
        topicstr[len] = '\0';
        strncat(topicstr, "message",sizeof(topicstr) - strlen (topicstr) - 1);        
        publishValue(topicstr, (char*) VdmSystem.systemMessage.c_str());
        lastCommonValues.systemMessage=VdmSystem.systemMessage; 
    }
}
    
void CMqtt::publish_all (uint8_t publishFlags) 
{
   if (CHECK_BIT(publishFlags,0)==1) publish_common (); 
   if (CHECK_BIT(publishFlags,1)==1) publish_valves ();
   if (CHECK_BIT(publishFlags,2)==1) publish_temps ();
   if (CHECK_BIT(publishFlags,3)==1) publish_volts ();
}

void CMqtt::checktValueTimeOut () 
{
    bool messenger = false;
    if (VdmConfig.configFlash.protConfig.mqttConfig.flags.timeoutTSActive) 
    {
        for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
            if ((!PiControl[x].windowState) && (!valveStates[x].tValueFailed) && VdmConfig.configFlash.valvesConfig.valveConfig[x].active && VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active && PiControl[x].controlActive) 
            {
                if ((millis()-lastValveValues[x].lasttValuets)>(1000*60*VdmConfig.configFlash.protConfig.mqttConfig.timeOut)) {
                    PiControl[x].setFailed(VdmConfig.configFlash.protConfig.mqttConfig.toPos);
                    valveStates[x].tValueFailed=true;
                    if (VdmConfig.configFlash.messengerConfig.reason.reasonFlags.tValueFailed) messenger=true;
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                        syslog.log(LOG_DEBUG, "MQTT: tValue timeout: valve "+String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)+"(#"+String(x+1));
                    }  
                }
            }
            if (!(VdmConfig.configFlash.valvesConfig.valveConfig[x].active && PiControl[x].controlActive && !PiControl[x].windowState))
            {
                lastValveValues[x].lasttValuets=millis();
                valveStates[x].tValueFailed=false;
                valveStates[x].messengerSent=false;
                PiControl[x].failed=false;

            }
        }
        if (messenger)
        { 
            bool messengerToSend=false;
            String title = String(VdmConfig.configFlash.systemConfig.stationName) + " : MQTT" ;
            String s = "tValue not received for room\r\n";
            for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
                if (valveStates[x].tValueFailed && (!valveStates[x].messengerSent)) {
                    s+=String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)+"\r\n";
                    valveStates[x].messengerSent=true;
                    messengerToSend=true;
                }
            }
            if (messengerToSend) Messenger.sendMessage (title.c_str(),s.c_str());
        }
    }
}

void CMqtt::deleteDiscoveryHA() 
{
    String line;
    VdmSystem.openFile(HA_FILE_NAME,FS_READ_MODE);
    while(VdmSystem.fileAvailable()) { 
        line = VdmSystem.readlnFromFile();
        mqtt_client.publish((char*) line.c_str(),(const uint8_t*) "",0,true);
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
            syslog.log(LOG_DEBUG, "MQTT: discovery delete: "+line);
        }  
       // UART_DBG.println("discovery delete: "+String(line));
    }
    VdmSystem.closeFile();
}

void CMqtt::sendDiscoveryHA(HA_Item thisHAItem) 
{
    HA_Item haItem=thisHAItem;
    String root="";
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishPathAsRoot) root="/";
    haItem.unique_id = String(VdmConfig.configFlash.systemConfig.stationName)+"."+haItem.unique_id;
    
    if (haItem.state_topic.length()==0) haItem.state_topic = "null" ;
    else haItem.state_topic = "\""+String(root)+String(VdmConfig.configFlash.systemConfig.stationName)+"/"+haItem.state_topic+"/value"+"\"";
    
    if (haItem.command_topic.length()==0) haItem.command_topic = "null" ;
    else haItem.command_topic = "\""+String(root)+String(VdmConfig.configFlash.systemConfig.stationName)+"/"+haItem.command_topic+"/set"+"\"";
    if (haItem.icon.length()>0) haItem.icon = "\"icon\": \""+haItem.icon+"\",\n"; 
    if (haItem.unit_of_measurement.length()>0) haItem.unit_of_measurement = "\"unit_of_measurement\" : \""+haItem.unit_of_measurement+"\",\n";
    
    if (haItem.device_class.length()==0) haItem.device_class = "null" ;
    else haItem.device_class = "\""+haItem.device_class+"\"";
    if (haItem.state_class.length()==0) haItem.state_class = "null" ;
    else haItem.state_class = "\""+haItem.state_class+"\"";

    haItem.unique_id.replace(" ","_");

    if (haItem.options.length()>0) haItem.options = haItem.options+",\n"; 

    String json =  "{\"name\": \""+haItem.name+"\",\n" 
                    "\"unique_id\": \""+haItem.unique_id+"\",\n"
                    "\"state_topic\": "+haItem.state_topic+",\n"
                    "\"command_topic\": "+haItem.command_topic+",\n"
                    +haItem.icon+
                    "\"device_class\": "+haItem.device_class+",\n"
                    "\"state_class\": "+haItem.state_class+",\n"
                    +haItem.unit_of_measurement+
                    haItem.options+
                    "\"device\": {\n"
                        "\"identifiers\": \""+String(VdmConfig.configFlash.systemConfig.stationName)+"\",\n"
                        "\"name\": \""+String(VdmConfig.configFlash.systemConfig.stationName)+"\",\n"
                        "\"sw_version\": \""+String(FIRMWARE_VERSION)+"\",\n"
                        "\"hw_version\": \"2.0\",\n"
                        "\"model\": \"full\",\n"
                        "\"manufacturer\": \"Lenti84/Surfgargano\",\n"
                        "\"configuration_url\": \"http://"+VdmNet.networkInfo.ip.toString()+"/#config\"\n"
                        "}\n"
                    "}\n";


    haDiscoveryTopic = "homeassistant/"+haItem.topicClass+"/"+String(VdmConfig.configFlash.systemConfig.stationName)+"/"+String (haItem.topic)+"/config";  
   
    uint16_t bl = 20+haDiscoveryTopic.length()+json.length();
    uint16_t bs = mqtt_client.getBufferSize();
    if (bs<bl) mqtt_client.setBufferSize(bl);
    
    bool result=mqtt_client.publish((char*) haDiscoveryTopic.c_str(),(uint8_t *) json.c_str(),json.length(),true);
    VdmSystem.writelnToFile(String(haDiscoveryTopic)+"\n");
   
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        syslog.log(LOG_DEBUG, "MQTT: discovery topic: "+haDiscoveryTopic);
        syslog.log(LOG_DEBUG, "MQTT: discovery item: "+json);
    }  

    //UART_DBG.println("discovery topic : "+String(haDiscoveryTopic));
    //UART_DBG.println("discovery item: "+String(json.length())+"="+String(json));
    //UART_DBG.println("discovery result: "+String(result)+" bufferSize "+String(mqtt_client.getBufferSize()));
    delay(100);
}

void CMqtt::executeDiscoveryHA() 
{
    switch (actionHA) {
        case HA_DISCOVERY_ONLY:
        {
            executeDiscoveryHANow();
            break;
        }
        case HA_DELETE_ONLY:
        {
            deleteDiscoveryHA();
            break;
        }
        case HA_DELETE_AND_DISCOVERY:
        {   
            deleteDiscoveryHA();
            executeDiscoveryHANow();
            break;
        }
    }
    Mqtt.hadState=HAD_IDLE;
}

void CMqtt::executeDiscoveryHANow() 
{
    // StationName/
    /* common/
        heatControl/
            value
            set
        message/
            value
        parkPosition/
            value
            set
        state/
            value
        uptime/              [config]
            value
    */

    /* valves/
        valveName/           [config]
            calibration/
                repetitions/
                     value
            control/
                mode/
                    set
            diag/            [config]
                openCount/
                    value
                closeCount/
                    value
                deadZoneCount/
                    value
                meanCurrent/
                    value
                moves/
                    value
            window/          [config]
                state/
                    set
                target/
                    set
            dynOffs/
                set
            state/
                value
            tTarget/         [config]
                set
            tValue/          [config]
                set
            target/
                value
                set
            temp1/           [config]
                value
            temp2/           [config]
                value
    */

    /* temps/
        sensorName/          [config]
            id/
                value
            value/
                value
    */

    /* sensors/
        sensorName/          [config]
            id/
                value
            unit/
                value
            value/
                value
    */
   
/* Example
        Topic :
        homeassistant/sensor/VdMot/Schlafzimmer_Heizung/config
        JSON:    
        {"name": "Schlafzimmer Heizung",
        "unique_id": "VdMot.Schlafzimmer_Heizung",
        "state_topic": "VdMot/temps/wohn/value/value",
        "icon": "mdi:temperature-celsius",
        "device_class": "temperature",
        "state_class": "measurement",
        "unit_of_measurement" : "°C",
        "device": {
            "identifiers": "VdMot",
            "name": "VdMot",
            "sw_version": "1.4.0dev",
            "hw_version": "2.0",
            "model": "",
            "manufacturer": "Lenti84/Surfgargano",
            "configuration_url": "http://ip/#config"
            }
        }

*/

     
/*
    String topicClass;
    String topic;
    String name; 
    String unique_id;
    String state_topic;
    String command_topic;
    String icon;
    String options;
    String device_class;
    String state_class;
    String unit_of_measurement;
    String configuration_url;    
*/

static const HA_Item haCommonItems[] = {
        {"select","heatControl","heatControl","common.heatControl","common/heatControl","common/heatControl","mdi:heat-pump-outline","\"options\": [\"manual\",\"heat\",\"cool\",\"off\"]","","",""},
        {"text","message","message","common.message","common/message","common/message","mdi:message","","volume","measurement",""},
        {"number","parkPosition","parkPosition","common.parkPosition","common/parkPosition","common/parkPosition","mdi:parking","","volume","measurement",""},
        {"text","state","state","common.state","common/state","common/state","mdi:state-machine","","volume","measurement",""},
        {"text","uptime","uptime","common.uptime","common/uptime","common/uptime","mdi:timelapse","","volume","measurement",""},
        {""}
    };

static const HA_Item haClimateItems[] =  {
        {"climate","climate","climate","valves.climate","tValue","tTarget","mdi:thermostat","","","",""},
        {""}
    };

static const HA_Item haValvesItems[] =  {
        {"text","valves_state","state","valves.state","state","state","mdi:state-machine","","volume","measurement",""},
    //    {"number","valves_tTarget","tTarget","valves.tTarget","tTarget","tTarget","mdi:thermometer","","temperature","measurement","°C"},
    //    {"number","valves_tValue","tValue","valves.tValue","tValue","tValue","mdi:thermometer","","temperature","measurement","°C"},
        {"valve","valves_target","target","valves.target","target","target","mdi:valve","\"reports_position\": true","water","measurement","%"},
        {"sensor","valves_temp1","temp1","valves.temp1","temp1","","mdi:thermometer","","temperature","measurement","°C"},
        {"sensor","valves_temp2","temp2","valves.temp2","temp2","","mdi:thermometer","","temperature","measurement","°C"},
    //    {"select","valves_control_mode","control.mode","valves.control.mode","control/mode","control/mode","mdi:switch","\"options\": [\"auto\",\"off\"]","","",""},
        {"number","valves_control_dynOffs","valves.control.dynOffs","valves.control.dynOffs","control/dynOffs","control/dynOffs","mdi:gauge","","volume","measurement",""},
        {"select","valves_window_state","window.state","valves.window.state","window/state","window/state","mdi:switch","\"options\": [\"close\",\"open\"]","","",""},
        {"number","valves_window_target","window.target","valves.window.target","window/target","window/target","mdi:gauge","","volume","measurement",""},
        {"text","valves_calibration_date","calibration.date","valves.calibration.date","calibration/date","calibration/date","mdi:timelapse","","volume","measurement",""},
        {"text","valves_calibration_repetitions","calibration.repetitions","valves.calibration.repetitions","calibration/repetitions","calibration/repetitions","mdi:valve","","volume","measurement",""},
        {"text","valves_diag_openCount","diag.openCount","valves.diag.openCount","diag/openCount","diag/openCount","mdi:valve","","volume","measurement",""},
        {"text","valves_diag_closeCount","diag.closeCount","valves.diag.closeCount","diag/closeCount","diag/closeCount","mdi:valve","","volume","measurement",""},
        {"text","valves_diag_deadZoneCount","diag.deadZoneCount","valves.diag.deadZoneCount","diag/deadZoneCount","diag/deadZoneCount","mdi:valve","","volume","measurement",""},
        {"text","valves_diag_moves","diag.moves","valves.diag.moves","diag/moves","diag/moves","mdi:valve","","volume","measurement",""},
        {"text","valves_diag_meanCurrrent","diag.meanCurrrent","valves.diag.meanCurrrent","diag/meanCurrrent","diag/meanCurrrent","mdi:valve","","volume","measurement",""},
        {""}
    };
  
static const HA_Item haTempsItems[] = {
        {"sensor","temps","temps","temps","temps","","mdi:thermometer","","temperature","measurement","°C"},
        {""}
    };

static const HA_Item haVoltsItems[] = {
        {"sensor","volts","volts","volts","sensors","","","","voltage","measurement",""},
        {""}
    };

    uint8_t i;
    HA_Item haItem;
    String rbName;

    /* common/
        heatControl/
            value
            set
        message/
            value
        parkPosition/
            value
            set
        state/
            value
        uptime/              [config]
            value
    */

    VdmSystem.openFile(HA_FILE_NAME,FS_WRITE_MODE);

    // common
    i=0;
    do {
        haItem=haCommonItems[i];
        if (haItem.topic=="uptime") {
            if (VdmConfig.configFlash.protConfig.protocolFlags.publishUpTime) sendDiscoveryHA(haItem);
        } else if (haItem.topic=="heatControl"){
            if (!VdmConfig.configFlash.protConfig.protocolFlags.publishPlainText) {
                haItem.options="\"options\": [\"0\",\"1\",\"2\",\"3\"]";  
            }
            sendDiscoveryHA(haItem);  
        } 
        else sendDiscoveryHA(haItem);
        i++;
    }
    while (haCommonItems[i].topicClass!="");
  
   // climate
    String tcTopic;
    String ctTopic;
    String modeTopic;
    String root="";
    if (VdmConfig.configFlash.protConfig.protocolFlags.publishPathAsRoot) root="/";

    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) {
            i=0;
            
            haItem=haClimateItems[i];
            rbName = String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name);
            if (rbName.length() == 0) rbName = String(x+1); 
            rbName.replace(" ","_");
            haItem.topic=haItem.topic+"_"+rbName;
            haItem.name=String("climate.")+rbName+"."+haItem.name;
            haItem.unique_id=haItem.name;
            
            switch (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].valueSource){
                case 1 : {
                    haItem.state_topic = "temp1";
                    break;
                }
                case 2 : {
                    haItem.state_topic = "temp2";
                    break;
                }
            }
            tcTopic = "\""+String(root)+String(VdmConfig.configFlash.systemConfig.stationName)+
                      "/valves/"+rbName+"/"+haItem.command_topic+"/set";

            
            ctTopic = "\""+String(root)+String(VdmConfig.configFlash.systemConfig.stationName)+
                      "/valves/"+rbName+"/"+haItem.state_topic+"/value";


            modeTopic = "\""+String(root)+String(VdmConfig.configFlash.systemConfig.stationName)+
                      "/valves/"+rbName+"/control/mode/set";

            haItem.options = 
                "\"temperature_command_topic\":"+ String(tcTopic)+"\",\n"+  
                "\"current_temperature_topic\":" +String(ctTopic)+"\",\n"+ 
                "\"mode_command_topic\":" +String(modeTopic)+"\",\n"+ 
                "\"modes\" : [\"auto\", \"off\"]"; 

            haItem.state_topic = "";
            haItem.command_topic = "";

            sendDiscoveryHA(haItem);
        } 
    }

    // valves
    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) {
            i=0;
            do {
                haItem=haValvesItems[i];
                rbName = String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name);
                if (rbName.length() == 0) rbName = String(x+1); 
                rbName.replace(" ","_");
                haItem.topic=haItem.topic+"_"+rbName;
                haItem.name=String("valves.")+rbName+"."+haItem.name;
                haItem.unique_id=haItem.name;
                if (haItem.state_topic.length()>0)
                    haItem.state_topic="valves/"+rbName+"/"+haItem.state_topic;
                if (haItem.command_topic.length()>0)
                    haItem.command_topic="valves/"+rbName+"/"+haItem.command_topic;
                if (haValvesItems[i].topic=="valves_temp1") {
                    if (StmApp.actuators[x].tIdx1>0) sendDiscoveryHA(haItem);
                } else if (haValvesItems[i].topic=="valves_temp2") { 
                    if (StmApp.actuators[x].tIdx2>0) sendDiscoveryHA(haItem);
                } 
                else if (haValvesItems[i].topic=="valves_window_state") { 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active && VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.windowInstalled) {
                        if (!VdmConfig.configFlash.protConfig.protocolFlags.publishPlainText) {
                            haItem.topicClass="switch";
                            haItem.options="\"payload_on\": \"1\",\"payload_off\": \"0\",\"state_on\": \"1\",\"state_off\": \"0\"";    
                        }
                        sendDiscoveryHA(haItem);
                    }
                } 
                else if (haValvesItems[i].topic=="valves_window_target") { 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active && VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.windowInstalled) sendDiscoveryHA(haItem);
                } 
                else if (haValvesItems[i].topic=="valves_control_mode") { 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active) {
                        if (!VdmConfig.configFlash.protConfig.protocolFlags.publishPlainText) {
                            haItem.topicClass="switch";
                            haItem.options="\"payload_on\": \"1\",\"payload_off\": \"0\",\"state_on\": \"1\",\"state_off\": \"0\"";    
                        }
                        sendDiscoveryHA(haItem);
                    }
                } 
                else if (haValvesItems[i].topic=="valves_control_dynOffs") { 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active) sendDiscoveryHA(haItem);
                } 
                
                else if (haValvesItems[i].topic.indexOf("valves_diag")>=0) { 
                    if (VdmConfig.configFlash.protConfig.protocolFlags.publishDiag) sendDiscoveryHA(haItem);
                }
                else sendDiscoveryHA(haItem);
                i++;
            }
            while (haValvesItems[i].topicClass!="");
        }
    }
    
    //temp sensors
    for (uint8_t x = 0;x<TEMP_SENSORS_COUNT;x++) {
        if (VdmConfig.configFlash.tempsConfig.tempConfig[x].active) {
            i=0;
            do {
                haItem=haTempsItems[i];
                rbName = String(VdmConfig.configFlash.tempsConfig.tempConfig[x].name);
                if (rbName.length() == 0) rbName = String(x+1); 
                rbName.replace(" ","_");
                haItem.topic=haItem.topic+"_"+rbName;
                haItem.name=haItem.name+"."+rbName;
                haItem.unique_id=VdmConfig.configFlash.tempsConfig.tempConfig[x].ID;
                haItem.state_topic=haItem.state_topic+"/"+rbName+String("/value");
                if (haItem.unique_id.length()>0) sendDiscoveryHA(haItem);
                i++;
            }
            while (haTempsItems[i].topicClass!="");    
        }
    }
    // volt sensors
    for (uint8_t x = 0;x<VOLT_SENSORS_COUNT;x++) {
        if (VdmConfig.configFlash.voltsConfig.voltConfig[x].active) {
            i=0;
            do {
                haItem=haVoltsItems[i];
                rbName = String(VdmConfig.configFlash.voltsConfig.voltConfig[x].name);
                if (rbName.length() == 0) rbName = String(x+1); 
                rbName.replace(" ","_");
                haItem.topic=haItem.topic+"_"+rbName;
                haItem.name=haItem.name+"."+VdmConfig.configFlash.voltsConfig.voltConfig[x].name;
                haItem.unique_id=VdmConfig.configFlash.voltsConfig.voltConfig[x].ID;
                haItem.state_topic=haItem.state_topic+"/"+rbName+String("/value");
                haItem.unit_of_measurement = VdmConfig.configFlash.voltsConfig.voltConfig[x].unit;
                if (haItem.unique_id.length()>0) sendDiscoveryHA(haItem);
                i++;
            }
            while (haVoltsItems[i].topicClass!="");   
        }
    }
    VdmSystem.closeFile();
    hadState = HAD_FINISHED;
}
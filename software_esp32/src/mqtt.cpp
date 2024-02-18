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
    strncat(mqtt_tempsTopic, DEFAULT_TEMPSTOPIC,sizeof(mqtt_tempsTopic)- strlen (mqtt_tempsTopic) - 1);

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
        return publishCommon+publishValves+publishTemps;    
    }
    if (!VdmConfig.configFlash.protConfig.protocolFlags.publishOnChange) {
        if ((millis()-tsPublish)>(1000*VdmConfig.configFlash.protConfig.publishInterval)) {
            tsPublish = millis();
            return publishCommon+publishValves+publishTemps;    
        }
    } else {
        if ((millis()-tsForcePublish)>(1000*VdmConfig.configFlash.protConfig.publishInterval)) {
            #ifdef EnvDevelop
                 UART_DBG.println("force publish");
            #endif
            tsForcePublish = millis();
            return publishCommon+publishValves+publishTemps;    
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
       
        if (check!=0) { 
            firstPublish=false;
            publish_all(check);
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
       /* if (publishValue) {
            strncat(topicstr, "/value",size - strlen (topicstr) - 1);
            mqtt_client.subscribe(topicstr); 
            topicstr[len]=0;
        }*/
        strncat(topicstr, "/set",size - strlen (topicstr) - 1);   
    } 
    mqtt_client.subscribe(topicstr);  
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
    //strncat(topicstr, "heatControl",sizeof(topicstr) - strlen (topicstr) - 1);
    //mqtt_client.subscribe(topicstr);    
    
    topicstr[len] = '\0';
    subscribe (topicstr, (char*) "parkPosition",sizeof(topicstr),true);
    //strncat(topicstr, "parkPosition",sizeof(topicstr) - strlen (topicstr) - 1);
    //mqtt_client.subscribe(topicstr);    

    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        lastValveValues[x].ts=millis();
        lastValveValues[x].publishNow=false;
        lastValveValues[x].publishTimeOut=false;
        if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) {
            memset(topicstr,0x0,sizeof(topicstr));
            memset(nrstr,0x0,sizeof(nrstr));
            itoa((x+1), nrstr, 10);
            if (strlen(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)>0)
                strncpy(nrstr,VdmConfig.configFlash.valvesConfig.valveConfig[x].name,sizeof(nrstr));
            // prepare prefix
            strncat(topicstr, mqtt_valvesTopic,sizeof(topicstr) - strlen (topicstr) - 1);
            strncat(topicstr, nrstr,sizeof(topicstr) - strlen (topicstr) - 1);      
            len = strlen(topicstr);

            // target value
            subscribe (topicstr, (char*) "/target",sizeof(topicstr),true);
            //strncat(topicstr, "/target",sizeof(topicstr) - strlen (topicstr) - 1);
            //mqtt_client.subscribe(topicstr);

            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.active) {
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].valueSource==0) {
                        // temp value
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/tValue",sizeof(topicstr));
                        //strncat(topicstr, "/tValue",sizeof(topicstr) - strlen (topicstr) - 1);
                        //mqtt_client.subscribe(topicstr);    
                    } 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].targetSource==0) {
                        // temp target
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/tTarget",sizeof(topicstr));
                        //strncat(topicstr, "/tTarget",sizeof(topicstr) - strlen (topicstr) - 1);
                        //mqtt_client.subscribe(topicstr);    
                    } 
                }
                // window state
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].controlFlags.windowInstalled) {
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/window/state",sizeof(topicstr));
                        //strncat(topicstr, "/window/state",sizeof(topicstr) - strlen (topicstr) - 1);
                        //mqtt_client.subscribe(topicstr);  
                        // window target (%)
                        topicstr[len] = '\0';
                        subscribe (topicstr, (char*) "/window/target",sizeof(topicstr));
                        //strncat(topicstr, "/window/target",sizeof(topicstr) - strlen (topicstr) - 1);
                        //mqtt_client.subscribe(topicstr);  
                    }
                }
                 // control active
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    topicstr[len] = '\0';
                    subscribe (topicstr, (char*) "/control/mode",sizeof(topicstr));
                    //strncat(topicstr, "/control/mode",sizeof(topicstr) - strlen (topicstr) - 1);
                    //mqtt_client.subscribe(topicstr);                 
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

    // local zero terminated copy of payload
    memcpy(value, payload, std::min<size_t>(sizeof(value) - 1, length));
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
               syslog.log(LOG_DEBUG, "MQTT: callback "+String(topic)+" : "+String(value));
    }

    if (length>0) {
        //if (!VdmConfig.configFlash.protConfig.protocolFlags.publishPathAsRoot) {
            if (topic[0]=='/') {
                topic++;        // adjust topic 
            }
        //}
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
           // pt+= strlen(mqtt_commonTopic);
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
                    if (strncmp(VdmConfig.configFlash.valvesConfig.valveConfig[i].name,item,sizeof(VdmConfig.configFlash.valvesConfig.valveConfig[i].name))==0) {
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
                        } else if (checkTopicName(pt,(char*) "/dynOffs")) {
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
                if (strlen(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)>0)
                    strncpy(nrstr,VdmConfig.configFlash.valvesConfig.valveConfig[x].name,sizeof(nrstr));
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
                            if (strlen(VdmConfig.configFlash.tempsConfig.tempConfig[tempIdx].name)>0)
                                strncpy(nrstr,VdmConfig.configFlash.tempsConfig.tempConfig[tempIdx].name,sizeof(nrstr));
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
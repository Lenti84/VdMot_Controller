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

CMqtt Mqtt;

void mcallback(char* topic, byte* payload, unsigned int length) 
{
    Mqtt.callback(topic, payload, length);
}


WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void CMqtt::mqtt_setup(IPAddress brokerIP,uint16_t brokerPort) 
{
    mqtt_client.setServer(brokerIP, brokerPort);
    mqtt_client.setCallback(mcallback);

    memset (mqtt_mainTopic,0,sizeof(mqtt_mainTopic));
    if (VdmConfig.configFlash.protConfig.publishPathAsRoot) strcpy(mqtt_mainTopic,"/");

    if (strlen(VdmConfig.configFlash.systemConfig.stationName)>0) {
        strncat(mqtt_mainTopic, VdmConfig.configFlash.systemConfig.stationName,sizeof(mqtt_mainTopic) - strlen (mqtt_mainTopic) - 1);
        strncat(mqtt_mainTopic, "/",sizeof(mqtt_mainTopic) - strlen (mqtt_mainTopic) - 1);
    } else  {
        strncat(mqtt_mainTopic, DEFAULT_MAINTOPIC,sizeof(mqtt_mainTopic) - strlen (mqtt_mainTopic) - 1);
    }

    strcpy(mqtt_commonTopic, mqtt_mainTopic);
    strncat(mqtt_commonTopic, DEFAULT_COMMONTOPIC,sizeof(mqtt_commonTopic) - strlen (mqtt_commonTopic) - 1);
    strcpy(mqtt_valvesTopic, mqtt_mainTopic);
    strncat(mqtt_valvesTopic, DEFAULT_VALVESTOPIC,sizeof(mqtt_valvesTopic)- strlen (mqtt_valvesTopic) - 1);
    strcpy(mqtt_tempsTopic, mqtt_mainTopic);
    strncat(mqtt_tempsTopic, DEFAULT_TEMPSTOPIC,sizeof(mqtt_tempsTopic)- strlen (mqtt_tempsTopic) - 1);

    if (strlen(VdmConfig.configFlash.systemConfig.stationName)>0) {
        strncpy(stationName, VdmConfig.configFlash.systemConfig.stationName,sizeof(stationName));
    } else strncpy(stationName, DEVICE_HOSTNAME,sizeof(stationName));

}

CMqtt::CMqtt()
{
    
}

void CMqtt::mqtt_loop() 
{
    if (!mqtt_client.connected()) {
        reconnect();        
    }
    if (mqtt_client.connected()) {
        mqtt_client.loop();
        publish_valves();
    }
    mqttConnected=mqtt_client.connected();
    mqttState=mqtt_client.state();
}


void CMqtt::reconnect() 
{
    char topicstr[MAINTOPIC_LEN+50];
    char nrstr[11];
    char* mqttUser = NULL;
    char* mqttPwd = NULL;
    uint8_t len;
    

    if ((strlen(VdmConfig.configFlash.protConfig.userName)>0) && (strlen(VdmConfig.configFlash.protConfig.userPwd)>0)) {
        mqttUser = VdmConfig.configFlash.protConfig.userName;
        mqttPwd = VdmConfig.configFlash.protConfig.userPwd;
    }
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
        syslog.log(LOG_DEBUG, "MQTT reconnecting ...");
    }
    UART_DBG.println("Reconnecting MQTT...");
    if (!mqtt_client.connect(stationName,mqttUser,mqttPwd)) {
        UART_DBG.print("failed, rc=");
        UART_DBG.print(mqtt_client.state());
        UART_DBG.println(" retrying");
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
            syslog.log(LOG_ERR, "MQTT failed rc="+String(mqtt_client.state())+String(" retrying"));
        }
        mqttState=mqtt_client.state();
        VdmTask.yieldTask(5000);
        return;
    }
    
    // make some subscriptions
    memset(topicstr,0x0,sizeof(topicstr));
    strncat(topicstr,mqtt_commonTopic,sizeof(topicstr) - strlen (topicstr) - 1);
    len = strlen(topicstr);
    strncat(topicstr, "heatControl",sizeof(topicstr) - strlen (topicstr) - 1);
    mqtt_client.subscribe(topicstr);    
    topicstr[len] = '\0';
    strncat(topicstr, "parkPosition",sizeof(topicstr) - strlen (topicstr) - 1);
    mqtt_client.subscribe(topicstr);    

    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
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
            strncat(topicstr, "/target",sizeof(topicstr) - strlen (topicstr) - 1);
            mqtt_client.subscribe(topicstr);

            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].active) {
                if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].link==0) {
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].valueSource==0) {
                        // temp value
                        topicstr[len] = '\0';
                        strncat(topicstr, "/tValue",sizeof(topicstr) - strlen (topicstr) - 1);
                        mqtt_client.subscribe(topicstr);    
                    } 
                    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[x].targetSource==0) {
                        // temp target
                        topicstr[len] = '\0';
                        strncat(topicstr, "/tTarget",sizeof(topicstr) - strlen (topicstr) - 1);
                        mqtt_client.subscribe(topicstr);    
                    } 
                }
            }
        }
    }
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
        syslog.log(LOG_DEBUG, "MQTT Connected...");
    }
   
}

void CMqtt::callback(char* topic, byte* payload, unsigned int length) 
{
    bool found;
    char item[20];
    char value[10];
    char* pt;
    uint8_t i;
    uint8_t idx;
   
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
               syslog.log(LOG_DEBUG, "MQTT: callback "+String(topic));
    }
    if (length>0) {
        memset(value,0x0,sizeof(value));
        memcpy(value,payload,length);
        if (memcmp(mqtt_commonTopic,(const char*) topic, strlen(mqtt_commonTopic))==0) {
            memset(item,0x0,sizeof(item));
            pt= (char*) topic;
            pt+= strlen(mqtt_commonTopic);
            if (strncmp(pt,"heatControl",sizeof("heatControl"))==0) {
               VdmConfig.configFlash.valvesControlConfig.heatControl=atoi(value);
               VdmConfig.writeValvesControlConfig(false); 
            } 
            if (strncmp(pt,"parkPosition",sizeof("parkPosition"))==0) {
               VdmConfig.configFlash.valvesControlConfig.parkingPosition=atoi(value); 
               VdmConfig.writeValvesControlConfig(false); 
            }    
        }

        else if (memcmp(mqtt_valvesTopic,(const char*) topic, strlen(mqtt_valvesTopic))==0) {
            memset(item,0x0,sizeof(item));
            pt= (char*) topic;
            pt+= strlen(mqtt_valvesTopic);
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

            memset(value,0x0,sizeof(value));
            memcpy(value,payload,length);
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
               syslog.log(LOG_DEBUG, "MQTT: payload "+String(topic)+" : "+String(value));
            }  
            if (found) {
               
                if (isFloat(value)) {
                    if (strncmp(pt,"/target",7)==0) {
                        StmApp.actuators[idx].target_position = atoi(value);
                    } else if (strncmp(pt,"/tValue",7)==0) {
                        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].active) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].valueSource==0)
                                PiControl[idx].value=strtof(value, NULL);
                        }
                    } else if (strncmp(pt,"/tTarget",8)==0) {
                        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].active) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].targetSource==0)
                                PiControl[idx].target=strtof(value, NULL);
                        }
                    }else if (strncmp(pt,"/dynOffs",8)==0) {
                        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].active) {
                            if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].targetSource==0)
                                PiControl[idx].dynOffset=atoi(value);
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
            } else {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG, "MQTT: not found topic "+String(item));
                }   
            }
            
        }
    }
}

void CMqtt::publish_valves () 
{
    char topicstr[MAINTOPIC_LEN+30];
    char nrstr[11];
    char valstr[10];
    int8_t tempIdx;
    uint8_t len;
    
    if (VdmConfig.configFlash.protConfig.publishTarget) {
        memset(topicstr,0x0,sizeof(topicstr));
        strncat(topicstr,mqtt_commonTopic,sizeof(topicstr) - strlen (topicstr) - 1);
        len = strlen(topicstr);
        strncat(topicstr, "heatControl",sizeof(topicstr) - strlen (topicstr) - 1);
        itoa(VdmConfig.configFlash.valvesControlConfig.heatControl, valstr, 10);        
        mqtt_client.publish(topicstr, valstr);  

        topicstr[len] = '\0';
        strncat(topicstr, "parkPosition",sizeof(topicstr) - strlen (topicstr) - 1);
        itoa(VdmConfig.configFlash.valvesControlConfig.parkingPosition, valstr, 10);        
        mqtt_client.publish(topicstr, valstr);   
    }  

    memset(topicstr,0x0,sizeof(topicstr));
    strncat(topicstr,mqtt_commonTopic,sizeof(topicstr) - strlen (topicstr) - 1);
    len = strlen(topicstr);
    strncat(topicstr, "state",sizeof(topicstr) - strlen (topicstr) - 1);
    itoa(VdmSystem.systemState , valstr, 10);        
    mqtt_client.publish(topicstr, valstr);  

    topicstr[len] = '\0';
    strncat(topicstr, "message",sizeof(topicstr) - strlen (topicstr) - 1);       
    mqtt_client.publish(topicstr,VdmSystem.systemMessage);   

    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
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

            // actual value
            strncat(topicstr, "/actual",sizeof(topicstr) - strlen (topicstr) - 1);
            itoa(StmApp.actuators[x].actual_position, valstr, 10);        
            mqtt_client.publish(topicstr, valstr);
            
            // target
            if (VdmConfig.configFlash.protConfig.publishTarget) {
                topicstr[len] = '\0';
                strncat(topicstr, "/target",sizeof(topicstr) - strlen (topicstr) - 1);
                itoa(StmApp.actuators[x].target_position, valstr, 10);
                mqtt_client.publish(topicstr, valstr);
            }
            // state
            topicstr[len] = '\0';
            strncat(topicstr, "/state",sizeof(topicstr) - strlen (topicstr) - 1);
            itoa(StmApp.actuators[x].state, valstr, 10);
            mqtt_client.publish(topicstr, valstr);
            
            // meancurrent
            topicstr[len] = '\0';
            strncat(topicstr, "/meancur",sizeof(topicstr) - strlen (topicstr) - 1);
            itoa(StmApp.actuators[x].meancurrent, valstr, 10);
            mqtt_client.publish(topicstr, valstr);

            // temperature 1st sensor
            topicstr[len] = '\0';
            strncat(topicstr, "/temp1",sizeof(topicstr) - strlen (topicstr) - 1);
            String s = String(((float)StmApp.actuators[x].temp1)/10,1); 
            mqtt_client.publish(topicstr, (const char*) &s);

            // temperature 2nd sensor
            topicstr[len] = '\0';
            strncat(topicstr, "/temp2",sizeof(topicstr) - strlen (topicstr) - 1);
            s = String(((float)StmApp.actuators[x].temp2)/10,1); 
            mqtt_client.publish(topicstr, (const char*) &s);
        }
    }
    
    for (uint8_t x = 0;x<StmApp.tempsCount;x++) {
        tempIdx=VdmConfig.findTempID(StmApp.temps[x].id);
        if (tempIdx>=0) {
            if (VdmConfig.configFlash.tempsConfig.tempConfig[tempIdx].active) {
                if ((!Web.findIdInValve (tempIdx)) || VdmConfig.configFlash.protConfig.publishAllTemps) {
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
                    mqtt_client.publish(topicstr,StmApp.temps[x].id);
                    // actual value
                    topicstr[len] = '\0';
                    strncat(topicstr, "/value",sizeof(topicstr) - strlen (topicstr) - 1);
                    String s = String(((float)StmApp.temps[x].temperature)/10,1);     
                    mqtt_client.publish(topicstr,(const char*) &s);
                }
            }
        }
    }
    
}
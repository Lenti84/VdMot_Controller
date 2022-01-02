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
#include "VdmTask.h"

CMqtt Mqtt;

void mcallback(char* topic, byte* payload, unsigned int length) 
{
    Mqtt.callback(topic, payload, length);
}


WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void CMqtt::mqtt_setup(IPAddress brokerIP,uint16_t brokerPort) {
    mqtt_client.setServer(brokerIP, brokerPort);
    mqtt_client.setCallback(mcallback);

    strcpy(mqtt_maintopic, DEFAULT_MAINTOPIC);
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
}


void CMqtt::reconnect() {
    char topicstr[MAINTOPIC_LEN+20];
    char nrstr[3];
    char* mqttUser = NULL;
    char* mqttPwd = NULL;
    if ((strlen(VdmConfig.configFlash.protConfig.userName)>0) && (strlen(VdmConfig.configFlash.protConfig.userPwd)>0)) {
        mqttUser = VdmConfig.configFlash.protConfig.userName;
        mqttPwd = VdmConfig.configFlash.protConfig.userPwd;
    }
    
    UART_DBG.println("Reconnecting MQTT...");
    if (!mqtt_client.connect("VdMot",mqttUser,mqttPwd)) {
        UART_DBG.print("failed, rc=");
        UART_DBG.print(mqtt_client.state());
        UART_DBG.println(" retrying");
        VdmTask.yieldTask(5000);
        return;
    }
   

    // make some subscriptions
    for (unsigned int x = 1;x<=ACTUATOR_COUNT;x++) {
        topicstr[0] = '\0';
        nrstr[0] = '\0';
        itoa(x, nrstr, 10);

        // prepare prefix
        strcat(topicstr, mqtt_maintopic);
        strcat(topicstr, nrstr);      

        // target value
        strcat(topicstr, "/target");
        mqtt_client.subscribe(topicstr);
    }
    UART_DBG.println("MQTT Connected...");
}


void CMqtt::callback(char* topic, byte* payload, unsigned int length) {
    unsigned char found = 0;
    unsigned char value;
    char msg[length+1];

    for (unsigned int i = 0; i < length; i++) {
        msg[i] = (char)payload[i];
    }
   
    msg[length] = '\0';
  
    // test topic
    for(unsigned int x = 0; x < strlen(topic); x++) {
        if(topic[x] == '\0') break;

        if(topic[x] == mqtt_maintopic[x]) {
            found++;        
        }
        else if (mqtt_maintopic[x] == '\0' && strlen(mqtt_maintopic) == found) {
            break;
        }
        else {
            found = 0;
            break;
        }
    }

    // find out valve nr
    if(found) {
        // single digit
        if (topic[found+1] == '/') {
            value = topic[found] - 48;
            found += 2;
        }
        else if (topic[found+2] == '/') {
            value = (topic[found] - 48) * 10 + topic[found] - 48;
            found += 3;
        }
        else found = 0;
    }

    // find out subtopic
    if (found) {
        // subtopic "target"
        if (0 == strcmp(&topic[found],"target")) {
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
               syslog.log(LOG_INFO, "MQTT: found target topic");
            }	
            
            actuators[value-1].target_position = atoi(msg);
        }
        else {            
            found = 0;
        }
    }
    else {
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
            syslog.log(LOG_ERR, "MQTT: cant eval data");
        }	
    }
}


void CMqtt::publish_valves () {

    char topicstr[MAINTOPIC_LEN+20];
    char nrstr[3];
    char valstr[10];
    unsigned char len;

    for (unsigned int x = 1;x<=ACTUATOR_COUNT;x++) {
        topicstr[0] = '\0';
        nrstr[0] = '\0';
        itoa(x, nrstr, 10);

        // prepare prefix
        strcat(topicstr, mqtt_maintopic);
        strcat(topicstr, nrstr);
        len = (unsigned char) strlen(topicstr);

        // actual value
        strcat(topicstr, "/actual");
        itoa(actuators[x-1].actual_position, valstr, 10);        
        mqtt_client.publish(topicstr, valstr);

        // state
        topicstr[len] = '\0';
        strcat(topicstr, "/state");
        itoa(actuators[x-1].state, valstr, 10);
        mqtt_client.publish(topicstr, valstr);

        // meancurrent
        topicstr[len] = '\0';
        strcat(topicstr, "/meancur");
        itoa(actuators[x-1].meancurrent, valstr, 10);
        mqtt_client.publish(topicstr, valstr);

        // temperature
        topicstr[len] = '\0';
        strcat(topicstr, "/temperature");
        itoa(actuators[x-1].temperature, valstr, 10);
        mqtt_client.publish(topicstr, valstr);
    }
}
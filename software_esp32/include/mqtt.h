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
#include "VdmConfig.h"

#define MAINTOPIC_LEN 100

// MQTT settings
#define DEFAULT_MAINTOPIC    "VdMotFBH/"           //  /VdMotFBH/valve/1/actual
#define DEFAULT_COMMONTOPIC  "common/"
#define DEFAULT_VALVESTOPIC  "valves/"
#define DEFAULT_TEMPSTOPIC   "temps/"


#define publishNothing  0
#define publishCommon   1
#define publishValves   2
#define publishTemps    4

#define MAX_MQTT_RETRIES 100
#define STATE_FAILED 9

typedef struct {
  bool controlActive;
  uint8_t  position;
  uint8_t target;
  uint8_t state;
  uint16_t meanCurrent;
  int temp1;
  int temp2;
  uint32_t ts;
  bool publishNow;
  bool publishTimeOut;
  uint32_t lasttValuets;
  uint8_t calibRetries;
  uint32_t openCount;
  uint32_t closeCount;
  int32_t deadzoneCount; 
  uint32_t movements;
} LASTVALVEVALUES; 

typedef struct {
  int16_t temperature;          // temperature of assigned sensor
  char id[25];
  uint32_t ts;
  bool publishNow;
} LASTTEMPVALUES; 

typedef struct {
  uint8_t heatControl;          
  uint8_t parkingPosition;
  uint8_t systemState;
  String upTime;
  String systemMessage;
} LASTCOMMONVALUES; 

typedef struct {
  bool tValueFailed; 
  uint8_t thisState; 
  bool messengerSent;        
} VALVESTATE; 


class CMqtt
{
  public:
    CMqtt();
    void reconnect();
    void disconnect();
    void mqtt_setup(IPAddress brokerIP,uint16_t brokerPort);
    void mqtt_loop();
    void callback(char* topic, byte* payload, unsigned int length);
    int mqttState;
    bool mqttConnected;
    bool mqttReceived;
    VALVESTATE valveStates[ACTUATOR_COUNT];
  private:
    void publish_all (uint8_t publishFlags);
    void publish_common (); 
    void publish_valves ();
    void publish_temps ();
    uint8_t checkForPublish (); 
    void checktValueTimeOut (); 
    bool checkTopicName(char* topic,char* ref,bool set=true);
    bool checkTopicPath(char* topic,char* ref);
    void subscribe (char* topicstr, char* thisTopic, uint8_t size, bool publishValue=false);
    void publishValue (char* topicstr, char* valstr);
    bool messengerSend;
    bool firstPublish;
    char mqtt_mainTopic[MAINTOPIC_LEN] = {0};
    char mqtt_commonTopic[MAINTOPIC_LEN] = {0};
    char mqtt_valvesTopic[MAINTOPIC_LEN]= {0};
    char mqtt_tempsTopic[MAINTOPIC_LEN]= {0};
    char mqtt_callbackTopic[MAINTOPIC_LEN]= {0};
    char stationName[sizeof(VdmConfig.configFlash.systemConfig.stationName)+5]= {0};
    uint32_t tsPublish;
    uint32_t tsForcePublish;
    bool forcePublish;
    uint32_t connectTimeout;
    LASTCOMMONVALUES lastCommonValues;
    LASTVALVEVALUES lastValveValues[ACTUATOR_COUNT];
    LASTTEMPVALUES lastTempValues[TEMP_SENSORS_COUNT];
    boolean topicsReceived;
};

extern CMqtt Mqtt;
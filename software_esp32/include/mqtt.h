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

#define MAINTOPIC_LEN 50

// MQTT settings
#define DEFAULT_MAINTOPIC    "VdMotFBH/"           //  /VdMotFBH/valve/1/actual
#define DEFAULT_COMMONTOPIC  "common/"
#define DEFAULT_VALVESTOPIC  "valves/"
#define DEFAULT_TEMPSTOPIC   "temps/"

class CMqtt
{
  public:
    CMqtt();
    void mqtt_setup(IPAddress brokerIP,uint16_t brokerPort);
    void mqtt_loop();
    void callback(char* topic, byte* payload, unsigned int length);
    int mqttState;
    bool mqttConnected;
  private:
    void reconnect();
    void publish_valves ();

    char mqtt_mainTopic[MAINTOPIC_LEN];
    char mqtt_commonTopic[MAINTOPIC_LEN];
    char mqtt_valvesTopic[MAINTOPIC_LEN];
    char mqtt_tempsTopic[MAINTOPIC_LEN];
    char mqtt_callbackTopic[MAINTOPIC_LEN];
    char stationName[MAINTOPIC_LEN];
};

extern CMqtt Mqtt;
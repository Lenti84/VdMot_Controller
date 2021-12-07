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
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <FS.h>
//#include "stm32.h"
//#include "stm32ota.h"
#include <ESPmDNS.h>
#include "mqtt.h"
#include "app.h"
#include "telnet.h"
#include <WiFiUdp.h>
#include "espota.h"
#include "Logger.h"
#include "TaskManagerIO.h"

#include "VdmConfig.h"
#include "VdmNet.h"
#include "VdmTask.h"



//const char* host = "stm32ota";


Logger logger;      // web service logger

uint8_t vismode = VISMODE_ON;    // visualisation mode for debug messages



void setup(void)
{
  UART_DBG.begin(115200);
  if (!SPIFFS.begin(true)) {
    UART_DBG.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  UART_DBG.println("SPIFFS booted");

  // init config, read from flash, init network
  VdmConfig.init();
  VdmNet.init();
  VdmTask.init();

  
  //STM32ota_setup();

 
  //RunMode();


  while(UART_STM32.available()) UART_STM32.read();

  
  if (MDNS.begin("esp32")) {
    UART_DBG.println("MDNS responder started");
  }

    
  //telnet_setup();

  //app_setup();

  //ESPota_setup();

}


void loop(void) {
 /*
  static uint32_t timer1000ms = 0;

  // 1000 ms task
  if ((millis()-timer1000ms) > (uint32_t) 1000 ) {
      timer1000ms = millis();       
      //logger.println("Logger test");      // logs data to webservice 
      //logger.println("Logger data test", Logger::DATA);  // logs data to webservice 
      //telnet_loop();
  }
*/
 // mqtt_loop();

  //app_loop();

  //ESPota_loop();

  //yield();
  //delay(1);


  taskManager.runLoop();
}



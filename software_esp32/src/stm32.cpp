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

#include "stm32ota.h"
#include "globals.h"


int stm32ota_command = 0;


void STM32ota_begin();
void ResetSTM32();

void STM32ota_setup() {

  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  //UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  //while(UART_STM32.available()) UART_STM32.read();

  // BOOT and RESET Pin of STM32  
  pinMode(BOOT0, OUTPUT);
  pinMode(NRST, OUTPUT);

}



void STM32ota_loop() {

  enum ota_state {STM32OTA_INIT, STM32OTA_IDLE, STM32OTA_PREPARE, STM32OTA_SENDSIGN, \
                  STM32OTA_GETID, STM32OTA_FLASH, STM32OTA_DO, STM32OTA_ERROR };
  static enum ota_state stm32ota_state = STM32OTA_INIT;

  static int timeout = 0;
  static int count = 0;
  unsigned char buffer[10];
  unsigned char id;

  switch(stm32ota_state) {

        case STM32OTA_INIT: 
                //UART_DBG.println("STM32 ota: init state machine");
                stm32ota_command = 0;
                stm32ota_state = STM32OTA_IDLE;
                break;

        case STM32OTA_IDLE: 
                if (stm32ota_command > 0) {
                  UART_DBG.println("STM32 ota: start flashing");
                  stm32ota_state = STM32OTA_PREPARE;
                  stm32ota_command = 0;
                }
                break;

        case STM32OTA_PREPARE: 
                STM32ota_begin();
                ResetSTM32();
                delay(1500);            // stm32 is a little slow
                UART_STM32.flush();     // flush garbage in rx buffer
                UART_STM32.println("DEADBEEF");
                timeout = 20;    // 100 ms
                count = 10;
                stm32ota_state = STM32OTA_SENDSIGN;
        break;

        case STM32OTA_SENDSIGN:                              
                if (timeout == 0) {
                  timeout = 20;
                  UART_STM32.println("DEADBEEF");
                  UART_DBG.print("STM32 ota: send sign");
                  if(count) count--;                
                }
                else timeout--;

                if (count == 0) stm32ota_state = STM32OTA_ERROR;

                if (UART_STM32.available() >= 6) {
                   UART_STM32.readBytes(buffer,6);                  
                   UART_DBG.println("STM32 ota: got answer");
                   //UART_DBG.println(buffer);
                   if (memcmp("BEEFIT",&buffer[0],6) == 0) {                     
                     stm32ota_state = STM32OTA_FLASH;
                     timeout = 30;    // 300 ms
                   }
                   else stm32ota_state = STM32OTA_ERROR;
                }
                break;

        case STM32OTA_FLASH: 
                UART_DBG.println("STM32 ota: ready to beef");
                if (timeout == 0) {
                  UART_STM32.write(STM32INIT);
                  stm32ota_state = STM32OTA_GETID;
                  timeout = 3;
                }
                else timeout--;
                
                break;

        case STM32OTA_GETID: 
                UART_DBG.println("STM32 ota: get id");
                if (timeout == 0) {
                                    
                  id = stm32GetId();

                  if (id > 0) {
                    // tbd
                  }
                  else stm32ota_state = STM32OTA_IDLE;
                }
                else timeout--;

                break;

        case STM32OTA_ERROR: 
                UART_DBG.println("STM32 ota: error");
                stm32ota_state = STM32OTA_IDLE;
                break;

        default:
                stm32ota_state = STM32OTA_INIT;
                break;
  }

}


void STM32ota_start() {

  stm32ota_command = 1;

}



// has to be called prior to stm32 flash transactions
void STM32ota_begin() {
  UART_DBG.println("STM32 ota: begin");
  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  UART_STM32.begin(9600, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  while(UART_STM32.available()) UART_STM32.read();
}


void ResetSTM32() {
  UART_DBG.println("STM32 ota: reset STM32");
  delay(100);
  digitalWrite(NRST, HIGH);  
  delay(150);
  digitalWrite(NRST, LOW);
  delay(150);
}


void FlashMode()  {    //Tested
  UART_DBG.println("Set Flash mode");
  digitalWrite(BOOT0, HIGH);
  delay(100);
  digitalWrite(NRST, HIGH);
  //digitalWrite(LED, LOW);
  delay(1500);
  digitalWrite(NRST, LOW);
  delay(500);
//   for ( int i = 0; i < 3; i++) {
//     //digitalWrite(LED, !digitalRead(LED));
//     delay(100);
//   }

//   while(UART_STM32.available()) UART_STM32.read();
//   delay(100);
}

void RunMode()  {    //Tested
  UART_DBG.println("Set Run mode");
  digitalWrite(BOOT0, LOW);
  delay(100);
  digitalWrite(NRST, HIGH);
//digitalWrite(LED, LOW);
  delay(150);
  digitalWrite(NRST, LOW);
  delay(500);
//   for ( int i = 0; i < 3; i++) {
//     //digitalWrite(LED, !digitalRead(LED));
//     delay(100);
//   }
}
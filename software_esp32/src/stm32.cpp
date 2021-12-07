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


void STM32ota_setup() {

  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  //UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  //while(UART_STM32.available()) UART_STM32.read();

  // BOOT and RESET Pin of STM32  
  pinMode(BOOT0, OUTPUT);
  pinMode(NRST, OUTPUT);

}


// has to be called prior to stm32 flash transactions
void STM32ota_begin() {
  UART_DBG.println("STM32 ota begin");
  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  while(UART_STM32.available()) UART_STM32.read();

}


void ResetSTM32() {
  UART_DBG.println("Reset STM32");
  delay(100);
  digitalWrite(NRST, HIGH);
  
  delay(150);
  digitalWrite(NRST, LOW);
  delay(500);
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
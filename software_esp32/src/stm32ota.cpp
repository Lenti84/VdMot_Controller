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

/* ------------------------------------------------- */
/*

Credits to https://github.com/csnol/1CHIP-Programmers

*/
/* ------------------------------------------------- */

#include "globals.h"
#include "stm32ota.h"
#include "stm32.h"

CStmOta StmOta;

CStmOta::CStmOta()
{

}

const TIdChipName idChipNames[] = {
        {"STM32F40xx/41xx", 0x413},
        {"STM32F401xB/C",   0x423},
        {"STM32F411xx",     0x431},
        {"STM32F401xD/E",   0x433},
        {"",                0}
  };

String CStmOta::checkChipName (uint32_t thisID) 
{
  TIdChipName *entry = (TIdChipName*) idChipNames;
 
  while (entry->chipName != "") {
      if (entry->id == thisID) {
        return(entry->chipName);
      }
      entry++;
  }
  
  return("Unknown Chip");
}

void CStmOta::stm32SendCommand(uint8_t commd) 
{    // Tested
  delayMicroseconds(1000);
  UART_STM32.write(commd);
  delayMicroseconds(otaDelayCmd);
  UART_STM32.write(~commd);
  delayMicroseconds(otaDelayCmd);
}

uint8_t CStmOta::stm32Erase() 
{     // Tested
  Stm32.clearUART_STM32Buffer();
  stm32SendCommand(STM32ERASE);

  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK)
  {
    UART_STM32.write(0xFF);
    UART_STM32.write(0x00);
  }
  else return STM32ERR;
  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  return UART_STM32.read();
}

uint8_t CStmOta::stm32Erasen() 
{     // Tested
  Stm32.clearUART_STM32Buffer();
  stm32SendCommand(STM32ERASEN);
  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK)
  {
    UART_STM32.write(0xFF);
    UART_STM32.write(0xFF);
    UART_STM32.write(0x00);
  }
  else return STM32ERR;
  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  return UART_STM32.read();
}

uint8_t CStmOta::stm32ErasenStart() 
{     // Tested
  uint8_t x;

  stm32SendCommand(STM32ERASEN);

  for (x=0;x<100;x++) {
    if (UART_STM32.available()>0) {    
      if (UART_STM32.read() == STM32ACK)
      {
        UART_STM32.write(0xFF);
        UART_STM32.write(0xFF);
        UART_STM32.write(0x00);
        return STM32OK;
      }
      else return STM32ERR;
    }
    delayMicroseconds(otaDelayErase);
  }
  return STM32ERR;
}

// No test yet
uint8_t CStmOta::stm32Run()   
{
  Stm32.clearUART_STM32Buffer();
  stm32SendCommand(STM32RUN);

  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK) {
    stm32Address(STM32STADDR);
    return STM32ACK;
  }
  else
    return STM32ERR;
}

// Tested
uint8_t CStmOta::stm32Read(uint8_t * rdbuf, uint32_t rdaddress, uint16_t rdlen) 
{
  uint16_t timer = 0;
  size_t getlen;
  
  // send read request
  Stm32.clearUART_STM32Buffer();
  stm32SendCommand(STM32RD);  

  // wait for ACK
  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK) {

    // send read address
    // got ACK?
    if (stm32Address(rdaddress) == STM32ACK) {
      // send read length
      stm32SendCommand(rdlen - 1);
    }
    else return STM32ERR;
  }
  else return STM32ERR;

  // wait for ACK
  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK) {

    // wait for requested data
    for (timer=0;timer<50;timer++) {
      delay(10);
      getlen = UART_STM32.available();
      if (getlen == rdlen) {
        UART_STM32.readBytes(rdbuf, getlen);
        return STM32ACK;
      }
    }     
  }
  
  return STM32ERR;
}

uint8_t CStmOta::stm32Address(uint32_t addr) 
{    // Tested
  Stm32.clearUART_STM32Buffer();

  uint8_t sendaddr[4];
  uint8_t addcheck = 0;
  sendaddr[0] = addr >> 24;
  sendaddr[1] = (addr >> 16) & 0xFF;
  sendaddr[2] = (addr >> 8) & 0xFF;
  sendaddr[3] = addr & 0xFF;
  for (int i = 0; i <= 3; i++) {
    UART_STM32.write(sendaddr[i]);
    addcheck ^= sendaddr[i];
  }
  UART_STM32.write(addcheck);
  if (!Stm32.waitForSTMResponse(60000)) return STM32ERR;
  return UART_STM32.read();
}

uint8_t CStmOta::stm32SendData(uint8_t * data, uint8_t wrlen) 
{     // Tested
  UART_STM32.write(wrlen-1);
  for (int i = 0; i <= wrlen; i++) {
    UART_STM32.write(*data);
    data++;
    delayMicroseconds(otaDelayFlash);
  }
  //Stm32.clearUART_STM32Buffer();
  //UART_STM32.write(getChecksum(data, wrlen));
  //while (!UART_STM32.available());
  //return UART_STM32.read();
  return 0;
}

char CStmOta::stm32Version() 
{     // Tested
  int x;
  uint8_t vsbuf[14];

  while(UART_STM32.available()) UART_STM32.read();

  Serial.println("GetVersion");
  
  stm32SendCommand(STM32GET);

  for (x=0;x<100;x++) {
    if (UART_STM32.available()>0) {  
      vsbuf[0] = UART_STM32.read();
      if (vsbuf[0] != STM32ACK) {
        Serial.println("Error GetVersion: wrong answer");
        return STM32ERR;
      }
      else {
        UART_STM32.readBytesUntil(STM32ACK, vsbuf, 14);
        return vsbuf[1];
      }
    }
    else Serial.println("Error GetVersion: no data");
    delayMicroseconds(otaDelayCmd);
  }

  return STM32ERR;  
}

uint8_t CStmOta::stm32GetId() 
{     // Tested
  uint8_t x;
  chipId = 0;
  uint8_t sbuf[5];

  while(UART_STM32.available()) UART_STM32.read();

  Serial.print("GetId");
  
  stm32SendCommand(STM32ID);
  
  for (x=0;x<100;x++) {
    if (UART_STM32.available()>0) {          
      sbuf[0] = UART_STM32.read();
      if (sbuf[0] == STM32ACK) {
        UART_STM32.readBytesUntil(STM32ACK, sbuf, 4);
        chipId = sbuf[1];
        chipId = (chipId << 8) + sbuf[2];
        chipName = checkChipName (chipId);
        #ifdef EnvDevelop
          UART_DBG.print("- Id: 0x");
          UART_DBG.print(chipId, HEX); 
          UART_DBG.println(""); 
          UART_DBG.print("--> type is ");
          UART_DBG.println(chipName);
        #endif
        return 1;
      }
      else {
        #ifdef EnvDevelop
          UART_DBG.println("Error: wrong answer");
        #endif
        return 0;
      }
    }    
    delayMicroseconds(otaDelayCmd);
  }
  #ifdef EnvDevelop
    UART_DBG.println("- Error: no data");
  #endif
  return 0;
}

uint8_t CStmOta::getChecksum( uint8_t * data, uint8_t datalen) {    // Tested
  uint8_t lendata = datalen;
  for (int i = 0; i <= datalen; i++)
    lendata ^= data[i];
  return lendata;
}

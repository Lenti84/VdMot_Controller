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

CStmOta StmOta;

CStmOta::CStmOta()
{

}

const String STM32_CHIPNAME[10] = {
  "Unknown Chip",
  "STM32F03xx4/6",
  "STM32F030x8/05x",
  "STM32F030xC",
  "STM32F103x4/6",
  "STM32F103x8/B",
  "STM32F103xC/D/E",
  "STM32F105/107",
  "STM32F401xB/C",
  "STM32F411xE"
};

void CStmOta::stm32SendCommand(unsigned char commd) 
{    // Tested
  UART_STM32.write(commd);
  UART_STM32.write(~commd);
}

unsigned char CStmOta::stm32Erase() 
{     // Tested
  stm32SendCommand(STM32ERASE);
  while (!UART_STM32.available());
  if (UART_STM32.read() == STM32ACK)
  {
    UART_STM32.write(0xFF);
    UART_STM32.write(0x00);
  }
  else return STM32ERR;
  while (!UART_STM32.available());
  return UART_STM32.read();
}

unsigned char CStmOta::stm32Erasen() 
{     // Tested
  stm32SendCommand(STM32ERASEN);
  while (!UART_STM32.available());
  if (UART_STM32.read() == STM32ACK)
  {
    UART_STM32.write(0xFF);
    UART_STM32.write(0xFF);
    UART_STM32.write(0x00);
  }
  else return STM32ERR;
  while (!UART_STM32.available());
  return UART_STM32.read();
}

unsigned char CStmOta::stm32ErasenStart() 
{     // Tested
  int x;

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
    delayMicroseconds(10);
  }
  return STM32ERR;
}

// No test yet
unsigned char CStmOta::stm32Run()   
{
  stm32SendCommand(STM32RUN);
  while (!UART_STM32.available());
  if (UART_STM32.read() == STM32ACK) {
    stm32Address(STM32STADDR);
    return STM32ACK;
  }
  else
    return STM32ERR;
}

// Tested
unsigned char CStmOta::stm32Read(unsigned char * rdbuf, unsigned long rdaddress, unsigned int rdlen) 
{
  unsigned int timer = 0;
  size_t getlen;
  
  // send read request
  //UART_DBG.println("send STM32RD");
  stm32SendCommand(STM32RD);  

  // wait for ACK
  while (!UART_STM32.available());
  if (UART_STM32.read() == STM32ACK) {

    // send read address
    // UART_DBG.println("send rdadress");

    // got ACK?
    if (stm32Address(rdaddress) == STM32ACK) {
      // send read length
      //UART_DBG.println("send rdlen");
      stm32SendCommand(rdlen - 1);
    }
    else return STM32ERR;
  }
  else return STM32ERR;

  // wait for ACK
  while (!UART_STM32.available());
  if (UART_STM32.read() == STM32ACK) {

    // wait for requested data
    for (timer=0;timer<50;timer++) {
      delay(10);
      getlen = UART_STM32.available();
      if (getlen == rdlen) {
        //UART_DBG.print("got bytes: ");
        //UART_DBG.println(getlen, DEC);
        UART_STM32.readBytes(rdbuf, getlen);
        return STM32ACK;
      }
    } 
    //UART_DBG.print("got wrong number of bytes: ");
    //UART_DBG.println(getlen, DEC);
    
  }
  
  return STM32ERR;
}

unsigned char CStmOta::stm32Address(unsigned long addr) 
{    // Tested
  unsigned char sendaddr[4];
  unsigned char addcheck = 0;
  sendaddr[0] = addr >> 24;
  sendaddr[1] = (addr >> 16) & 0xFF;
  sendaddr[2] = (addr >> 8) & 0xFF;
  sendaddr[3] = addr & 0xFF;
  for (int i = 0; i <= 3; i++) {
    UART_STM32.write(sendaddr[i]);
    addcheck ^= sendaddr[i];
  }
  UART_STM32.write(addcheck);
  while (!UART_STM32.available());
  return UART_STM32.read();
}

unsigned char CStmOta::stm32SendData(unsigned char * data, unsigned char wrlen) 
{     // Tested
  UART_STM32.write(wrlen);
  for (int i = 0; i <= wrlen; i++) {
    UART_STM32.write(data[i]);
    delayMicroseconds(20);
  }
  UART_STM32.write(getChecksum(data, wrlen));
  while (!UART_STM32.available());
  return UART_STM32.read();
}

char CStmOta::stm32Version() 
{     // Tested
  int x;
  unsigned char vsbuf[14];

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
    delayMicroseconds(20);
  }

  return STM32ERR;  
}

unsigned char CStmOta::stm32GetId() 
{     // Tested
  int x;
  int getid = 0;
  unsigned char sbuf[5];

  while(UART_STM32.available()) UART_STM32.read();

  Serial.print("GetId");
  
  stm32SendCommand(STM32ID);
  
  for (x=0;x<100;x++) {
    if (UART_STM32.available()>0) {          
      sbuf[0] = UART_STM32.read();
      if (sbuf[0] == STM32ACK) {
        UART_STM32.readBytesUntil(STM32ACK, sbuf, 4);
        getid = sbuf[1];
        getid = (getid << 9) + sbuf[2];
        Serial.print("- Id: ");
        Serial.print(getid, HEX); 
        Serial.println(""); 
        if (getid == 0x444)
          return 1;
        if (getid == 0x440)
          return 2;
        if (getid == 0x442)
          return 3;
        if (getid == 0x412)
          return 4;
        if (getid == 0x410)
          return 5;
        if (getid == 0x414)
          return 6;
        if (getid == 0x418)
          return 7;
        if (getid == 0x423)
          return 8;  
        if (getid == 0x831)  
          return 9;
      }
      else {
        Serial.println("Error: wrong answer");
        return 0;
      }
    }    
    delayMicroseconds(20);
  }
  
  Serial.println("- Error: no data");
  return 0;
}

unsigned char CStmOta::getChecksum( unsigned char * data, unsigned char datalen) {    // Tested
  unsigned char lendata = datalen;
  for (int i = 0; i <= datalen; i++)
    lendata ^= data[i];
  return lendata;
}

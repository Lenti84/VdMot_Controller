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

#include <Arduino.h>

#define STERR     	  "ERROR"
#define STM32INIT  	  0x7F          // Send Init command
#define STM32ACK  	  0x79          // return ACK answer
#define STM32NACK  	  0x1F          // return NACK answer
#define STM32GET  	  0x00          // get version command
#define STM32GVR      0x01          // get read protection status           never used in here
#define STM32ID       0x02          // get chip ID command
#define STM32RUN  	  0x21          // Restart and Run programm
#define STM32RD  	  0x11          // Read flash command                   never used in here
#define STM32WR  	  0x31          // Write flash command
#define STM32ERASE    0x43          // Erase flash command
#define STM32ERASEN   0x44          // Erase extended flash command
#define STM32WP       0x63          // Write protect command                never used in here
#define STM32WP_NS    0x64          // Write protect no-stretch command     never used in here
#define STM32UNPCTWR  0x73          // Unprotect WR command                 never used in here
#define STM32UW_NS    0x74          // Unprotect write no-stretch command   never used in here
#define STM32RP       0x82          // Read protect command                 never used in here
#define STM32RP_NS    0x83          // Read protect no-stretch command      never used in here
#define STM32UR       0x92          // Unprotect read command               never used in here
#define STM32UR_NS    0x93          // Unprotect read no-stretch command    never used in here


#define STM32STADDR     0x8000000     // STM32 codes start address, you can change to other address if use custom bootloader like 0x8002000
#define STM32ERR  		  0x00
#define STM32OK         0xFF


typedef struct Tid_Chip_Name {
  String  chipName;
  uint32_t id;
} TIdChipName;


class CStmOta 
{
public:
  CStmOta();
  uint8_t stm32GetId();
  uint8_t stm32ErasenStart();
  uint8_t getChecksum( uint8_t * data, uint8_t datalen);
  uint8_t stm32Address(uint32_t addr);
  void stm32SendCommand(uint8_t commd);
  uint8_t stm32SendData(uint8_t * data, uint8_t wrlen);
  uint32_t chipId;
  String chipName;

private:
  char stm32Version();
  uint8_t stm32Erase();
  uint8_t stm32Erasen();
  uint8_t stm32Read(uint8_t * rdbuf, uint32_t rdaddress, uint16_t rdlen);
  
  uint8_t stm32Run();
  String checkChipName (uint32_t thisID);
};

extern CStmOta StmOta;

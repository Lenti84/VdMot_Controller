/**HEADER*******************************************************************
  project : VdMot Controller
  author : Lenti84
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

#include "stm32.h"
#include "stm32ota.h"
#include "globals.h"
#include <SPIFFS.h>
#include <FS.h>
#include <CRC32.h>


int stm32ota_command = 0;

File dir;
File file;

typedef struct {
  File fsfile;
  unsigned long size;
  unsigned int blockcnt;
  unsigned int lastbytes;
  uint32_t crc;
} flashfile;

flashfile myflashfile;



  enum ota_state {STM32OTA_IDLE, STM32OTA_PREPARE, STM32OTA_SENDSIGN, \
                  STM32OTA_INITSTM, STM32OTA_ERASE_START, STM32OTA_ERASE_FIN, STM32OTA_PREPAREFILE, \
                  STM32OTA_VERIFY, STM32OTA_GETID, STM32OTA_FLASH, STM32OTA_DO, STM32OTA_STARTOVER, \
                  STM32OTA_ERROR };
  volatile enum ota_state stm32ota_state = STM32OTA_IDLE;

  

  otaUpdateStatus stmUpdateStatus = updNotStarted;

  static int timeout = 0;
  static int count = 0;
  unsigned char buffer[256];
  unsigned char id;
  static uint8_t skipsigning;
  static int blockcounter;
  static uint32_t timer = 0;

  String tempstr;
  uint32_t  tempcrc = 0;
  static CRC32 crc;




void STM32ota_begin();
void ResetSTM32();
int PrepareFile(String FileName);
int FlashBytes(int Block, int Bytes);
String updateFileName;


void STM32ota_setup() {

  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  //UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  //while(UART_STM32.available()) UART_STM32.read();

  // BOOT and RESET Pin of STM32  
  pinMode(BOOT0, OUTPUT);
  pinMode(NRST, OUTPUT);
  stm32ota_command = 0;
  stm32ota_state = STM32OTA_IDLE;
  stmUpdateStatus = updNotStarted;
}

void STM32ota_loop() {
  //UART_DBG.println("STM32 ota: state "+String(stm32ota_state));
  switch(stm32ota_state) {

        case STM32OTA_IDLE: 
                if (stm32ota_command == STM32OTA_START) {
                  UART_DBG.println("STM32 ota: start flash attemp");
                  skipsigning = 0;
                  stm32ota_state = STM32OTA_PREPAREFILE;

                  stm32ota_command = 0;
                }
                else if (stm32ota_command == STM32OTA_STARTBLANK) {
                  UART_DBG.println("STM32 ota: start blank flash attemp");
                  skipsigning = 1;
                  stm32ota_state = STM32OTA_PREPAREFILE;

                  stm32ota_command = 0;
                }

                break;

        case STM32OTA_PREPAREFILE:
                UART_DBG.println("STM32 ota: prepare file");
                stmUpdateStatus = updStarted;
                if(PrepareFile(updateFileName) == 0){
                  stm32ota_state = STM32OTA_PREPARE;                  
                }
                else stm32ota_state = STM32OTA_ERROR;

                break;

        case STM32OTA_PREPARE: 
                STM32ota_begin();
                ResetSTM32();
                delay(1500);            // stm32 is a little slow
                UART_STM32.flush();     // flush garbage in rx buffer
                while (UART_STM32.available()) {
                  UART_STM32.read();
                }
                if (!skipsigning) {                 
                  UART_STM32.println("DEADBEEF");
                  timeout = 0;
                  count = 0;
                  stm32ota_state = STM32OTA_SENDSIGN;
                }
                else {
                  UART_DBG.println("button BOOT0 must be pressed on BlackPill board!");
                  timeout = 0;
                  stm32ota_state = STM32OTA_INITSTM;
                }

                
        break;

        case STM32OTA_SENDSIGN:                              
                if (timeout == 20) {
                  timeout = 0;
                  UART_STM32.println("DEADBEEF");
                  UART_DBG.println("STM32 ota: send sign");
                  count++;                
                }
                else timeout++;

                if (count >= 4) stm32ota_state = STM32OTA_ERROR;

                if (UART_STM32.available() >= 6) {
                   UART_STM32.readBytes(buffer,6);                  
                   UART_DBG.println("STM32 ota: got answer");
                   for (uint8_t i=0; buffer[i]!=0;i++) UART_DBG.println(buffer[i]);
                   if (memcmp("BEEFIT",&buffer[0],6) == 0) {                     
                     stm32ota_state = STM32OTA_INITSTM;
                     timeout = 0;
                   }
                   else stm32ota_state = STM32OTA_ERROR;
                }
                break;

        case STM32OTA_INITSTM: 
                if (timeout==0) UART_DBG.println("STM32 ota: ready to beef");
                if (timeout >= 30) {      // 300 ms
                  UART_STM32.write(STM32INIT);
                  stm32ota_state = STM32OTA_GETID;
                  timeout = 0;
                }
                else timeout++;
                
                break;

        case STM32OTA_GETID: 
                if (timeout==0) UART_DBG.println("STM32 ota: get id");
                if (timeout >= 3) {
                                    
                  id = stm32GetId();

                  tempstr = STM32_CHIPNAME[id];
                  UART_DBG.print("--> type is ");
                  UART_DBG.println(tempstr);

                  if (id > 0) {
                    stm32ota_state = STM32OTA_ERASE_START;

                    // stm32ota_state = STM32OTA_VERIFY;
                    // blockcounter = 0;
                    // crc.reset();
                  }
                  else stm32ota_state = STM32OTA_IDLE;
                }
                else timeout++;

                break;

        case STM32OTA_ERASE_START:
                UART_DBG.println("STM32 ota: start erasing"); 

                // if (stm32Erase() == STM32ACK) {
                //   UART_DBG.print(" - done using erase() - took ");
                //   UART_DBG.print(millis() - timer, DEC);
                //   UART_DBG.println(" ms");
                //   stm32ota_state = STM32OTA_FLASH;
                //   blockcounter = 0;
                //   count = myflashfile.blockcnt + 10;    // for timeout
                // }
                // else 
                if (stm32ErasenStart() == STM32OK) {
                  timer = millis();                  
                  stm32ota_state = STM32OTA_ERASE_FIN;
                  timeout = 0;
                }
                else stm32ota_state = STM32OTA_ERROR;

                break;


        case STM32OTA_ERASE_FIN:                
                if (timeout == 0) UART_DBG.println("STM32 ota: erasing in progress");

                if(timeout>1000) {
                  UART_DBG.print("--> timeout");
                  stm32ota_state = STM32OTA_ERROR;
                }
                else {
                  timeout++;
                  if (Serial2.available()) {
                    
                    if (Serial2.read() == STM32ACK) {
                      UART_DBG.print("--> done using etended erase - took ");
                      UART_DBG.print(millis() - timer, DEC);
                      UART_DBG.println(" ms");
                      stm32ota_state = STM32OTA_FLASH;
                      blockcounter = 0;
                      count = myflashfile.blockcnt + 10;    // for timeout
                    }
                    else {
                      UART_DBG.print(" - error after ");
                      UART_DBG.print(millis() - timer, DEC);
                      UART_DBG.println(" ms");
                      stm32ota_state = STM32OTA_ERROR;
                    }
                  }
                }

                break;

        case STM32OTA_FLASH:
                stmUpdateStatus = updInProgress;
                if( blockcounter==0 ) {
                  UART_DBG.println("STM32 ota: flashing");
                  myflashfile.fsfile.seek(0);
                }

                if(count > 0) count--;
                else {
                  UART_DBG.println("STM32 ota: error flashing");
                  stm32ota_state = STM32OTA_ERROR;
                }

                if(blockcounter < myflashfile.blockcnt) {
                  UART_DBG.print("STM32 ota: write block ");
                  UART_DBG.println(blockcounter, DEC);
                  if (FlashBytes(blockcounter, 256) != 0) {
                    stm32ota_state = STM32OTA_ERROR;
                    break;
                  }
                  blockcounter++;
                }  
                else {
                  UART_DBG.println("STM32 ota: write last bytes");
                  if (FlashBytes(myflashfile.blockcnt, myflashfile.lastbytes) != 0) {
                    stm32ota_state = STM32OTA_ERROR;
                    break;
                  }
                  stm32ota_state = STM32OTA_VERIFY;
                  blockcounter = 0;
                  crc.reset();
                }     

                break;


        case STM32OTA_VERIFY:
                if( blockcounter==0 ) {
                  UART_DBG.println("STM32 ota: verify");
                }

                if(blockcounter < myflashfile.blockcnt) {
                  UART_DBG.print("STM32 ota: verify block ");
                  UART_DBG.println(blockcounter, DEC);
                  stm32Read(buffer, STM32STADDR + (blockcounter * 256), 256);
                  for (size_t i = 0; i < 256; i++)
                  {
                    crc.update(buffer[i]);
                  }
                  blockcounter++;
                }  
                else {
                  UART_DBG.println("STM32 ota: verify last bytes");
                  stm32Read(buffer, STM32STADDR + (myflashfile.blockcnt * 256), myflashfile.lastbytes);
                  for (size_t i = 0; i < myflashfile.lastbytes; i++)
                  {
                    crc.update(buffer[i]);
                  }
                  stm32ota_state = STM32OTA_IDLE;
                  tempcrc = crc.finalize();
                  UART_DBG.print("verify --> crc32: 0x"); UART_DBG.println(tempcrc,HEX);
                    

                  if(tempcrc == myflashfile.crc) {
                    UART_DBG.println("STM32 ota: flash checksum matches file checksum");
                    stm32ota_state = STM32OTA_STARTOVER;
                  }
                  else {
                    UART_DBG.println("STM32 ota: error verfying");
                    stm32ota_state = STM32OTA_ERROR;
                  }
                }

                //stm32ota_state = STM32OTA_IDLE;
                break;

        case STM32OTA_STARTOVER:
                myflashfile.fsfile.close();
                UART_DBG.println("STM32 ota: start over");
                ResetSTM32();
                UART_STM32.begin(115200, SERIAL_8N1, STM32_RX, STM32_TX, false, 20000UL);
                delay(1000);
                stm32ota_state = STM32OTA_IDLE;
                stmUpdateStatus = updFinished;
                break;


        case STM32OTA_ERROR:
                myflashfile.fsfile.close();
                UART_DBG.println("STM32 ota: error");
                stmUpdateStatus = updError;
                stm32ota_state = STM32OTA_IDLE;
                break;

        default:
                stm32ota_command = 0;
                stm32ota_state = STM32OTA_IDLE;
                UART_DBG.println("default");
                break;
  }

}


void STM32ota_start(uint8_t command, String thisFileName) {

  if (command == STM32OTA_START) stm32ota_command = STM32OTA_START;
  else if (command == STM32OTA_STARTBLANK) stm32ota_command = STM32OTA_STARTBLANK;
  updateFileName=thisFileName;
  stm32ota_state = STM32OTA_IDLE;
  stmUpdateStatus = updStarted;
  UART_DBG.println("STM32 ota: start");
}


// has to be called prior to stm32 flash transactions
void STM32ota_begin() {
  UART_DBG.println("STM32 ota: begin");
  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  while(UART_STM32.available()) UART_STM32.read();
  UART_DBG.println("STM32 ota: init state machine");
  stmUpdateStatus = updNotStarted;
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
  
  delay(1500);
  digitalWrite(NRST, LOW);
  delay(500);
}


void RunMode()  {    //Tested
  UART_DBG.println("Set Run mode");
  digitalWrite(BOOT0, LOW);
  delay(100);
  digitalWrite(NRST, HIGH);

  delay(150);
  digitalWrite(NRST, LOW);
  delay(500);
}


int PrepareFile(String FileName) {

  CRC32 crc;
  uint8_t buffer;
  
  dir = SPIFFS.open("/");
  
  UART_DBG.print("file found: ");
  UART_DBG.println(FileName);

  myflashfile.fsfile = SPIFFS.open(FileName, "r");

  if (myflashfile.fsfile) {

    myflashfile.size = myflashfile.fsfile.size();
 
    myflashfile.blockcnt = myflashfile.size / 256;
    myflashfile.lastbytes = myflashfile.size % 256;

    UART_DBG.print("--> blocks: "); UART_DBG.println(myflashfile.blockcnt,DEC);
    UART_DBG.print("--> and bytes: "); UART_DBG.println(myflashfile.lastbytes,DEC);

    myflashfile.fsfile.seek(0);

    // calc crc32
    for (size_t i = 0; i < myflashfile.size; i++)
    {
      buffer = myflashfile.fsfile.read();
      crc.update(buffer);
    }

    myflashfile.crc = crc.finalize();
    UART_DBG.print("--> crc32: 0x"); UART_DBG.println(myflashfile.crc,HEX);

    return 0;
  }
  else {    
    UART_DBG.println("error opening");
    return -1;
  }

}


int FlashBytes(int Block, int Bytes) {
  uint8_t binbuffer[256];
  uint8_t cflag;
  size_t readlen;

  readlen = myflashfile.fsfile.read(binbuffer, (size_t) Bytes);

  if (readlen == Bytes) {
    stm32SendCommand(STM32WR);

    while (!UART_STM32.available()) ;
    cflag = UART_STM32.read();

    if (cflag == STM32ACK) {
      if (stm32Address(STM32STADDR + (256 * Block)) == STM32ACK) {
        if (stm32SendData(binbuffer, Bytes-1) == STM32ACK) return 0;
      }
    }
  }

  return -1;
}

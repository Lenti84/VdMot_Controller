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

#include "Arduino.h"
#include "stm32.h"
#include "stm32ota.h"
#include "globals.h"
#include "VdmTask.h"
#include <CRC32.h>

#include <FS.h>
#ifdef USE_LittleFS
  #define SPIFFS LittleFS
  #include <LITTLEFS.h> 
#else
  #include <SPIFFS.h>
#endif 

CStm32 Stm32;


CStm32::CStm32()
{
  stm32ota_command = 0;
  stmUpdPercent = 0;
  stm32ota_state = STM32OTA_IDLE;
  stmUpdateStatus = updNotStarted;
  timeout = 0;
  count = 0;
  timer = 0;
  tempcrc = 0;
}

bool CStm32::waitForSTMResponse (uint32_t timeout_ms)
{
  uint32_t start=millis();
 
  while ((millis() - start) < timeout_ms) {
    if (UART_STM32.available()) return true;
  }
  #ifdef EnvDevelop
    UART_DBG.println("STM32 ota: Time out");
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "STM32 ota: Time out");
  }  
  return false;
}

void CStm32::clearUART_STM32Buffer()
{
   while(UART_STM32.available()) UART_STM32.read();
}

void CStm32::STM32ota_setup() 
{

  // flash interface
  // ATTENTION: on WT32-ETH01 use for UART_STM32 pins: RX2 IO35 and TX2 IO17
  //UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  
  // BOOT and RESET Pin of STM32  
  pinMode(BOOT0, OUTPUT);
  digitalWrite(BOOT0, LOW); 

  pinMode(NRST, OUTPUT);
  digitalWrite(NRST, LOW); 

  stm32ota_command = 0;
  stm32ota_state = STM32OTA_IDLE;
  stmUpdateStatus = updNotStarted;
}

void CStm32::STM32ota_loop() 
{
  switch(stm32ota_state) {

        case STM32OTA_IDLE: 
                if (stm32ota_command == STM32OTA_START) {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: start flash attemp");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG,"STM32 ota: start flash attemp");
                  }  
                  skipsigning = 0;
                  stm32ota_state = STM32OTA_PREPAREFILE;
                  stmUpdPercent = 0;
                  stm32ota_command = 0;
                }
                else if (stm32ota_command == STM32OTA_STARTBLANK) {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: start blank flash attemp");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: start blank flash attemp");
                  }  
                  skipsigning = 1;
                  stm32ota_state = STM32OTA_PREPAREFILE;
                  stmUpdPercent = 0;
                  stm32ota_command = 0;
                }

                break;

        case STM32OTA_PREPAREFILE:
                stmUpdateStatus = updInProgress;
                #ifdef EnvDevelop
                  UART_DBG.println("STM32 ota: prepare file");
                #endif
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                  syslog.log(LOG_DEBUG, "STM32 ota: prepare file");
                }  
                stmUpdateStatus = updStarted;
                if (PrepareFile(updateFileName) == 0) {
                  stm32ota_state = STM32OTA_PREPARE;
                  stmUpdPercent = 5;                  
                }
                else stm32ota_state = STM32OTA_ERROR;

                break;

        case STM32OTA_PREPARE: 
                STM32ota_begin();
                ResetSTM32(true);           
                VdmTask.yieldTask(1500); // stm32 is a little slow
                UART_STM32.flush();     // flush garbage in rx buffer
                clearUART_STM32Buffer();
                if (!skipsigning) {                 
                  UART_STM32.println("DEADBEEF");
                  timeout = 0;
                  count = 0;
                  stm32ota_state = STM32OTA_SENDSIGN;
                }
                else {
                  #ifdef EnvDevelop
                    UART_DBG.println("button BOOT0 must be pressed on BlackPill board!");
                  #endif
                  timeout = 0;
                  stm32ota_state = STM32OTA_INITSTM;
                }

                
        break;

        case STM32OTA_SENDSIGN:                              
                if (timeout == 20) {
                  timeout = 0;
                  UART_STM32.println("DEADBEEF");
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: send sign");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: send sign");
                  }  
                  count++;                
                }
                else timeout++;

                if (count >= 4) stm32ota_state = STM32OTA_ERROR;

                if (UART_STM32.available() >= 6) {
                   #ifdef EnvDevelop 
                    UART_DBG.print("STM32 ota sign received: ") ;
                    UART_DBG.println(UART_STM32.available());
                   #endif
                   memset(buffer,0x0,sizeof(buffer));
                   UART_STM32.readBytes(buffer,6);
                   #ifdef EnvDevelop
                    UART_DBG.print("STM32 ota sign: got answer ");
                    UART_DBG.print((char*) buffer);
                    UART_DBG.println();
                   #endif
                   
                   if (memcmp("BEEFIT",&buffer[0],6) == 0) {                     
                     stm32ota_state = STM32OTA_INITSTM;
                     timeout = 0;
                   }
                   else stm32ota_state = STM32OTA_ERROR;
                }
                break;

        case STM32OTA_INITSTM: 
                #ifdef EnvDevelop
                  if (timeout==0) UART_DBG.println("STM32 ota: ready to beef");
                #endif
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                  syslog.log(LOG_DEBUG, "STM32 ota: ready to beef");
                }  
                if (timeout >= 30) {      // 300 ms
                  UART_STM32.write(STM32INIT);
                  stm32ota_state = STM32OTA_GETID;
                  stmUpdPercent = 10;
                  timeout = 0;
                }
                else timeout++;
                
                break;

        case STM32OTA_GETID:
                #ifdef EnvDevelop 
                  if (timeout==0) {
                    UART_DBG.println("STM32 ota: get id");
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                      syslog.log(LOG_DEBUG, "STM32 ota: get id");
                    }  
                  }
                #endif
                if (timeout >= 3) {
                                    
                 // id = StmOta.stm32GetId();

                  if (StmOta.stm32GetId()) {
                    stm32ota_state = STM32OTA_ERASE_START;
                    stmUpdPercent = 15;
                  }
                  else stm32ota_state = STM32OTA_ERROR;
                }
                else timeout++;

                break;

        case STM32OTA_ERASE_START:
                stmUpdateStatus = updErasing;
                #ifdef EnvDevelop
                  UART_DBG.println("STM32 ota: start erasing"); 
                #endif
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                  syslog.log(LOG_DEBUG, "STM32 ota: start erasing");
                }  
                if (StmOta.stm32ErasenStart() == STM32OK) {
                  timer = millis();                  
                  stm32ota_state = STM32OTA_ERASE_FIN;
                  timeout = 0;
                }
                else stm32ota_state = STM32OTA_ERROR;

                break;


        case STM32OTA_ERASE_FIN:   
                #ifdef EnvDevelop             
                  if (timeout == 0) {
                    UART_DBG.println("STM32 ota: erasing in progress");
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                      syslog.log(LOG_DEBUG, "STM32 ota: erasing in progress");
                    }  
                  }
                #endif
                if(timeout>1000) {
                  #ifdef EnvDevelop
                    UART_DBG.println("--> timeout");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: --> timeout");
                  }  
                  stm32ota_state = STM32OTA_ERROR;
                }
                else {
                  timeout++;
                  if (UART_STM32.available()) {
                    
                    if (UART_STM32.read() == STM32ACK) {
                      #ifdef EnvDevelop
                        UART_DBG.print("--> done using extended erase - took ");
                        UART_DBG.print(millis() - timer, DEC);
                        UART_DBG.println(" ms");
                      #endif
                      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                        syslog.log(LOG_DEBUG, "STM32 ota: --> done using extended erase - took "+String(millis() - timer)+ "ms");
                      }  
                      stm32ota_state = STM32OTA_FLASH;
                      stmUpdPercent = 20;
                      blockcounter = 0;
                      count = myflashfile.blockcnt + 10;    // for timeout
                    }
                    else {
                      #ifdef EnvDevelop
                        UART_DBG.print(" - error after ");
                        UART_DBG.print(millis() - timer, DEC);
                        UART_DBG.println(" ms");
                      #endif
                      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                        syslog.log(LOG_DEBUG, "STM32 ota: - error after "+String(millis() - timer)+ "ms");
                      }  
                      stm32ota_state = STM32OTA_ERROR;
                    }
                  }
                }

                break;

        case STM32OTA_FLASH:
                stmUpdateStatus = updFlashing;
                if( blockcounter==0 ) {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: flashing");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: flashing");
                  }  
                  myflashfile.fsfile.seek(0);
                }

                if(count > 0) count--;
                else {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: error flashing");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: error flashing");
                  }  
                  stm32ota_state = STM32OTA_ERROR;
                }

                if (blockcounter > 0) {
                  if (UART_STM32.read() != STM32ACK) {
                    #ifdef EnvDevelop
                      UART_DBG.println("--> no ACK");
                    #endif
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                      syslog.log(LOG_DEBUG, "STM32 ota: --> no ACK");
                    }  
                    stm32ota_state = STM32OTA_ERROR;
                    break;
                  }
                } 

                stmUpdPercent = 20 + (uint8_t) ((400 * (uint32_t) blockcounter) / (uint32_t) myflashfile.blockcnt / 10); 

                if(blockcounter < myflashfile.blockcnt) {
                  #ifdef EnvDevelop
                    UART_DBG.print("STM32 ota: write block ");
                    UART_DBG.println(blockcounter, DEC);
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: write block "+String(blockcounter));
                  }  
                  if (FlashBytes(blockcounter, STM32OTA_BLOCKSIZE) != 0) {
                    stm32ota_state = STM32OTA_ERROR;
                    break;
                  }
                  blockcounter++;
                }  
                else {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: write last bytes");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: write last bytes");
                  }  
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
                stmUpdateStatus = upVerifying;
                if( blockcounter==0 ) {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: verify");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: verify");
                  }  
                  clearUART_STM32Buffer();
                }

                stmUpdPercent = 60 + (uint8_t) ((400 * (uint32_t) blockcounter) / (uint32_t) myflashfile.blockcnt / 10); 
               
                if(blockcounter < myflashfile.blockcnt) {
                  #ifdef EnvDevelop
                    UART_DBG.print("STM32 ota: verify block ");
                    UART_DBG.println(blockcounter, DEC);
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: verify block "+String(blockcounter));
                  }  
                  if (STM32OK == stm32StartRead(STM32STADDR + (blockcounter * STM32OTA_BLOCKSIZE), STM32OTA_BLOCKSIZE)) {
                    count = 3;
                    stm32ota_state = STM32OTA_VERIFYREAD;                    
                  }
                  else {
                    #ifdef EnvDevelop
                      UART_DBG.println("flash verify --> read command failed");
                    #endif
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                      syslog.log(LOG_DEBUG, "STM32 ota: flash verify --> read command failed");
                    }  
                    stm32ota_state = STM32OTA_ERROR;
                  }
                }  
                else {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: verify last bytes");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: verify last bytes");
                  }  
                  if (STM32OK == stm32StartRead(STM32STADDR + (myflashfile.blockcnt * STM32OTA_BLOCKSIZE), myflashfile.lastbytes)) {
                    count = 3;
                    stm32ota_state = STM32OTA_VERIFYREAD;
                  }
                  else {
                    #ifdef EnvDevelop
                      UART_DBG.println("--> last bytes read command failed");
                    #endif
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                      syslog.log(LOG_DEBUG, "STM32 ota: --> last bytes read command failed");
                    }  
                    stm32ota_state = STM32OTA_ERROR;
                  }
                }
                break;

        case STM32OTA_VERIFYREAD:
                if(count > 0) count--;
                else {
                  #ifdef EnvDevelop
                    UART_DBG.println("STM32 ota: error flash verify read");
                  #endif
                  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STM32 ota: error flash verify read");
                  }  
                  stm32ota_state = STM32OTA_ERROR;
                }
                
                if(blockcounter < myflashfile.blockcnt) {
                  if (UART_STM32.available() == STM32OTA_BLOCKSIZE) {
                    UART_STM32.readBytes(buffer, STM32OTA_BLOCKSIZE);
                    
                    for (size_t i = 0; i < STM32OTA_BLOCKSIZE; i++)
                    {
                      crc.update(buffer[i]);
                    }
                    blockcounter++;
                    stm32ota_state = STM32OTA_VERIFY;
                  }
                }
                else {
                  if (UART_STM32.available() == myflashfile.lastbytes) {
                    UART_STM32.readBytes(buffer, myflashfile.lastbytes);
                    
                    for (size_t i = 0; i < myflashfile.lastbytes; i++)
                    {
                      crc.update(buffer[i]);
                    }
                  
                    tempcrc = crc.finalize();
                    #ifdef EnvDevelop
                      UART_DBG.print("verify --> crc32: 0x"); UART_DBG.println(tempcrc,HEX);                      
                    #endif
                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                      syslog.log(LOG_DEBUG, "STM32 ota: verify --> crc32: 0x"+String(tempcrc,HEX));
                    }  
                    if(tempcrc == myflashfile.crc) {
                      #ifdef EnvDevelop
                        UART_DBG.println("STM32 ota: flash checksum matches file checksum");
                      #endif
                      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                        syslog.log(LOG_DEBUG, "STM32 ota: flash checksum matches file checksum");
                      }  
                      stm32ota_state = STM32OTA_STARTOVER;
                    }
                    else {
                      #ifdef EnvDevelop
                        UART_DBG.println("STM32 ota: error verfying");
                      #endif
                      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                        syslog.log(LOG_DEBUG, "STM32 ota: error verfying");
                      }  
                      stm32ota_state = STM32OTA_ERROR;
                    }
                  }
                }
                break;


        case STM32OTA_STARTOVER:
                stmUpdateStatus = updFinished;
                myflashfile.fsfile.close();
                #ifdef EnvDevelop
                  UART_DBG.println("STM32 ota: start over");
                #endif
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                  syslog.log(LOG_DEBUG, "STM32 ota: start over");
                }  
                VdmTask.yieldTask(2000);
                ResetSTM32(true);
                UART_STM32.begin(115200, SERIAL_8N1, STM32_RX, STM32_TX, false, 20000UL);
                //delay(1000);
                VdmTask.yieldTask(5000);
                stm32ota_state = STM32OTA_IDLE;
                stmUpdateStatus = updRestart;
                Services.restartStmApp(2000);
                break;


        case STM32OTA_ERROR:
                myflashfile.fsfile.close();
                #ifdef EnvDevelop
                  UART_DBG.println("STM32 ota: error");
                #endif
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                  syslog.log(LOG_DEBUG, "STM32 ota: error");
                }  
                stmUpdPercent = 100; 
                stmUpdateStatus = updError;
                stm32ota_state = STM32OTA_IDLE;
                Services.restartStmApp(2000);
                break;

        default:
                stm32ota_command = 0;
                stm32ota_state = STM32OTA_IDLE;
                #ifdef EnvDevelop
                  UART_DBG.println("default");
                #endif
                break;
  }
}


void CStm32::STM32ota_start(uint8_t command, String thisFileName) 
{
  if (command == STM32OTA_START) stm32ota_command = STM32OTA_START;
  else if (command == STM32OTA_STARTBLANK) stm32ota_command = STM32OTA_STARTBLANK;
  updateFileName=thisFileName;
  stm32ota_state = STM32OTA_IDLE;
  stmUpdateStatus = updStarted;
  #ifdef EnvDevelop
    UART_DBG.println("STM32 ota: start");
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
    syslog.log(LOG_DEBUG, "STM32 ota: start");
  }  
}


// has to be called prior to stm32 flash transactions
void CStm32::STM32ota_begin() 
{
  #ifdef EnvDevelop
    UART_DBG.println("STM32 ota: begin");
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
    syslog.log(LOG_DEBUG, "STM32 ota: begin");
  }  
  // flash interface
  // ATTENTION: on WT32-ETH01 use for UART_STM32 pins: RX2 IO35 and TX2 IO17
  UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  clearUART_STM32Buffer();
  #ifdef EnvDevelop
    UART_DBG.println("STM32 ota: init state machine");
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
    syslog.log(LOG_DEBUG, "STM32 ota: init state machine");
  }  
}


void CStm32::ResetSTM32(bool useTask) 
{
  #ifdef EnvDevelop
    UART_DBG.println("Reset STM32");
  #endif
  if (useTask) VdmTask.yieldTask(100); else delay(100); ;
  digitalWrite(NRST, HIGH);  
  if (useTask) VdmTask.yieldTask(150); else delay(150);
  digitalWrite(NRST, LOW);
  if (useTask) VdmTask.yieldTask(150); else delay(150);
}


void CStm32::FlashMode()  
{    //Tested
  #ifdef EnvDevelop
    UART_DBG.println("Set Flash mode");
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
    syslog.log(LOG_DEBUG, "STM32 ota: Set Flash mode");
  }  
  digitalWrite(BOOT0, HIGH);
  delay(100);
  digitalWrite(NRST, HIGH);
  
  delay(1500);
  digitalWrite(NRST, LOW);
  delay(500);
}


void CStm32::RunMode()  
{    //Tested
  #ifdef EnvDevelop
    UART_DBG.println("Set Run mode");
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
    syslog.log(LOG_DEBUG, "STM32 ota: Set Run mode");
  }  
  digitalWrite(BOOT0, LOW);
  delay(100);
  digitalWrite(NRST, HIGH);

  delay(150);
  digitalWrite(NRST, LOW);
  delay(500);
}


int CStm32::PrepareFile(String FileName) 
{

  CRC32 crc;
  uint8_t buffer;
  
  //dir = SPIFFS.open("/");

  myflashfile.fsfile = SPIFFS.open(FileName, "r");

  if (myflashfile.fsfile) {

    myflashfile.size = myflashfile.fsfile.size();
 
    myflashfile.blockcnt = myflashfile.size / STM32OTA_BLOCKSIZE;
    myflashfile.lastbytes = myflashfile.size % STM32OTA_BLOCKSIZE;
    #ifdef EnvDevelop
      UART_DBG.print("--> filename: "); UART_DBG.println(FileName);
      UART_DBG.print("--> size: "); UART_DBG.println(myflashfile.size,DEC);
      UART_DBG.print("--> blocks: "); UART_DBG.println(myflashfile.blockcnt,DEC);
      UART_DBG.print("--> and bytes: "); UART_DBG.println(myflashfile.lastbytes,DEC);
    #endif
    myflashfile.fsfile.seek(0);

    // calc crc32
    for (size_t i = 0; i < myflashfile.size; i++)
    {
      buffer = myflashfile.fsfile.read();
      crc.update(buffer);
    }

    myflashfile.crc = crc.finalize();
    #ifdef EnvDevelop
      UART_DBG.print("--> crc32: 0x"); UART_DBG.println(myflashfile.crc,HEX);
    #endif
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "STM32 ota: get File --> crc32: 0x"+String(myflashfile.crc,HEX));
    }  
    return 0;
  }
  else {    
    #ifdef EnvDevelop
      UART_DBG.println("error opening");
    #endif
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "STM32 ota: get File --> error opening");
    }  
    return -1;
  }
}


int CStm32::FlashBytes(int Block, int Bytes) 
{
  uint8_t binbuffer[STM32OTA_BLOCKSIZE+1];
  uint8_t cflag;
  size_t readlen;
  uint8_t response;

  myflashfile.fsfile.seek(STM32OTA_BLOCKSIZE*Block);
  readlen = myflashfile.fsfile.read(binbuffer, (size_t) Bytes);
  
  #ifdef OTADebug
    UART_DBG.print("Flash STM : Readlen = ");
    UART_DBG.print(readlen,DEC);
    UART_DBG.print(" Bytes = ");
    UART_DBG.println(Bytes,DEC);
  #endif
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "STM32 ota: Flash STM : Readlen ="+String(readlen)+" Bytes = "+String(Bytes));
  }  
  // append checksum to buffer
  binbuffer[Bytes] = StmOta.getChecksum(binbuffer, Bytes-1);

  if (readlen == Bytes) {
    clearUART_STM32Buffer();
    StmOta.stm32SendCommand(STM32WR);

    if (!waitForSTMResponse(10000)) return -1;
    cflag = UART_STM32.read();

    if (cflag == STM32ACK) {
      response = StmOta.stm32Address(STM32STADDR + (STM32OTA_BLOCKSIZE * Block));
      #ifdef EnvDevelop
        UART_DBG.print(" OTA stm32Address response : 0x");
        UART_DBG.println(response,HEX);
      #endif
      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        syslog.log(LOG_DEBUG, "STM32 ota: OTA stm32Address response : 0x"+String(response));
      }  
      if (response == STM32ACK) {
        UART_STM32.write(Bytes-1);                 // length of data
        UART_STM32.flush();
        delayMicroseconds(otaDelayFlash);
        for (int x = 0; x<Bytes+1; x++) {
          UART_STM32.write(binbuffer[x]);  
          UART_STM32.flush();
          delayMicroseconds(otaDelayFlash);
        }
        //UART_STM32.write(binbuffer, Bytes + 1);    // data + checksum
        return 0;
      } else {
        #ifdef EnvDevelop
          UART_DBG.println(" OTA Error stm32Address ");
        #endif
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
          syslog.log(LOG_DEBUG, "STM32 ota: OTA Error stm32Address");
        }  
      }
    } else {
      #ifdef EnvDevelop
        UART_DBG.print(" OTA Error cflag = ");
        UART_DBG.println(cflag,DEC);  
      #endif
      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
        syslog.log(LOG_DEBUG, "STM32 ota: OTA Error cflag = "+String(cflag));
      }  
    }
  }

  return -1;
}



// unTested
uint8_t CStm32::stm32StartRead(uint32_t rdaddress, uint16_t rdlen) 
{
  // send read request
  StmOta.stm32SendCommand(STM32RD);

  delayMicroseconds(otaDelayFlash);  

  // wait for ACK
  if (!waitForSTMResponse(10000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK) {

    // send read address
    // got ACK?
    if (StmOta.stm32Address(rdaddress) == STM32ACK) {
      // send read length
      StmOta.stm32SendCommand(rdlen - 1);

      delayMicroseconds(otaDelayFlash);  
      if (!waitForSTMResponse(10000)) return STM32ERR;
      if (UART_STM32.read() == STM32ACK) return STM32OK;
      else return STM32ERR;
    }
    else return STM32ERR;
  }
  else {
    #ifdef EnvDevelop
      UART_DBG.println("error: STM32RD no ACK");
    #endif
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "STM32 ota: error: STM32RD no ACK");
    }  
    return STM32ERR;
  }
  // wait for ACK
  
  if (!waitForSTMResponse(10000)) return STM32ERR;
  if (UART_STM32.read() == STM32ACK) {
    return STM32OK;    
  }
  
  return STM32ERR;
}
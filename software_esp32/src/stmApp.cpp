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
#include <ArduinoJson.h>
#include <Arduino.h>
#include "stmApp.h"
#include <WiFi.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include "VdmNet.h"
#include "VdmSystem.h"
#include "VdmConfig.h"
#include "stm32.h"
#include "Queue.h"


#define     MAX_CMD_LEN     10
#define     MAX_ARG_LEN     120

static const char* commCmds [] = 
               {APP_PRE_SETTARGETPOS,APP_PRE_GETONEWIRECNT,APP_PRE_GETONEWIREDATA,
                APP_PRE_SETONEWIRESEARCH,APP_PRE_SETALLVLVOPEN,APP_PRE_GETVLVDATA,
                APP_PRE_SETVLLEARN,APP_PRE_GETVERSION,APP_PRE_GETTARGETPOS,
                APP_PRE_SETMOTCHARS,APP_PRE_GETMOTCHARS,APP_PRE_SETVLVSENSOR,
                APP_PRE_GETONEWIRESETT,APP_PRE_SETLEARNMOVEM,APP_PRE_GETLEARNMOVEM,
                APP_PRE_GETVLSTATUS,APP_PRE_SETDETECTVLV,APP_PRE_MATCHSENS,
                APP_PRE_SOFTRESET,NULL};


CStmApp StmApp;

CStmApp::CStmApp() 
{
    settarget_check =false;
    tempIndex=0;
    checkTempsCount = 0;
    cmd_buffer = "";
    found = false;
    bufptr = buffer;
    buflen = 0;
    argcnt = 0;
    arg0ptr = arg0;
	arg1ptr = arg1;
    arg2ptr = arg2;
    arg3ptr = arg3;
    arg4ptr = arg4;
    arg5ptr = arg5;
	argcnt = 0;
    tempsPrivCount=0;
    tempsCount=0;
    setTempIdxActive=false;
    waitForFinishQueue=false;
    setMotorCharsActive=false;
    motorChars.startOnPower=50;
    fastQueueMode=false;
    stmStatus=STM_NOT_READY;
    getindex = 0;
    memset(cmd,0x0,sizeof(cmd));
    appState=APP_IDLE;
    timeout = 0;
    retry = 0;
    cnt_alive = COMM_ALIVE_CYCLE;
    appRetry=0;
    appTimeOuts=0;
    commstate=COMM_IDLE;
    matchSensorRequest=false;
}

void  CStmApp::app_setup() {
    UART_STM32.setRxBufferSize (2000);
    UART_STM32.begin(115200, SERIAL_8N1, STM32_RX, STM32_TX, false, 20000UL); 
    
    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        actuators[x].actual_position = 100;
        actuators[x].target_position = 0;
        actuators[x].meancurrent = 342;
        actuators[x].state = VLV_STATE_START; //VLV_STATE_IDLE;
        actuators[x].temp1 = -500;
        actuators[x].temp2 = -500;
        actuators[x].tIdx1 = 0;
        actuators[x].tIdx2 = 0;
   }

    for (uint8_t x = 0; x<ACTUATOR_COUNT; x++) {
        target_position_mirror[x] = actuators[x].target_position;
    }
    commstate=COMM_IDLE;
    UART_DBG.println("application setup finished");
}

void  CStmApp::setupStartPosition(uint8_t thisStartPosition) 
{
    for (uint8_t x = 0;x<ACTUATOR_COUNT;x++) {
        actuators[x].target_position = thisStartPosition;
        target_position_mirror[x] = actuators[x].target_position;
   }
   UART_DBG.println("application setup start position finished");
}


int16_t CStmApp::ConvertCF(int16_t cValue)
{
    if (VdmConfig.configFlash.systemConfig.celsiusFahrenheit==0) {
        return (cValue);
    } else {
        float F=((float) cValue)/10 * 1.8 + 32;
        return (int16_t(F*10+0.5));
    }
} 

int16_t CStmApp::getTOffset(uint8_t tIdx)
{
    int16_t result = 0;
    if (tIdx>0) {
        result = VdmConfig.configFlash.tempsConfig.tempConfig[tIdx].offset;
    } 
    return (result); 
}

bool CStmApp::checkNewTarget() {
    for (uint8_t i=0; i<ACTUATOR_COUNT; i++) {
        if (VdmConfig.configFlash.valvesConfig.valveConfig[i].active) {
            // check if target has changed
            if(target_position_mirror[i] != actuators[i].target_position) return (true);
        }
    }
    return (false);
}

void CStmApp::valvesCalibration()
{
    uint8_t i=255;
    app_cmd(APP_PRE_SETVLLEARN,String(i));
}

void CStmApp::valvesAssembly()
{
    app_cmd(APP_PRE_SETALLVLVOPEN, "255");
}

void CStmApp::valvesDetect()
{
    app_cmd(APP_PRE_SETDETECTVLV, "255");
}


void CStmApp::getParametersFromSTM()
{
    fastQueueMode=true;  
    app_cmd(APP_PRE_GETVERSION);
    app_cmd(APP_PRE_GETMOTCHARS);
    app_cmd(APP_PRE_GETLEARNMOVEM);
    app_cmd(APP_PRE_GETONEWIRECNT,String(255));
    app_cmd(APP_PRE_GETONEWIRESETT,String(255));
    app_cmd(APP_PRE_GETVLSTATUS);
}

// soft reset of STM32 ensures that eeprom content is saved before restart
void CStmApp::softReset()
{   
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
        syslog.log(LOG_DEBUG, "stmApp: soft reset of STM32");
    }
    UART_DBG.println("stmApp: send soft reset command");
    app_cmd(APP_PRE_SOFTRESET);
}

void CStmApp::scanTemps()
{
    tempsCount=0;
    for (uint8_t i=0;i<TEMP_SENSORS_COUNT;i++) {
        memset(tempsId[i].id,0x0,sizeof(tempsId[i].id));
    }
    app_cmd(APP_PRE_SETONEWIRESEARCH);
}

void CStmApp::matchSensors()
{
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
        syslog.log(LOG_DEBUG, "stmApp: send match sensor request");
    }
    UART_DBG.println("stmApp: send match sensor request");
    app_cmd(APP_PRE_MATCHSENS);      
}

void CStmApp::setTempIdx()
{
    String s1;
    String s2;
    uint8_t tIdx1;
    uint8_t tIdx2;
    
    for (uint8_t i=0;i<ACTUATOR_COUNT;i++) {
        tIdx1=StmApp.actuators[i].tIdx1;
        tIdx2=StmApp.actuators[i].tIdx2;
        s1=APP_PRE_UNDEFSENSOR;
        s2=APP_PRE_UNDEFSENSOR;
        if (tIdx1>0) {
            if (strlen(VdmConfig.configFlash.tempsConfig.tempConfig[tIdx1-1].ID)>0) {
                s1=VdmConfig.configFlash.tempsConfig.tempConfig[tIdx1-1].ID;
            }   
        }
        if (tIdx2>0) {
            if (strlen(VdmConfig.configFlash.tempsConfig.tempConfig[tIdx2-1].ID)>0) {
                s2=VdmConfig.configFlash.tempsConfig.tempConfig[tIdx2-1].ID; 
            }
        }
        
        waitForFinishQueue=true;
        fastQueueMode=true;
        app_cmd(APP_PRE_SETVLVSENSOR,String(i)+ARG_DELIMITER+s1+ARG_DELIMITER+s2);
        fastQueueMode=true;

        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
            syslog.log(LOG_DEBUG,"stmApp: "+String(APP_PRE_SETVLVSENSOR)+" "+String(i)+ARG_DELIMITER+s1+ARG_DELIMITER+s2);
        }
    }
    fastQueueMode=true;
}

void CStmApp::setLearnAfterMovements()
{
    waitForFinishQueue=true;
    app_cmd(APP_PRE_SETLEARNMOVEM,String(learnAfterMovements));
    fastQueueMode=true;   
}

void CStmApp::setMotorChars()
{
    waitForFinishQueue=true;
    app_cmd(APP_PRE_SETMOTCHARS,String(motorChars.maxLowCurrent)+ARG_DELIMITER+String(motorChars.maxHighCurrent)+ARG_DELIMITER+String(motorChars.startOnPower));   
    fastQueueMode=true;
}

void  CStmApp::app_loop() 
{
    static int match_counter = 0;

    app_check_data();
    appHandler();

    // maybe find a better place for this code
    if(matchSensorRequest == true) {
        match_counter++;
        if (match_counter > 30) {
            match_counter = 0;
            matchSensorRequest = false;
            matchSensors();
        }
    }
}

bool CStmApp::checkCmdIsAvailable (String thisCmd)
{   
    uint8_t i=0;
    while (commCmds[i] != NULL) {
        if (memcmp((const char*) &thisCmd,commCmds[i],5)==0) {
            return(true);
        }
        i++;
    }
    UART_DBG.println("Command not found "+String(thisCmd));
    return (false);
}

void  CStmApp::app_cmd(String command,String args) 
{    
    if (checkCmdIsAvailable(command)) {
        String s=command+ARG_DELIMITER;
        if (args!="") s+=args+ARG_DELIMITER;
        UART_DBG.println("push "+s);
        Queue.push(s);
    }
    else {
        if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
            syslog.log(LOG_DEBUG, "STMApp: app_cmd - command not found");
        }
    }
}

int8_t CStmApp::findTempID(char* ID) 
{
    for (uint8_t i=0;i<TEMP_SENSORS_COUNT;i++) {
        if (strncmp(ID,VdmConfig.configFlash.tempsConfig.tempConfig[i].ID,sizeof(VdmConfig.configFlash.tempsConfig.tempConfig[i].ID))==0) {
           return (i);
        }
    }
    return -1;
}

void CStmApp::setSensorIndex(uint8_t valveIndex,char* sensor1,char* sensor2)
{
    int8_t sensorIdx;
    sensorIdx=findTempID(sensor1);
    if (sensorIdx>=0) actuators[valveIndex].tIdx1=sensorIdx+1; else actuators[valveIndex].tIdx1=0;
    sensorIdx=findTempID(sensor2);
    if (sensorIdx>=0) actuators[valveIndex].tIdx2=sensorIdx+1; else actuators[valveIndex].tIdx2=0;
}

void  CStmApp::app_check_data() 
{
    static int errorcnt = 0;
    int availcnt;

    #ifdef STMSimulation
        #warning STMSimulation active
        appState=APP_IDLE;  
        stmStatus=STM_READY; 
        cmd_buffer=""; 
        return;
    #endif

    availcnt = UART_STM32.available();
    found=false;
   
    if (availcnt > 0) {

        for (int c = 0; c < availcnt; c++)
        {           
            *bufptr++ = (char) UART_STM32.read();
            buflen++;  
        }
        if (buflen>=sizeof(buffer)-1) {
            *bufptr='\r';
        }        
    }  

    // if there is a little in the buffer
    if(buflen >= 5) 
    {
        for (uint16_t c = 0; c < buflen; c++)
        {     
            if (buffer[c] == '\r') {
                errorcnt = 0;
                buffer[c] = '\0';
                found = true;
                buflen = 0;         // reset counter
                bufptr = buffer;    // reset ptr    
                UART_STM32.read();  // read possible \n
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
                    syslog.log(LOG_DEBUG, "STMApp:found new data packet: >" + String(buffer) + "<");
                }
            }
        }
        
        // timeout incomplete recv sequences
        if (!found) errorcnt++;
        if (errorcnt > 5) {
            syslog.log(LOG_DEBUG, "incomplete buffer : >" + String(buffer) + "<");

            errorcnt = 0;
            buflen = 0;
            bufptr = buffer;            
        }
    }
   

//    if (found) {
    if(!found) 
    {
        //syslog.log(LOG_DEBUG, "incomplete buffer : >" + String(buffer) + "<");
    }
    else 
    {
        found = false;

        if (stmStatus==STM_NOT_READY) stmStatus=STM_READY;
        // devide buffer into command and data
		// ****************************************
		cmdptr = buffer;
        
		for(int x=0;x<7;x++) {
			cmdptrend = strchr(cmdptr,' ');
			if (cmdptrend!=NULL) {
				*cmdptrend = '\0';
				if(x==0) 		strncpy(cmd,cmdptr,sizeof(cmd)-1);		// command
				else if(x==1) { 
                    memset (arg0,0x0,sizeof(arg0));
                    strncpy(arg0,cmdptr,sizeof(arg0)-1); 
                    argcnt=1;	
                } 	// 1st argument
				else if(x==2) {	
                    memset (arg1,0x0,sizeof(arg1));
                    strncpy(arg1,cmdptr,sizeof(arg1)-1); 
                    argcnt=2;	
                } 	// 2nd argument
                else if(x==3) {	
                    memset (arg2,0x0,sizeof(arg2));
                    strncpy(arg2,cmdptr,sizeof(arg2)-1); 
                    argcnt=3;	
                } 	// 3rd argument
                else if(x==4) {
                    memset (arg3,0x0,sizeof(arg3));	
                    strncpy(arg3,cmdptr,sizeof(arg3)-1); 
                    argcnt=4;	
                } 	// 4th argument
                else if(x==5) {	
                    memset (arg4,0x0,sizeof(arg4));
                    strncpy(arg4,cmdptr,sizeof(arg4)-1); 
                    argcnt=5;	
                } 	// 5th argument

                else if(x==6) {	
                    memset (arg5,0x0,sizeof(arg5));
                    strncpy(arg5,cmdptr,sizeof(arg5)-1); 
                    argcnt=6;	
                } 	// 6th argument
				cmdptr = cmdptrend + 1;
			}
		}
 
		// get actual values
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		if(memcmp(APP_PRE_GETACTUALPOS,cmd,5) == 0) {
            if(argcnt == 2) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:actual position answer "+String(arg0ptr)+" : "+String(arg1ptr));
                }
                actuators[atoi(arg0ptr)].actual_position = atoi(arg1ptr);
            }
            appState=APP_IDLE;
        }

        // get mean current 
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETMEANCURR,cmd,5) == 0) {
            if(argcnt == 2) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:mean current answer "+String(arg0ptr)+" : "+String(arg1ptr));  
                }
                actuators[atoi(arg0ptr)].meancurrent = atoi(arg1ptr);
            }
            appState=APP_IDLE;
        }
    
        // get valve status
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETVLSTATUS,cmd,5) == 0) { 
            if(argcnt == 2) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:valve status "+String(arg1ptr));
                }
                uint8_t nActuators = atoi(arg0ptr);
                char* cmdptr;
                char* ps=arg1ptr;
                for (uint8_t idx=0; idx<nActuators;idx++) {
                    if ((cmdptr=strchr(ps,','))!=NULL) *cmdptr='\0';
                    actuators[idx].state = atoi(ps);
                    if (cmdptr!=NULL) ps=cmdptr+1;
                    idx++;
                }  
                stmStatus=STM_READ_ALL_FROM_QUEUE;    
            }
            appState=APP_IDLE;
        }

        // get set target answer
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETTARGETPOS,cmd,5) == 0) {
            if(argcnt == 0) {
                settarget_check = true; 
            }
            if (memcmp(cmd,(const void*) &cmd_buffer,5) ==0) cmd_buffer="";
            appState=APP_IDLE;
        }
        // get target position
            // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            else if(memcmp(APP_PRE_GETTARGETPOS,cmd,5) == 0) {
                if(argcnt == 2) {
                    actuators[atoi(arg0ptr)].target_position = atoi(arg1ptr);
                }
                appState=APP_IDLE;
            }

        // get data values
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETVLVDATA,cmd,5) == 0) {
            if(argcnt == 6) {
                uint8_t idx=atoi(arg0ptr);
                if(idx < ACTUATOR_COUNT) {
                    actuators[idx].actual_position = atoi(arg1ptr);
                    actuators[idx].meancurrent = atoi(arg2ptr);
                    actuators[idx].state = atoi(arg3ptr);
                    actuators[idx].temp1 = ConvertCF(atoi(arg4ptr))+getTOffset(actuators[idx].tIdx1);
                    actuators[idx].temp2 =  ConvertCF(atoi(arg5ptr))+getTOffset(actuators[idx].tIdx2);

                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                        syslog.log(LOG_DEBUG, "STMApp:got valve data #"+String(arg0ptr)+" pos:"+String(arg1ptr)+
                        " mean:"+String(arg2ptr)+" state:"+String(arg3ptr)+" t1:"+String(arg4ptr)+" t2:"+String(arg5ptr));
                    }
                }
            } 
            appState=APP_IDLE;
        }

        // get onewire data answer
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        
        else if(memcmp(APP_PRE_GETONEWIRECNT,cmd,5) == 0) {
            if(argcnt == 1) {
                tempsPrivCount= atoi(arg0ptr); 
                if (tempsPrivCount!=tempsCount) app_cmd(APP_PRE_GETONEWIRECNT,String(255));
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:one wire count "+String(tempsPrivCount));
                }       
            }
            if(argcnt == 2) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:one wire data "+String(arg1ptr));
                } 
                tempsPrivCount= atoi(arg0ptr);
                if (tempsPrivCount>0) {
                    char* cmdptr;
                    char* ps=arg1ptr;
                    for (uint8_t idx=0; idx<tempsPrivCount;idx++) {
                        if ((cmdptr=strchr(ps,','))!=NULL) *cmdptr='\0';
                        strncpy(tempsId[idx].id,ps,sizeof(tempsId[idx].id));
                        ps=cmdptr+1;
                        idx++;
                    }
                }
                tempsCount=tempsPrivCount;
            }
            appState=APP_IDLE;
        }

		else if(memcmp(APP_PRE_GETONEWIREDATA,cmd,5) == 0) {
            if(argcnt == 2) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:one wire data "+String(arg0ptr)+":"+String(arg1ptr));
                } 
                strncpy(tempsId[tempIndex].id,arg0ptr,sizeof(tempsId[tempIndex].id));

                int8_t idx=findTempID(arg0ptr);
                if (idx>=0) {
                    memset(temps[idx].id,0x0,sizeof(temps[idx].id)); 
                    strncpy(temps[idx].id,arg0ptr,sizeof(temps[idx].id));
                    int16_t cValue=atoi(arg1ptr);
                    temps[idx].temperature=ConvertCF(cValue)+VdmConfig.configFlash.tempsConfig.tempConfig[idx].offset;
                } 
                tempIndex++;
                if (tempIndex>=tempsCount) {  // all temp sensors read
                    tempIndex=0;
                }
            }
            // handle incomplete messages to not block 1-wire request cycle
            else if(argcnt == 1) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:one wire data - only 1 arg");
                }
                tempIndex++;
                if (tempIndex>=tempsCount) {  // all temp sensors read
                    tempIndex=0;
                }
            }
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_GETVERSION,cmd,5) == 0) {
            if(argcnt > 0) {
                VdmSystem.stmVersion=arg0ptr; 
                VdmSystem.stmBuild=0;
            }
            if(argcnt > 1) {
                 VdmSystem.stmBuild=atoi(arg1ptr);
            }
            if (memcmp(cmd,(const void*) &cmd_buffer,5) ==0) cmd_buffer="";
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_GETONEWIRESETT,cmd,5) == 0) {
            if(argcnt == 3) {
                setSensorIndex(atoi(arg0ptr),arg1ptr,arg2ptr);
            }
            if(argcnt == 2) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG,"STMApp:one wire settings data "+String(arg0ptr)+" : "+String(arg1ptr));
                }   
                uint8_t nItems= atoi(arg0ptr);
                if (nItems>0) {
                    char* cmdptr;
                    char* ps=arg1ptr;
                    for (uint8_t idx=0; idx<nItems;idx++) {
                        if ((cmdptr=strchr(ps,','))!=NULL) *cmdptr='\0';
                        strncpy(arg4ptr,ps,sizeof(arg4));
                        if (cmdptr!=NULL) ps=cmdptr+1;
                        if ((cmdptr=strchr(ps,','))!=NULL) *cmdptr='\0';
                        strncpy(arg5ptr,ps,sizeof(arg5));
                        setSensorIndex(idx,arg4ptr,arg5ptr);
                        if (cmdptr!=NULL) ps=cmdptr+1;
                    }
                }
                
            }
            appState=APP_IDLE;
        }
        
        else if(memcmp(APP_PRE_SETONEWIRESEARCH,cmd,5) == 0) {
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_SETLEARNMOVEM,cmd,5) == 0) {
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_GETLEARNMOVEM,cmd,5) == 0) {
            if(argcnt == 1) {
                learnAfterMovements=atoi(arg0ptr);
            }
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_SETVLVSENSOR,cmd,5) == 0) {
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_SETALLVLVOPEN,cmd,5) == 0) {
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_SETVLLEARN,cmd,5) == 0) {
            appState=APP_IDLE;
        }
        
        else if(memcmp(APP_PRE_SETDETECTVLV,cmd,5) == 0) {
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_SETMOTCHARS,cmd,5) == 0) {
            appState=APP_IDLE;
        }

        else if(memcmp(APP_PRE_GETMOTCHARS,cmd,5) == 0) {
            if(argcnt == 2) {
                motorChars.maxLowCurrent=atoi(arg0ptr);
                motorChars.maxHighCurrent=atoi(arg1ptr);
            }
            if(argcnt == 3) {
                motorChars.maxLowCurrent=atoi(arg0ptr);
                motorChars.maxHighCurrent=atoi(arg1ptr);
                motorChars.startOnPower=atoi(arg2ptr);
                setupStartPosition(motorChars.startOnPower);
            }
            appState=APP_IDLE;
        }

        // match sensors request
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_MATCHSENS,&cmd[0],5) == 0) {			
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                syslog.log(LOG_DEBUG, "stmApp: got match sensor answer");
            }
            appState=APP_IDLE;
		}

        // very important to reset to idle if no valid command was found
        else {
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                syslog.log(LOG_DEBUG, "unknown command: >" + String(cmd) + "<");
            }
            appState=APP_IDLE;
        }

        cmd_buffer="";
        buffer[0] = '\0';
    }
}


void CStmApp::appHandler()
{
    switch (appState) {
        case APP_IDLE: 
            appRetry=0;   
            app_comm_machine();
            break;
        
        case APP_PENDING: 
            appRetry ++;
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {		
                syslog.log(LOG_DEBUG,"STMApp:App retries "+String(appRetry)+String(":")+String(commstate));
            }
            if (appRetry>maxAppRetries) {
                appState=APP_TIMEOUT;
                appRetry=0;
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {		
                    syslog.log(LOG_DEBUG,"STMApp:Max app retries reached, set app to timeout");
                }
            }
            break;
        
        case APP_TIMEOUT:
            appTimeOuts++;
            if (appTimeOuts>maxAppTimeOuts) {
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {		
                    syslog.log(LOG_DEBUG,"STMApp:Max app timeouts reached, reset STM");
                } 
                VdmSystem.setSystemState(systemStateError,systemMsgSTMReset);
                appTimeOuts=0; 
                Stm32.ResetSTM32(true); 
                appState=APP_IDLE; 
            } else {
                commstate = COMM_GETDATA;
                app_comm_machine();   
            } 
            break;
        
        default:    
            appState=APP_IDLE;
            break;
    }
}


void  CStmApp::app_comm_machine()
{
    char sendbuffer[30];
    char valbuffer[10];

    switch(commstate) {

        case COMM_IDLE: 
                appState=APP_IDLE;
                if (checkNewTarget()) {
                    commstate = COMM_SENDTARGET;
                } else {
                    commstate = COMM_GETDATA;
                }
                break;

        // send target position values
        case COMM_SENDTARGET:
                commstate = COMM_GETDATA;
                
                for (uint8_t x=0; x<ACTUATOR_COUNT; x++) {
                    if (VdmConfig.configFlash.valvesConfig.valveConfig[x].active) {
                        // check if target has changed
                        if(target_position_mirror[x] != actuators[x].target_position)
                        {
                            target_position_mirror[x] = actuators[x].target_position;
                            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                                syslog.log(LOG_DEBUG, "STMApp:valve position has changed : "+String(VdmConfig.configFlash.valvesConfig.valveConfig[x].name)+"(#"+String(x+1)+") = "+String(actuators[x].target_position));
                            }
                            settarget_check=false;
                            memset(sendbuffer,0x0,sizeof(sendbuffer));
                            strcat(sendbuffer, APP_PRE_SETTARGETPOS);
                            strcat(sendbuffer, " ");

                            itoa(x, valbuffer, 10);
                            strcat(sendbuffer, valbuffer);
                            strcat(sendbuffer, " ");   

                            itoa(actuators[x].target_position, valbuffer, 10);      
                            strcat(sendbuffer, valbuffer);
                            strcat(sendbuffer, " ");
                            
                            UART_STM32.println(sendbuffer);
                            UART_DBG.println("valve position has changed : "+String(x)+" = "+String(actuators[x].target_position));
                            // break loop, next valve will be served in next cycle
                            appState=APP_PENDING;
                            break;

                            timeout = 10;
                        }   
                    }
                }
                
                break;

        // check correct transmission of target value
        case COMM_CHECKTARGET:
                if (timeout>0) timeout--;
                else {
                    // answer received
                    if (settarget_check) {                        
                        commstate = COMM_GETDATA; 
                    }
                    else {
                        if (retry>0) retry--;
                        else commstate = COMM_GETDATA; 
                        commstate = COMM_SENDTARGET;
                    }
                }
                
                break;

        case COMM_GETDATA:  
                commstate =  COMM_GETONEWIRECOUNT;
                
                // walk through all valves, one per cycle
                if (getindex<ACTUATOR_COUNT-1) getindex++; else getindex=0;

                memset(sendbuffer,0x0,sizeof(sendbuffer));
                strcat(sendbuffer, APP_PRE_GETVLVDATA);
                strcat(sendbuffer, " ");

                itoa(getindex, valbuffer, 10);
                strcat(sendbuffer, valbuffer);
                strcat(sendbuffer, " ");   

                UART_STM32.println(sendbuffer);
                
                if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG, sendbuffer);
                }
                appState=APP_PENDING;
                break;

        case COMM_GETONEWIRECOUNT:
                if (tempsPrivCount>0) {
                    commstate = COMM_GETONEWIRE;
                } else {
                    commstate =  COMM_HANDLEQUEUE;   
                }
                if (checkTempsCount==0) {
                    memset(sendbuffer,0x0,sizeof(sendbuffer));
                    
                    strcat(sendbuffer, APP_PRE_GETONEWIRECNT);
                    strcat(sendbuffer, " ");
                    UART_STM32.println(sendbuffer);
                    checkTempsCount=20;
                    appState=APP_PENDING;

                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                        syslog.log(LOG_DEBUG,"get one wire count");
                    } 
                }    
                checkTempsCount--;
                break;

        case COMM_GETONEWIRE:
                commstate = COMM_HANDLEQUEUE;
                if (tempsPrivCount>0) {
                    memset(sendbuffer,0x0,sizeof(sendbuffer));
                    
                    strcat(sendbuffer, APP_PRE_GETONEWIREDATA);
                    strcat(sendbuffer, " ");

                    itoa(tempIndex, valbuffer, 10);
                    strcat(sendbuffer, valbuffer);
                    strcat(sendbuffer, " ");   

                    UART_STM32.println(sendbuffer);
                    appState=APP_PENDING;

                    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
                        syslog.log(LOG_DEBUG,"get one wire data: " + String(sendbuffer));
                    } 
                }
                break;

        case COMM_HANDLEQUEUE:
                if ((cmd_buffer=="") && (Queue.available()>0) && (stmStatus>=STM_READY)) {
                    memset(&cmd_buffer,0x0,sizeof(cmd_buffer));
                    cmd_buffer=Queue.pop();
                    UART_DBG.println("pop "+String(cmd_buffer));
                    UART_STM32.println(cmd_buffer);
                    appState=APP_PENDING;
                }
                if (fastQueueMode) {
                    if (Queue.available()==0) {
                        fastQueueMode=false;
                        commstate = COMM_IDLE;
                    }
                } else commstate =COMM_IDLE;
                
                break;
        
        default:    
                commstate = COMM_IDLE;
                break;
    }
}

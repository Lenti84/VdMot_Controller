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
#include "telnet.h"
#include <WiFi.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include "Logger.h"
#include "VdmNet.h"
#include "stm32.h"

#define     MAX_CMD_LEN     10
#define     MAX_ARG_LEN     120

actuator_str    actuators[ACTUATOR_COUNT];

String cmd_buffer = "";

unsigned char target_position_mirror[ACTUATOR_COUNT];

int settarget_check = 0;

uint8_t     stm32alive = 0;           // 0 - not alive, >0 - alive 

char calc_checksum (char *dataptr);
void app_check_data();
void app_cmd(int command);
void app_comm_machine();
void app_alive_check();
void app_web_cmd_check();

void app_setup() {

    UART_STM32.begin(115200, SERIAL_8N1, STM32_RX, STM32_TX, false, 20000UL);
    //#warning fixme
    //UART_STM32.begin(9600, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);


    for (unsigned int x = 0;x<ACTUATOR_COUNT;x++) {
        actuators[x].actual_position = 100;
        actuators[x].target_position = 0;
        actuators[x].meancurrent = 342;
        actuators[x].state = 1;
        actuators[x].temperature = -500;
    }

    for(unsigned int x = 0; x<ACTUATOR_COUNT; x++)
    {
        target_position_mirror[x] = actuators[x].target_position;
    }

    
    UART_DBG.println("application setup finished");
}


void app_loop() {
            app_check_data();
            app_comm_machine(); 
            app_alive_check();
            app_web_cmd_check(); 
}

void app_cmd(String command) {    
    cmd_buffer = command;
}

char calc_checksum (char *dataptr) {

    char result = 0;

    while(*dataptr != '\0') {
        result += *dataptr++;
    }

    return result;
}

void app_check_data() {

    static char buffer[1200];
    static char *bufptr = buffer;
    static unsigned int buflen = 0;
    int availcnt;
    int found = 0;
    
    char*       cmdptr;
    char*	    cmdptrend;
    char        cmd[10];
    char		arg0[1200];	
    char        arg1[20];		
    char        arg2[20];
    char        arg3[20];
    char        arg4[20];
	char*		arg0ptr = arg0;
	char*		arg1ptr = arg1;
    char*		arg2ptr = arg2;
    char*		arg3ptr = arg3;
    char*		arg4ptr = arg4;
	uint8_t		argcnt = 0;

    availcnt = UART_STM32.available(); 
    if (availcnt>0)
    {    
        for (int c = 0; c < availcnt; c++)
        {           
            *bufptr++ = (char) UART_STM32.read();
            buflen++;
        }
        if (buflen>=sizeof(buffer)) {
            buffer[sizeof(buffer)-1] = '\r';
        }
    } else return;

   
    if(buflen > 4) 
    {
        for (unsigned int c = 0; c < buflen; c++)
        {           
            if(buffer[c] == '\r') 
            {
                buffer[c] = '\0';
               // UART_DBG.print("recv "); UART_DBG.println(buffer);
                found = 1;

                buflen = 0;           // reset counter
                bufptr = buffer;    // reset ptr
            }
        }

    }

    if (found)
    {

        // devide buffer into command and data
		// ****************************************
		cmdptr = buffer;

		for(int x=0;x<6;x++){
			cmdptrend = strchr(cmdptr,' ');
			if (cmdptrend!=NULL) {
				*cmdptrend = '\0';
				if(x==0) 		strncpy(cmd,cmdptr,sizeof(cmd)-1);		// command
				else if(x==1) { strncpy(arg0,cmdptr,sizeof(arg0)-1); argcnt=1;	} 	// 1st argument
				else if(x==2) {	strncpy(arg1,cmdptr,sizeof(arg1)-1); argcnt=2;	} 	// 2nd argument
                else if(x==3) {	strncpy(arg2,cmdptr,sizeof(arg2)-1); argcnt=3;	} 	// 3rd argument
                else if(x==4) {	strncpy(arg3,cmdptr,sizeof(arg3)-1); argcnt=4;	} 	// 4th argument
                else if(x==5) {	strncpy(arg4,cmdptr,sizeof(arg4)-1); argcnt=5;	} 	// 5th argument
				cmdptr = cmdptrend + 1;
			}
		}

        //UART_DBG.print(buffer);
        //UART_DBG.println("");

        // stm32 alive packet
		if(memcmp("STMalive",cmd,8) == 0) {			
			//telnet_msg("help command received");
            //UART_DBG.println("STMalive received");
            stm32alive = COMM_ALIVE_CYCLE + 100;
		}

        // help
		else if(memcmp("help",cmd,4) == 0) {
			//UART_DBG.println("Help:");
			//UART_DBG.println("*********************");
			telnet_msg("help command received");
            //UART_DBG.println("help command received");
			//return CMD_HELP;
		}

		// get actual values
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETACTUALPOS,cmd,5) == 0) {
            telnet_msg("actual position answer");
            UART_DBG.println("actual position answer");
            //app_cmd(APP_CMD_GETACTUAL);
            
            if(argcnt == 2) {
                UART_DBG.println("argcnt 2");
                UART_DBG.print("argcnt "); UART_DBG.println(argcnt);
                UART_DBG.print("arg0 "); UART_DBG.println(atoi(arg0ptr));
                UART_DBG.print("arg1 "); UART_DBG.println(atoi(arg1ptr));
                actuators[atoi(arg0ptr)].actual_position = atoi(arg1ptr);
            }
        }

        // get mean current 
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETMEANCURR,cmd,5) == 0) {
            telnet_msg("mean current answer");
            UART_DBG.println("mean current answer");
            //app_cmd(APP_CMD_GETMEANCURR);
            
            if(argcnt == 2) {
                actuators[atoi(arg0ptr)].meancurrent = atoi(arg1ptr);
            }
        }
    
        // get status
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETSTATUS,cmd,5) == 0) {
            telnet_msg("status answer");
            UART_DBG.println("status answer");
            //app_cmd(APP_CMD_GETSTATUS);
            
            if(argcnt == 2) {
                actuators[atoi(arg0ptr)].state = atoi(arg1ptr);
            }
        }

         // get set target answer
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETTARGETPOS,cmd,5) == 0) {
            if(argcnt == 0) {
                settarget_check = 1;
            }
        }

        // get data values
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETVLVDATA,cmd,5) == 0) {
            //telnet_msg("got data packet");
            //UART_DBG.println("got data packet");
            //app_cmd(APP_CMD_GETACTUAL);
            
            if(argcnt == 5) {
                //UART_DBG.println("argcnt 2");
                //UART_DBG.print("argcnt "); UART_DBG.println(argcnt);
                //UART_DBG.print("arg0 "); UART_DBG.println(atoi(arg0ptr));
                //UART_DBG.print("arg1 "); UART_DBG.println(atoi(arg1ptr));
                actuators[atoi(arg0ptr)].actual_position = atoi(arg1ptr);
                actuators[atoi(arg0ptr)].meancurrent = atoi(arg2ptr);
                actuators[atoi(arg0ptr)].state = atoi(arg3ptr);
                actuators[atoi(arg0ptr)].temperature = atoi(arg4ptr);

                if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_DETAIL) {
                   // logger.print("new data package: ", logger.DATA); 
                   // logger.println(buffer, logger.DATA); 
                    syslog.log(LOG_DEBUG, "got full vlv data packet");
                }
                // create some debug messages
                if (vismode > VISMODE_DETAIL) {
                    UART_DBG.println("got full vlv data packet");
                    //
                    logger.print("got full vlv data packet");
                }
            }

            
        }

        // get onewire data answer
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETONEWIREDATA,cmd,5) == 0) {
            logger.print("got one wire data packet");
            if(argcnt == 1) {
                logger.print(arg0ptr);
            }
            
        }


    
    }

}

void app_comm_machine(){
    #define COMM_IDLE           0
    #define COMM_ALIVE          1
    #define COMM_SENDTARGET     2
    #define COMM_CHECKTARGET    3
    #define COMM_GETDATA        4

    static unsigned char commstate=0;
    char sendbuffer[30];
    char valbuffer[10];

    static unsigned int cnt_alive = COMM_ALIVE_CYCLE;
    static unsigned int getindex = 0;
        
    static unsigned int timeout = 0;
    static unsigned int retry = 0;
   
    switch(commstate) {

        case COMM_IDLE: 
                commstate = COMM_ALIVE;
                break;

        case COMM_ALIVE: 
                if (cnt_alive) {
                    cnt_alive = COMM_ALIVE_CYCLE;
                    UART_STM32.println("ESPalive");
                    if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_DETAIL) {
                        syslog.log(LOG_INFO, "ESPalive");
                    }
                }
                else cnt_alive--;

                retry = 5;
                commstate = COMM_SENDTARGET;

                break;

        // send target position values
        case COMM_SENDTARGET:
                commstate = COMM_GETDATA;
                if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG, "checking valve positions");
                }
                for(unsigned int x=0; x<ACTUATOR_COUNT; x++)
                {
                    // check if target has changed
                    if(target_position_mirror[x] != actuators[x].target_position)
                    {
                        sendbuffer[0] = '\0';
                        //UART_DBG.println("sending new target value");
                        strcat(sendbuffer, APP_PRE_SETTARGETPOS);
                        strcat(sendbuffer, " ");

                        itoa(x, valbuffer, 10);
                        strcat(sendbuffer, valbuffer);
                        strcat(sendbuffer, " ");   

                        itoa(actuators[x].target_position, valbuffer, 10);      
                        strcat(sendbuffer, valbuffer);
                        strcat(sendbuffer, " ");

                        UART_STM32.println(sendbuffer);
                        
                        // generate debug messages
                        if (vismode == VISMODE_DETAIL) {
                            UART_DBG.println(sendbuffer);
                            if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_DETAIL) {
                                syslog.log(LOG_DEBUG, sendbuffer);
                            }
                            logger.print(sendbuffer, logger.DATA);
                        }

                        // break loop, next valve will be served in next cycle
                        x = ACTUATOR_COUNT;

                        timeout = 10;
                        commstate = COMM_CHECKTARGET;
                    }

                    
                }

                // update target position mirror - 
                for(unsigned int x = 0; x<ACTUATOR_COUNT; x++)
                {
                    target_position_mirror[x] = actuators[x].target_position;
                }
                if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG, "updated position mirror");
                }
                break;

        // check correct transmission of target value
        case COMM_CHECKTARGET:

                if(timeout>0) timeout--;
                else {
                    // answer received
                    if(settarget_check) {                        
                        commstate = COMM_GETDATA; 
                    }
                    else {
                        if(retry>0) retry--;
                        else commstate = COMM_GETDATA; 
                        commstate = COMM_SENDTARGET;
                    }
                }
                
                break;

        case COMM_GETDATA:  
                commstate = COMM_IDLE;
                
                // walk through all valves, one per cycle
                if(getindex<ACTUATOR_COUNT-1) getindex++;
                //if(getindex<1) getindex++;
                else getindex=0;

                sendbuffer[0] = '\0';
                
                strcat(sendbuffer, APP_PRE_GETVLVDATA);
                strcat(sendbuffer, " ");

                itoa(getindex, valbuffer, 10);
                strcat(sendbuffer, valbuffer);
                strcat(sendbuffer, " ");   

                UART_STM32.println(sendbuffer);
                
                if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_DETAIL) {
                    syslog.log(LOG_DEBUG, sendbuffer);
                }
                // generate debug messages
                if (vismode == VISMODE_DETAIL) {
                    UART_DBG.println(sendbuffer);                
                    logger.print(sendbuffer, logger.DATA);
                }
                        
                break;

        default:    
                commstate = COMM_IDLE;
                break;
    }
}

void app_alive_check() {
    static uint8_t oldalivestate;

    if(stm32alive) {
        stm32alive--;
        if (oldalivestate == 0) {
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
                syslog.log(LOG_DEBUG, "connection to STM32 established");
            }
            // generate debug messages
            if (vismode > VISMODE_OFF) {
                UART_DBG.println("connection to STM32 established");
                logger.print("connection to STM32 established");
            }
        }
        oldalivestate = 1;
    }
    else {
        // generate debug messages
        if (oldalivestate == 1) {
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
                syslog.log(LOG_DEBUG, "connection to STM32 lost");
            }
            if (vismode > VISMODE_OFF) {
                UART_DBG.println("connection to STM32 lost");
                logger.print("connection to STM32 lost");
            }
        }
        oldalivestate = 0;
    }
}


void app_web_cmd_check(){
    
    if(cmd_buffer.length() >= 5) {
        if(cmd_buffer.startsWith(APP_PRE_SETALLVLVOPEN)) {	
            UART_DBG.println("send open all valves request to STM32");
            if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_ATOMIC) {
                syslog.log(LOG_DEBUG, "send open all valves request to STM32");
            }		
            UART_STM32.println(APP_PRE_SETALLVLVOPEN);	 
        }

        else if(cmd_buffer.startsWith(APP_PRE_SETALLVLVOPEN)) {	
            if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_ATOMIC) {
                syslog.log(LOG_DEBUG, "send target position request to STM32");
            }
            UART_DBG.println("send target position request to STM32");		
            UART_STM32.println(cmd_buffer);	 
        }

        else if(cmd_buffer.startsWith(APP_PRE_GETONEWIREDATA)) {
            if (VdmConfig.configFlash.netConfig.syslogLevel==VISMODE_ATOMIC) {
                syslog.log(LOG_DEBUG, "send onewire data request to STM32");
            }	
            UART_DBG.println("send onewire data request to STM32");		
            UART_STM32.println(cmd_buffer);
        }

        else {
            if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ON) {
                syslog.log(LOG_DEBUG, "unknown command for STM32 from webinterface");
            }
            UART_DBG.println("unknown command for STM32 from webinterface");
            logger.print("unknown command for STM32");
        }   
    }  

    cmd_buffer = "";    
}
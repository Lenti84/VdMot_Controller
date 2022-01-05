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

#include <Arduino.h>
#include "hardware.h"
#include "app.h"
#include "motor.h"
#include "communication.h"
#include "temperature.h"

#define COMM_SER				Serial1		// serial port to ESP32
//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging

#define COMM_MAX_CMD_LEN		15			// max length of a command without arguments
#define COMM_ARG_CNT			 2			// number of allowed command arguments
#define COMM_MAX_ARG_LEN		20			// max length of one command argument
#define SEND_BUFFER_LEN			100


void communication_setup (void) {
	
	// UART to ESP32
	COMM_SER.setRx(PA10);			//STM32F401 blackpill USART1 RX PA10
	COMM_SER.setTx(PA9);			//STM32F401 blackpill USART1 TX PA9
	COMM_SER.begin(115200);
	while(!COMM_SER);
	//COMM_SER.println("alive");COMM_SER.flush();
}


/**
  * @brief  Handler
  * @param  None
  * @retval None
  */
int16_t communication_loop (void) {
	static char buffer[300];
    static char *bufptr = buffer;
    static unsigned int buflen = 0;
    int availcnt;
    unsigned int found = 0;

	char*       cmdptr;
    char*	    cmdptrend;
    char        cmd[10];
    char		arg0[20];	
    char        arg1[20];		
	char*		arg0ptr = arg0;
	char*		arg1ptr = arg1;
	uint8_t		argcnt = 0;
    
	uint16_t		x;
	uint32_t		xu32;
	uint16_t		y;
	char sendbuffer[30];
    char valbuffer[10];

	char sendbuf[SEND_BUFFER_LEN];

	availcnt = COMM_SER.available(); 
    if(availcnt>0)
    {    
        for (int c = 0; c < availcnt; c++)
        {           
            *bufptr++ = (char) COMM_SER.read();
            buflen++;
        }
        if (buflen>=sizeof(buffer)) {
            buffer[sizeof(buffer)-1] = '\n';
        }
    }

	// if there is a little in the buffer
    if(buflen >= 5) 
    {
        for (unsigned int c = 0; c < buflen; c++)
        {           
            if(buffer[c] == '\n') 
            {
				if(buffer[c-1]=='\r') buffer[c-1] = '\0';
                else buffer[c] = '\0';
                //COMM_SER.print("recv "); COMM_SER.println(buffer);
                found = 1;

                buflen = 0;           	// reset counter
                bufptr = buffer;    	// reset ptr
            }
        }
    }

	// was something received
	if(found) {

		// devide buffer into command and data
		// ****************************************
		cmdptr = buffer;

		for(unsigned int xx=0;xx<3;xx++){
			cmdptrend = strchr(cmdptr,' ');
			if (cmdptrend!=NULL) {
				*cmdptrend = '\0';
				if(xx==0) 		strncpy(cmd,cmdptr,sizeof(cmd)-1);		// command
				else if(xx==1) { strncpy(arg0,cmdptr,sizeof(arg0)-1); argcnt=1;	} 	// 1st argument
				else if(xx==2) {	strncpy(arg1,cmdptr,sizeof(arg1)-1); argcnt=2;	} 	// 2nd argument
				cmdptr = cmdptrend + 1;
			}
		}

		// evaluate data
		// ****************************************************************************************
		// clear sendbuffer, otherwise there is something from older commands
		memset (sendbuffer,0x0,sizeof(sendbuffer));
		
		// set target position
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		if(memcmp(APP_PRE_SETTARGETPOS,&cmd[0],5) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);

			if(argcnt == 2) {				
				COMM_DBG.println("comm: set target position");
				if (y >= 0 && y <= 100 && x < ACTUATOR_COUNT) 
				{
					myvalvemots[x].target_position = (byte) y;
					COMM_SER.println(APP_PRE_SETTARGETPOS);
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// get target position
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		if(memcmp(APP_PRE_GETTARGETPOS,&cmd[0],5) == 0) {
			x = atoi(arg0ptr);

			if(argcnt == 1) {				
				COMM_DBG.println("comm: got target position request");
				if (x >= 0 && x < ACTUATOR_COUNT) 
				{	
					COMM_SER.print(APP_PRE_GETTARGETPOS);
					COMM_SER.print(" ");
					COMM_SER.print(myvalvemots[x].target_position, DEC);
					COMM_SER.println(" ");
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// get valve data (actual position, target position, meancurrent, status, temperature)
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETVLVDATA,&cmd[0],5) == 0) {
			x = atoi(arg0ptr);

			if(argcnt == 1) {				
				//COMM_DBG.println("comm: get valve data");
				//if(0) 
				if (x < ACTUATOR_COUNT) 
				{
					sendbuffer[0] = '\0';
					//COMM_SER.println("sending new target value");
                    
					strcat(sendbuffer, APP_PRE_GETVLVDATA);
					strcat(sendbuffer, " ");
                    
					itoa(x, valbuffer, 10);		// valve nr
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");       
                    
					itoa(myvalvemots[x].actual_position, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");

					// itoa(myvalvemots[x].target_position, valbuffer, 10);      
					// strcat(sendbuffer, valbuffer);
					// strcat(sendbuffer, " ");
                    
					itoa(myvalvemots[x].meancurrent, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");
                    
					itoa(myvalvemots[x].status, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");
                    
					if(myvalves[x].sensorindex1<MAXSENSORCOUNT)
					{
						itoa(tempsensors[myvalves[x].sensorindex1].temperature, valbuffer, 10);      
						strcat(sendbuffer, valbuffer);
					}
					else strcat(sendbuffer, "-500");
					
					strcat(sendbuffer, " ");

					if(myvalves[x].sensorindex2<MAXSENSORCOUNT)
					{
						itoa(tempsensors[myvalves[x].sensorindex2].temperature, valbuffer, 10);      
						strcat(sendbuffer, valbuffer);
					}
					else strcat(sendbuffer, "-500");

					// end
					strcat(sendbuffer, " ");
                    
					COMM_SER.println(sendbuffer);
					//COMM_DBG.println(sendbuffer);		// debug				
				}
			}
			else COMM_DBG.println("to few arguments");

			//return CMD_LEARN;
		}


		// get onewire sensor count
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETONEWIRECNT,&cmd[0],5) == 0) {
			COMM_DBG.println("cmd: get onewire sensor count");
			COMM_SER.print(APP_PRE_GETONEWIRECNT);
			COMM_SER.print(" ");			
			COMM_SER.print(numberOfDevices, DEC);
			COMM_SER.println(" ");			
		}


		// get onewire sensor data of sensor x (adress and temperature)
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETONEWIREDATA,&cmd[0],5) == 0) {
			COMM_DBG.println("cmd: get onewire sensor data");

			x = atoi(arg0ptr);

			if(argcnt == 1) {

				if(x<MAXSENSORCOUNT)
				{
					// 8 Byte adress
					for (uint8_t i = 0; i < 8; i++)
					{
						if (tempsensors[x].address[i] < 16) strcat(sendbuffer, "0");
						itoa(tempsensors[x].address[i], valbuffer, 16);      
						strcat(sendbuffer, valbuffer);
						if (i<7) strcat(sendbuffer, "-");
					}
					strcat(sendbuffer," ");
					// temperature
					itoa(tempsensors[x].temperature, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
				}
				else strcat(sendbuffer, "0");

			}
			else strcat(sendbuffer, "0");

			COMM_SER.print(APP_PRE_GETONEWIREDATA);
			COMM_SER.print(" ");			
			COMM_SER.print(sendbuffer);
			COMM_SER.println(" ");			
		}


		// start new onewire sensor search
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETONEWIRESEARCH,&cmd[0],5) == 0) {
			COMM_DBG.println("start new 1-wire search");
			temp_command(TEMP_CMD_NEWSEARCH);
		}
		

		// set valve learning time
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETLEARNTIME,&cmd[0],5) == 0) {
			COMM_DBG.print("set valve learning time to ");
			xu32 = atol(arg0ptr);

			if(argcnt == 1 && xu32 >= 0) {
				if( app_set_learntime(xu32) == 0) COMM_DBG.println(xu32, DEC);
				else COMM_DBG.println("- error");
			}
			else {
				COMM_DBG.println("- error");
			}
		}


		// set valve learning movements
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETLEARNMOVEM,&cmd[0],5) == 0) {
			COMM_DBG.print("set valve learning movements to ");
			x = atoi(arg0ptr);

			if(argcnt == 1 && x >= 0) {
				if( app_set_learnmovements(x) == 0) COMM_DBG.println(x, DEC);
				else COMM_DBG.println("- error");
			}
			else {
				COMM_DBG.println("- error");
			}
		}


		// ESPalive
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("ESP",&cmd[0],3) == 0) {	
			COMM_DBG.println("received ESPalive 22");		
			if (buflen >= 8 && memcmp("ESPalive",cmd,8) == 0) {
				COMM_DBG.println("received ESPalive");
			}			 
		}


		// set first sensor index of valve
		// x - valve index
		// y - temp sensor index
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SET1STSENSORINDEX,&cmd[0],5) == 0) {
			x = atoi(arg0ptr); // valve index
			y = atoi(arg1ptr); // temp sensor index

			if(argcnt == 2) {				
				COMM_DBG.println("comm: set 1st sensor index");
				if (x >= 0 && x < ACTUATOR_COUNT && y < MAXSENSORCOUNT) 
				{
					myvalves[x].sensorindex1 = (byte) y;
					//COMM_SER.println(APP_PRE_SET1STSENSORINDEX);
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// set second sensor index of valve
		// x - valve index
		// y - temp sensor index
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SET2NDSENSORINDEX,&cmd[0],5) == 0) {
			x = atoi(arg0ptr); // valve index
			y = atoi(arg1ptr); // temp sensor index

			if(argcnt == 2) {				
				COMM_DBG.println("comm: set 2nd sensor index");
				if (x >= 0 && x < ACTUATOR_COUNT && y < MAXSENSORCOUNT) 
				{
					myvalves[x].sensorindex2 = (byte) y;
					//COMM_SER.println(APP_PRE_SET2NDSENSORINDEX);
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// open all valves request
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETALLVLVOPEN,&cmd[0],5) == 0) {
			COMM_DBG.print("got open valve request for ");

			x = atoi(arg0ptr);

			if(argcnt == 1 && x >= 0) {
				if( app_set_valveopen(x) == 0) COMM_DBG.println(x, DEC);
				else COMM_DBG.println("- error");
			}
			else {
				COMM_DBG.println("- error");
			}
		}


		// learn valve x request
		// if x is 255 all valves will be learned
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETVLLEARN,&cmd[0],5) == 0) {
			COMM_DBG.print("start learning for valve ");
			
			x = atoi(arg0ptr);

			if(argcnt == 1 && x >= 0) {
				if( app_set_valvelearning(x) == 0) COMM_DBG.println(x, DEC);
				else COMM_DBG.println("- error");
			}
			else {
				COMM_DBG.println("- error");
			}
		}


		// get version request
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETVERSION,&cmd[0],5) == 0) {
			COMM_DBG.println("got version request");

			COMM_SER.print(APP_PRE_GETVERSION);
			COMM_SER.print(" ");			
			COMM_SER.print(FIRMWARE_VERSION);
			COMM_SER.println(" ");	
		}


		// unknown command
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else {
			//COMM_DBG.println("unknown command received from ESP");
		}

	}
	else return -1;


return 0;
}



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
#include "terminal.h"
#include "motor.h"
#include "temperature.h"
#include "communication.h"
#include "eeprom.h"


//extern HardwareSerial Serial3;
extern void callback_app(char cmd, byte valveindex, byte pos);

void WriteEEPROMBaselayout(void);

//HardwareSerial Serial3(USART3);
HardwareSerial Serial6(USART6);


//#define COMM_DBG				Serial3		// serial port for debugging
//#define COMM_DBG				Serial6		// serial port for debugging


int testmode = 0;							// flag for testmode

//#define COMM_DBG				Serial3		// serial port for debugging
//#define COMM_DBG				Serial6		// serial port for debugging

#define TERM_MAX_CMD_LEN		15			// max length of a command without arguments
#define TERM_ARG_CNT			 2			// number of allowed command arguments
#define TERM_MAX_ARG_LEN		20			// max length of one command argument
#define SEND_BUFFER_LEN			800

int16_t Terminal_Init (void) {

	Serial6.setRx(PA12);			//STM32F401 blackpill USART6 RX PA12
	Serial6.setTx(PA11);			//STM32F401 blackpill USART6 TX PA11
	
	COMM_DBG.begin(115200);
	while(!COMM_DBG);
	COMM_DBG.print("VdMot Controller "); 
	COMM_DBG.print(FIRMWARE_VERSION);

	#ifdef HARDWARE_REVISION_C1
		COMM_DBG.println("_C1");
	#elif HARDWARE_REVISION_C2
		COMM_DBG.println("_C2");
	#else
		error "no hardware revision defined"
	#endif

	COMM_DBG.flush();

	testmode = 0;

	return 0;
}


/**
  * @brief  Handler
  * @param  None
  * @retval None
  */
int16_t Terminal_Serve (void) {
	static char buffer[300];
    static char *bufptr = buffer;
    static unsigned int buflen = 0;
    int availcnt;
    unsigned int found = 0;

	char*       cmdptr;
    char*	    cmdptrend;
    char        cmd[10];
    char		arg0[30];	
    char        arg1[30];		
	char        arg2[30];
	char*		arg0ptr = arg0;
	char*		arg1ptr = arg1;
	char*		arg2ptr = arg2;
	uint8_t		argcnt = 0;

	//int			itempval;
	uint16_t		x;
	uint32_t		xu32;
	uint16_t		y;

	char sendbuf[SEND_BUFFER_LEN];
    char valbuffer[10];

	DeviceAddress currAddress;
	uint8_t numberOfDevices;

	availcnt = COMM_DBG.available(); 
    if(availcnt>0)
    {    
        for (int c = 0; c < availcnt; c++)
        {           
            *bufptr++ = (char) COMM_DBG.read();
            buflen++;
        }
        if (buflen>=sizeof(buffer)) {
            buffer[sizeof(buffer)-1] = '\r';
        }
    }

	// if there is a little in the buffer
    if(buflen > 5) 
    {
        for (unsigned int c = 0; c < buflen; c++)
        {           
            if(buffer[c] == '\r') 
            {
                buffer[c] = '\0';
                //COMM_DBG.print("recv "); COMM_DBG.println(buffer);
                found = 1;

                buflen = 0;           	// reset counter
                bufptr = buffer;    	// reset ptr
            }
        }
    }

	// was something received
	if(found) {

		// devide request into command and data
		// ****************************************
		cmdptr = buffer;

		for(unsigned int x=0;x<4;x++){
			cmdptrend = strchr(cmdptr,' ');
			if (cmdptrend!=NULL) {
				*cmdptrend = '\0';
				if(x==0) 		strncpy(cmd,cmdptr,sizeof(cmd)-1);		// command
				else if(x==1) { strncpy(arg0,cmdptr,sizeof(arg0)-1); argcnt=1;	} 	// 1st argument
				else if(x==2) {	strncpy(arg1,cmdptr,sizeof(arg1)-1); argcnt=2;	} 	// 2nd argument
				else if(x==3) {	strncpy(arg2,cmdptr,sizeof(arg2)-1); argcnt=3;	} 	// 3rd argument
				cmdptr = cmdptrend + 1;
			}
		}

		if(argcnt==1) {
			x = atoi(arg0ptr);
		}
		else if (argcnt==2) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);
		}


		// evsluate commands
		// ****************************************************************************************
		sendbuf[0] = '\0';			// reset sendbuf

		// help
		if(memcmp("help",&cmd[0],4) == 0) {
			COMM_DBG.println("Help:");
			COMM_DBG.println("*********************");
			
			return CMD_HELP;
		}

		// learn
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("learn",&cmd[0],5) == 0) {
			//x = atoi(arg0ptr);
			//y = atoi(arg0ptr);

			if(argcnt == 1) {				
				//COMM_DBG.println("learn valve x");				
				if (appsetaction(CMD_A_LEARN,x,0)!=0) COMM_DBG.println("valve machine command not accepted");				
			}
			else COMM_DBG.println("to few arguments");

			return CMD_LEARN;
		}

		// open to value
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("open",&cmd[0],5) == 0) {
			//x = atoi(arg0ptr);
			//y = atoi(arg1ptr);

			if(argcnt == 2) {								
				if (appsetaction(CMD_A_OPEN,x,y)!=0) COMM_DBG.println("valve machine not idle");
			}
			else COMM_DBG.println("to few arguments");

			return CMD_OPEN;
		}

		// close to value
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("close",&cmd[0],5) == 0) {
			//x = atoi(arg0ptr);
			//y = atoi(arg1ptr);

			if(argcnt == 2) {								
				if (appsetaction(CMD_A_CLOSE,x,y)!=0) COMM_DBG.println("valve machine not idle");
			}
			else COMM_DBG.println("to few arguments");

			return CMD_CLOSE;
		}

		// set target value
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("settar",&cmd[0],6) == 0) {
			//x = atoi(arg0ptr);
			//y = atoi(arg1ptr);

			if(argcnt == 2)
			{								
				//if (appsetaction(CMD_A_CLOSE,x,y)!=0) COMM_DBG.println("valve machine not idle");
				if(x < ACTUATOR_COUNT && y<=100 && y>=0)
				{
					myvalvemots[x].target_position = y;
					COMM_DBG.print("set valve "); COMM_DBG.print(x, 10); COMM_DBG.print(" to "); COMM_DBG.println(y);
				}
			}
			else COMM_DBG.println("to few arguments");

			return CMD_CLOSE;
		}


		// set muxer
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("smux",&cmd[0],4) == 0) {
			//x = atoi(arg0ptr);
			if(argcnt == 1) {
				if(x==0) MUX_OFF();
				else MUX_ON();
			}
		}

		// set direction
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("sdir",&cmd[0],4) == 0) {
			//x = atoi(arg0ptr);
			if(argcnt == 1) {
				if(x==0) DIR_OFF();
				else DIR_ON();
			}
		}

		// set enable
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("sena",&cmd[0],4) == 0) {
			//x = atoi(arg0ptr);
			//y = atoi(arg1ptr);
			if(argcnt == 2) {
				if(x==0) {
					if(y==0) ENA0_OFF();
					else ENA0_ON();
				}
				else if (x==1) {
					if(y==0) ENA1_OFF();
					else ENA1_ON();
				}
				else if (x==2) {
					if(y==0) ENA2_OFF();
					else ENA2_ON();
				}
				else if (x==3) {
					if(y==0) ENA3_OFF();
					else ENA3_ON();
				}
				else if (x==4) {
					if(y==0) ENA4_OFF();
					else ENA4_ON();
				}
				else if (x==5) {
					if(y==0) ENA5_OFF();
					else ENA5_ON();
				}
			}
		}

		// set test mode
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("stm",&cmd[0],3) == 0) {
			//x = atoi(arg0ptr);
			if(argcnt == 1) {
				if(x==0) { testmode = 0; COMM_DBG.println("testmode off"); }
				else {testmode = 1; COMM_DBG.println("testmode on"); }
			}
		}

		// get onewire sensor data - sensor count and data and adress of all connected sensors
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("getone",&cmd[0],6) == 0) {
			get_sensordata(0, sendbuf, SEND_BUFFER_LEN);
			COMM_DBG.println(sendbuf);
		}

		// set eeprom layout
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("seteep",&cmd[0],6) == 0) {
			WriteEEPROMBaselayout();
			COMM_DBG.println("set eeprom layout");
		}

		// save eeprom layout
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("saveep",&cmd[0],6) == 0) {
			//eeprom_write_layout(&eep_content);
			eeprom_changed();
			COMM_DBG.println("saved eeprom layout");
		}

		// set sensor index first sensor
		// x - valve index
		// y - temp sensor index
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SET1STSENSORINDEX,&cmd[0],5) == 0) {

			if(argcnt == 2) {				
			COMM_DBG.println("comm: set 1st sensor index");
				if (x >= 0 && x < ACTUATOR_COUNT && y < MAXONEWIRECNT) 
				{
					numberOfDevices = sensors.getDeviceCount();
					if (numberOfDevices > 0) 
					{
						// write sensor address code to eeprom layout mirror
						sensors.getAddress(currAddress, y);
						eep_content.owsensors1[x].familycode = currAddress[0];
						eep_content.owsensors1[x].romcode[0] = currAddress[1];
						eep_content.owsensors1[x].romcode[1] = currAddress[2];
						eep_content.owsensors1[x].romcode[2] = currAddress[3];
						eep_content.owsensors1[x].romcode[3] = currAddress[4];
						eep_content.owsensors1[x].romcode[4] = currAddress[5];
						eep_content.owsensors1[x].romcode[5] = currAddress[6];
						eep_content.owsensors1[x].crc = currAddress[7];

						// write index to valves structure
						myvalves[x].sensorindex1 = (byte) y;

						eeprom_changed();	
					}					
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// set sensor index second sensor
		// x - valve index
		// y - temp sensor index
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SET2NDSENSORINDEX,&cmd[0],5) == 0) {

			if(argcnt == 2) {				
				COMM_DBG.println("comm: set 2nd sensor index");
				if (x >= 0 && x < ACTUATOR_COUNT && y < MAXONEWIRECNT) 
				{
					numberOfDevices = sensors.getDeviceCount();
					if (numberOfDevices > 0) 
					{
						// write sensor address code to eeprom layout mirror
						sensors.getAddress(currAddress, y);
						eep_content.owsensors2[x].familycode = currAddress[0];
						eep_content.owsensors2[x].romcode[0] = currAddress[1];
						eep_content.owsensors2[x].romcode[1] = currAddress[2];
						eep_content.owsensors2[x].romcode[2] = currAddress[3];
						eep_content.owsensors2[x].romcode[3] = currAddress[4];
						eep_content.owsensors2[x].romcode[4] = currAddress[5];
						eep_content.owsensors2[x].romcode[5] = currAddress[6];
						eep_content.owsensors2[x].crc = currAddress[7];

						// write index to valves structure
						myvalves[x].sensorindex2 = (byte) y;	

						eeprom_changed();					
					}
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// set valve sensors
		// arg0 - valve index
		// arg1 - 8 byte hex address of 1st 1-wire sensor
		// arg2 - 8 byte hex address of 2nd 1-wire sensor
		// if hex address == 00-00-00-00-00-00-00-00 this sensor will be ignored
		// example: 'stvls 1 00-00-00-00-00-00-00-00 00-00-00-00-00-00-00-00 '
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		#warning handle 00-00-00... sensors explicitly
		else if(memcmp(APP_PRE_SETVLVSENSOR,&cmd[0],5) == 0) {
			x = atoi(arg0ptr); // valve index
			
			if(argcnt == 3) {				
				COMM_DBG.println("comm: set valve sensors");
				if (x >= 0 && x < ACTUATOR_COUNT) 
				{
					// first sensor address
					if(strlen(arg1ptr)==23) {

						// from end to beginning
						for(unsigned int xx=0;xx<=7;xx++) {
							cmdptr=strrchr(arg1ptr,'-');
							currAddress[7-xx] = (uint8_t) strtoul(cmdptr+1,NULL,16);
							COMM_DBG.print(currAddress[7-xx],HEX);
							COMM_DBG.print('-');
							*cmdptr = '\0';
						}
						currAddress[0] = (uint8_t) strtoul(arg1ptr,NULL,16);

						// COMM_DBG.println("add1 ");
						// printAddress(currAddress);
						// COMM_DBG.println("");
						
						if (sensors.validAddress(currAddress)) {
							eep_content.owsensors1[x].familycode = currAddress[0];
							eep_content.owsensors1[x].romcode[0] = currAddress[1];
							eep_content.owsensors1[x].romcode[1] = currAddress[2];
							eep_content.owsensors1[x].romcode[2] = currAddress[3];
							eep_content.owsensors1[x].romcode[3] = currAddress[4];
							eep_content.owsensors1[x].romcode[4] = currAddress[5];
							eep_content.owsensors1[x].romcode[5] = currAddress[6];
							eep_content.owsensors1[x].crc = currAddress[7];
						
							eeprom_changed();	
							
  							// match sensor address to valve struct at runtime, otherwise restart needed
  							app_match_sensors();
						}
					}

					// second sensor address
					if(strlen(arg2ptr)==23) {

						// from end to beginning
						for(unsigned int xx=0;xx<=7;xx++) {
							cmdptr=strrchr(arg2ptr,'-');
							currAddress[7-xx] = (uint8_t) strtoul(cmdptr+1,NULL,16);
							*cmdptr = '\0';
						}
						currAddress[0] = (uint8_t) strtoul(arg2ptr,NULL,16);

						// COMM_DBG.println("add2 ");
						// printAddress(currAddress);
						// COMM_DBG.println("");
						
						if (sensors.validAddress(currAddress)) {
							eep_content.owsensors2[x].familycode = currAddress[0];
							eep_content.owsensors2[x].romcode[0] = currAddress[1];
							eep_content.owsensors2[x].romcode[1] = currAddress[2];
							eep_content.owsensors2[x].romcode[2] = currAddress[3];
							eep_content.owsensors2[x].romcode[3] = currAddress[4];
							eep_content.owsensors2[x].romcode[4] = currAddress[5];
							eep_content.owsensors2[x].romcode[5] = currAddress[6];
							eep_content.owsensors2[x].crc = currAddress[7];
							
							eeprom_changed();	
							
  							// match sensor address to valve struct at runtime, otherwise restart needed
  							app_match_sensors();
						}
					}			
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// get 1st and 2nd onewire sensor address for valve x
		// example answer: gvlon 1 28-84-37-94-97-FF-03-23 00-00-00-00-00-00-00-00
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETONEWIRESETT,&cmd[0],5) == 0) {
			COMM_DBG.print("cmd: get 1st and 2nd onewire sensor addresses");

			x = atoi(arg0ptr);

			if(argcnt == 1) {

				if(x<ACTUATOR_COUNT)
				{
					// answer begin
					COMM_DBG.print(" - ");
					COMM_DBG.print(APP_PRE_GETONEWIRESETT);
					COMM_DBG.print(" ");

					// valve index
					COMM_DBG.print(x, DEC);
					COMM_DBG.print(" ");
					
					// 1st sensor 8 Byte adress
					y = myvalves[x].sensorindex1;					
					if(y!=VALVE_SENSOR_UNKNOWN) {
						sendbuf[0] = '\0';			// reset sendbuffer
						for (uint8_t i = 0; i < 8; i++)
						{
							if (tempsensors[y].address[i] < 16) strcat(sendbuf, "0");
							itoa(tempsensors[y].address[i], valbuffer, 16);      
							strcat(sendbuf, valbuffer);
							if (i<7) strcat(sendbuf, "-");
						}
						COMM_DBG.print(sendbuf);
					}
					else {
						COMM_DBG.print("00-00-00-00-00-00-00-00");
					}
					COMM_DBG.print(" ");

					// 2nd sensor 8 Byte adress
					y = myvalves[x].sensorindex2;
					if(y!=VALVE_SENSOR_UNKNOWN) {
						sendbuf[0] = '\0';			// reset sendbuffer
						for (uint8_t i = 0; i < 8; i++)
						{
							if (tempsensors[y].address[i] < 16) strcat(sendbuf, "0");
							itoa(tempsensors[y].address[i], valbuffer, 16);      
							strcat(sendbuf, valbuffer);
							if (i<7) strcat(sendbuf, "-");
						}
						COMM_DBG.print(sendbuf);
					}
					else {
						COMM_DBG.print("00-00-00-00-00-00-00-00");
					}

					// finish answer
					COMM_DBG.println(" ");

				}
				else {
					COMM_DBG.println(" - error");
				}
			}
			else {
				COMM_DBG.println(" - error");		
			}			
		}


		// get version request
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("gvers",&cmd[0],5) == 0) {			
			COMM_DBG.print("Version: ");			
			COMM_DBG.println(FIRMWARE_VERSION);
		}


		// start new onewire sensor search
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETONEWIRESEARCH,&cmd[0],5) == 0) {
			COMM_DBG.print("start new 1-wire search");
			temp_command(TEMP_CMD_NEWSEARCH);
		}


		// set valve learning time
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETLEARNTIME,&cmd[0],5) == 0) {
			COMM_DBG.print("set valve learning time to ");
			x = atoi(arg0ptr);

			if(argcnt == 1 && x > 0) {
				if( app_set_learntime(x) == 0) COMM_DBG.println(x, DEC);
				else COMM_DBG.println("- error");
			}
			else {
				COMM_DBG.println("- error");
			}
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


		// set motor characteristics
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETMOTCHARS,&cmd[0],5) == 0) {
			COMM_DBG.print("got set motor characteristics request ");

			if(argcnt == 2) {
				x = atoi(arg0ptr);
				y = atoi(arg1ptr);

				if (x>=5 && x<=50 && y>=5 && y<=50) {
					currentbound_low_fac = atoi(arg0ptr);
					currentbound_high_fac = atoi(arg1ptr);

					eep_content.currentbound_low_fac = currentbound_low_fac;
					eep_content.currentbound_high_fac = currentbound_high_fac;
					eeprom_changed();
					COMM_DBG.println("- valid");
				}
				else COMM_DBG.println("- values out of bounds");
			}
			else {
				COMM_DBG.println("- error");
			}
		}


		// get motor characteristics
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETMOTCHARS,&cmd[0],5) == 0) {
			COMM_DBG.print("got get motor characteristics request - low: ");			
			COMM_DBG.print(currentbound_low_fac, DEC);
			COMM_DBG.println(" high: ");		
			COMM_DBG.println(currentbound_high_fac, DEC);			
		} 


		// set valve sensors
		// x - valve index
		// arg1 - 8 byte hex address of 1st 1-wire sensor
		// arg2 - 8 byte hex address of 2nd 1-wire sensor
		// if hex address == 00-00-00-00-00-00-00-00 this sensor will be ignored
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETVLVSENSOR,&cmd[0],5) == 0) {
			COMM_DBG.println("set valve sensors by address");

			x = atoi(arg0ptr); // valve index
			
			if(argcnt == 1) {				
				COMM_DBG.println("comm: set valve sensors");
				if (x >= 0 && x < ACTUATOR_COUNT) 
				{
					// first sensor address
					if(strlen(arg1ptr)==23) {

						// from end to beginning
						for(unsigned int xx=0;xx<=7;xx++) {
							cmdptr=strrchr(arg1ptr,'-');
							currAddress[7-xx] = atoi(cmdptr+1);
							*cmdptr = '\0';
						}
						
						if (sensors.validAddress(currAddress)) {
							eep_content.owsensors1[x].familycode = currAddress[0];
							eep_content.owsensors1[x].romcode[0] = currAddress[1];
							eep_content.owsensors1[x].romcode[1] = currAddress[2];
							eep_content.owsensors1[x].romcode[2] = currAddress[3];
							eep_content.owsensors1[x].romcode[3] = currAddress[4];
							eep_content.owsensors1[x].romcode[4] = currAddress[5];
							eep_content.owsensors1[x].romcode[5] = currAddress[6];
							eep_content.owsensors1[x].crc = currAddress[7];
						
							eeprom_changed();	
							
  							// match sensor address to valve struct at runtime, otherwise restart needed
  							app_match_sensors();
						}
					}

					// second sensor address
					if(strlen(arg2ptr)==23) {

						// from end to beginning
						for(unsigned int xx=0;xx<=7;xx++) {
							cmdptr=strrchr(arg2ptr,'-');
							currAddress[7-xx] = atoi(cmdptr+1);
							*cmdptr = '\0';
						}
						
						if (sensors.validAddress(currAddress)) {
							eep_content.owsensors2[x].familycode = currAddress[0];
							eep_content.owsensors2[x].romcode[0] = currAddress[1];
							eep_content.owsensors2[x].romcode[1] = currAddress[2];
							eep_content.owsensors2[x].romcode[2] = currAddress[3];
							eep_content.owsensors2[x].romcode[3] = currAddress[4];
							eep_content.owsensors2[x].romcode[4] = currAddress[5];
							eep_content.owsensors2[x].romcode[5] = currAddress[6];
							eep_content.owsensors2[x].crc = currAddress[7];
							
							eeprom_changed();	
							
  							// match sensor address to valve struct at runtime, otherwise restart needed
  							app_match_sensors();
						}
					}			
				}
			}
			else COMM_DBG.println("to few arguments");
		}


		// detect valve status
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_SETDETECTVLV,&cmd[0],5) == 0) {
			#ifdef commDebug 
				COMM_DBG.print("got detect valve status request");
			#endif

			x = atoi(arg0ptr);

			if(argcnt == 1 && x >= 0) {
				// reset status of all valves so they will be detected again
				if( x == 255) {
					for(unsigned int xx=0;xx<ACTUATOR_COUNT;xx++) {
						myvalvemots[xx].status = VLV_STATE_UNKNOWN; 
					}
					COMM_DBG.println(" - reset all valves");
				}
				// not supported at the moment, app rework needed
				// reset status of valve x so it will be detected again
				// else if (x < ACTUATOR_COUNT) {
				// 	myvalvemots[x].status = VLV_STATE_UNKNOWN;
				// 	COMM_DBG.print(" - reset valve ");
				// 	COMM_DBG.println(x, DEC);
				// }
				else COMM_DBG.println(" - error");
				
				COMM_SER.print(APP_PRE_SETDETECTVLV);
				COMM_SER.println(" ");
			}
			else COMM_DBG.println(" - error");		
		} 

		// unknown command
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else {
			//sprintf(&PrtBuf[0],"unknown command: %s\r",&cmdbuf[0]);
			//USART1_PutStr(&PrtBuf[0]);
			COMM_DBG.println("unknown command");
			return CMD_NONE;
		}

	}
	else return -1;

return 0;
}


/**
  * @brief  Write EEPROM Baselayout
  * @param  None
  * @retval None
  */
void WriteEEPROMBaselayout(void) {

	COMM_DBG.println("write EEPROM layout...");

	eeprom_fill ();		// write eeprom mark

	eep_content.b_slave = 0;
	strcpy(eep_content.descr, SYSTEM_NAME);
	eep_content.OneWireCfg[0] = 0;
	eep_content.OneWireCfg[1] = 0;
	eep_content.OneWireCfg[2] = 0;

//	eep.sensors[0].romcode[0] = 1;
//	eep.sensors[0].romcode[1] = 2;
//	eep.sensors[0].romcode[2] = 3;
//	eep.sensors[0].romcode[3] = 4;
//	eep.sensors[0].romcode[4] = 5;
//	eep.sensors[0].romcode[5] = 6;
//	eep.sensors[0].cfg = 0x55;
//	//eep.sensors[0].errors = 0x2FF2;
//	eep.sensors[0].crc = 0x11;
//	eep.sensors[0].cfg = 0x2222;
//	strcpy(eep.sensors[0].descr, "Sensor 1");
//	eep.sensors[0].modbusreg = 0x1111;

	eeprom_write_layout (&eep_content);
}

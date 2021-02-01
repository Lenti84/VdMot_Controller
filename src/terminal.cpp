/**
  ******************************************************************************
  * @file    terminal.c
  * @author  
  * @version V1.0
  * @date    
  * @brief   Mini-Terminal
  ******************************************************************************
  *
  */

#include <Arduino.h>
#include "hardware.h"
#include "terminal.h"
#include "motor.h"

//extern HardwareSerial Serial3;
extern void callback_app(char cmd, byte valveindex, byte pos);

HardwareSerial Serial3(USART3);

#define COMM_DBG				Serial3		// serial port for debugging

#define TERM_MAX_CMD_LEN		15			// max length of a command without arguments
#define TERM_ARG_CNT			 2			// number of allowed command arguments
#define TERM_MAX_ARG_LEN		20			// max length of one command argument


int16_t Terminal_Init (void) {

	Serial3.begin(115200);
	while(!Serial3);
	Serial3.println("VdMot Controller"); Serial3.flush();

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
    char		arg0[20];	
    char        arg1[20];		
	char*		arg0ptr = arg0;
	char*		arg1ptr = arg1;
	uint8_t		argcnt = 0;

	//int			itempval;
	uint16_t		x;
	uint16_t		y;


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
                //Serial.print("recv "); Serial.println(buffer);
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

		for(unsigned int x=0;x<3;x++){
			cmdptrend = strchr(cmdptr,' ');
			if (cmdptrend!=NULL) {
				*cmdptrend = '\0';
				if(x==0) 		strncpy(cmd,cmdptr,sizeof(cmd)-1);		// command
				else if(x==1) { strncpy(arg0,cmdptr,sizeof(arg0)-1); argcnt=1;	} 	// 1st argument
				else if(x==2) {	strncpy(arg1,cmdptr,sizeof(arg1)-1); argcnt=2;	} 	// 2nd argument
				cmdptr = cmdptrend + 1;
			}
		}


		// evsluate commands
		// ****************************************************************************************

		// help
		if(memcmp("help",&cmd[0],4) == 0) {
			Serial3.println("Help:");
			Serial3.println("*********************");
			
			return CMD_HELP;
		}

		// learn
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("learn",&cmd[0],5) == 0) {
			x = atoi(arg0ptr);
			//y = atoi(arg0ptr);

			if(argcnt == 1) {				
				//Serial3.println("learn valve x");				
				if (appsetaction(CMD_A_LEARN,x,0)!=0) Serial3.println("valve machine not idle");				
			}
			else Serial3.println("to few arguments");

			return CMD_LEARN;
		}

		// open to value
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("open",&cmd[0],5) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);

			if(argcnt == 2) {								
				if (appsetaction(CMD_A_OPEN,x,y)!=0) Serial3.println("valve machine not idle");
			}
			else Serial3.println("to few arguments");

			return CMD_OPEN;
		}

		// close to value
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("close",&cmd[0],5) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);

			if(argcnt == 2) {								
				if (appsetaction(CMD_A_CLOSE,x,y)!=0) Serial3.println("valve machine not idle");
			}
			else Serial3.println("to few arguments");

			return CMD_CLOSE;
		}

		// set target value
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("settar",&cmd[0],6) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);

			if(argcnt == 2)
			{								
				//if (appsetaction(CMD_A_CLOSE,x,y)!=0) Serial3.println("valve machine not idle");
				if(y<=100 && y>=0)
				{
					myvalves[x].target_position = y;
					Serial3.print("set valve "); Serial3.print(x, 10); Serial3.print(" to "); Serial3.println(y);
				}
			}
			else Serial3.println("to few arguments");

			return CMD_CLOSE;
		}


		// disable uart tx, helps flashing ESP8266 without unplugging
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("stxen",&cmd[0],5) == 0) {
			x = atoi(arg0ptr);

			if(argcnt == 1) {				
				
				if (x == 0) 
				{					
					COMM_DBG.println("comm: disable tx pin");
					// set uart1 tx pin PA9 to input, no pull
					MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF9 + GPIO_CRH_MODE9, GPIO_CRH_CNF9_0);
				}
				else {
					COMM_DBG.println("comm: enable tx pin");
					// set uart1 tx pin PA9 to output 10 Mhz, alternate function
					MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF9 + GPIO_CRH_MODE9, GPIO_CRH_CNF9_1 + GPIO_CRH_MODE9_0);
				}
			}
			else COMM_DBG.println("to few arguments");

			//return CMD_LEARN;
		}

		// set muxer
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("smux",&cmd[0],4) == 0) {
			x = atoi(arg0ptr);
			if(argcnt == 1) {
				if(x==0) MUX_OFF();
				else MUX_ON();
			}
		}

		// set direction
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("sdir",&cmd[0],4) == 0) {
			x = atoi(arg0ptr);
			if(argcnt == 1) {
				if(x==0) DIR_OFF();
				else DIR_ON();
			}
		}

		// set enable
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp("sena",&cmd[0],4) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);
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

		// unknown command
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else {
			//sprintf(&PrtBuf[0],"unknown command: %s\r",&cmdbuf[0]);
			//USART1_PutStr(&PrtBuf[0]);
			Serial3.print("unknown command: ");
			return CMD_NONE;
		}

	}
	else return -1;

return 0;
}




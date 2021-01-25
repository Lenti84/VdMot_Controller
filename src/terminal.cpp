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
	int16_t		key;
	char		cmdbuf[50];
	uint8_t		cmdindex=0;
	uint8_t		charenter=0;
	//float		ftempval;
	int			itempval;

	uint16_t		x;
	uint16_t		y;
	char*		cmdptr;
	char*		cmdptrend;

	char 		cmd[TERM_MAX_CMD_LEN] 				= "\0";			// command
	char		arg[TERM_ARG_CNT][TERM_MAX_ARG_LEN];				// max. TERM_ARG_CNT arguments
	char*		arg0ptr = &arg[0][0];
	char*		arg1ptr = &arg[1][0];
	uint8_t		argcnt = 0;

	arg[0][0] = '\0';
	arg[1][0] = '\0';
	//arg[2][0] = '\0';
	//arg[3][0] = '\0';

	key = Serial3.read();

	// get data from USART buffer
	while (key > -1 && cmdindex<48) {								// do until buffer is empty (-1)
	//while( Serial3.available() && cmdindex < 48 ) {
			
		if ( (((char) key) == '\r') || (((char) key) == '\n') ) {	// if linefeed detected
			charenter = 1;											// set flag
		}
		else {														// else
			cmdbuf[cmdindex] = (char) (key & 0xFF);					// write char to buffer
			cmdindex++;												// increment buffer index
		}
		key = Serial3.read();   									// get another received char
		
		if(cmdindex>=48) cmdindex=0;								// overlength -> reset
		if(charenter) cmdbuf[cmdindex] = ' ';						// mark string end
	}

	// was something received
	if(charenter) {

		// devide request into command and data
		// ****************************************
		cmdptr = &cmdbuf[0];

		for(x=0;x<3;x++){
			cmdptrend = strchr(cmdptr,' ');
			if (cmdptrend!=NULL) {
				*cmdptrend = '\0';
				if(x==0) 		strncpy(&cmd[0],cmdptr,TERM_MAX_CMD_LEN-1);		// comand
				else if(x==1) { strncpy(&arg[0][0],cmdptr,TERM_MAX_ARG_LEN-1); argcnt=1;	} 	// 1st argument
				else if(x==2) {	strncpy(&arg[1][0],cmdptr,TERM_MAX_ARG_LEN-1); argcnt=2;	} 	// 2nd argument
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

}




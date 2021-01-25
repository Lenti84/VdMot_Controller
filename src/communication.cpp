/**
  ******************************************************************************
  * @file    communication.c
  * @author  
  * @version V1.0
  * @date    
  * @brief   communication module to ESP8266
  ******************************************************************************
  *
  */

#include <Arduino.h>
#include "hardware.h"
#include "motor.h"
#include "communication.h"

#define COMM_SER				Serial		// serial port to ESP8266

#define COMM_MAX_CMD_LEN		15			// max length of a command without arguments
#define COMM_ARG_CNT			 2			// number of allowed command arguments
#define COMM_MAX_ARG_LEN		20			// max length of one command argument


void communication_setup (void) {
	
	// UART to ESP8266
	Serial.begin(115200);
	while(!Serial);
	//Serial.println("alive");Serial.flush();

}


/**
  * @brief  Handler
  * @param  None
  * @retval None
  */
int16_t communication_loop (void) {
	int16_t		key;
	char		cmdbuf[50];
	uint8_t		cmdindex=0;
	uint8_t		charenter=0;
	//float		ftempval;
	//int			itempval;

	char sendbuffer[30];
    char valbuffer[10];


	uint16_t		x;
	uint16_t		y;
	char*		cmdptr;
	char*		cmdptrend;

	char 		cmd[COMM_MAX_CMD_LEN] 				= "\0";			// command
	char		arg[COMM_ARG_CNT][COMM_MAX_ARG_LEN];				// max. COMM_ARG_CNT arguments
	char*		arg0ptr = &arg[0][0];
	char*		arg1ptr = &arg[1][0];
	uint8_t		argcnt = 0;

	arg[0][0] = '\0';
	arg[1][0] = '\0';

	key = COMM_SER.read();

	// get data from USART buffer
	while (key > -1 && cmdindex<48) {								// do until buffer is empty (-1)
	//while( COMM_SER.available() && cmdindex < 48 ) {
			
		if ( (((char) key) == '\r') || (((char) key) == '\n') ) {	// if linefeed detected
			charenter = 1;											// set flag
		}
		else {														// else
			cmdbuf[cmdindex] = (char) (key & 0xFF);					// write char to buffer
			cmdindex++;												// increment buffer index
		}
		key = COMM_SER.read();   									// get another received char
		
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
				if(x==0) 		strncpy(&cmd[0],cmdptr,COMM_MAX_CMD_LEN-1);		// comand
				else if(x==1) { strncpy(&arg[0][0],cmdptr,COMM_MAX_ARG_LEN-1); argcnt=1;	} 	// 1st argument
				else if(x==2) {	strncpy(&arg[1][0],cmdptr,COMM_MAX_ARG_LEN-1); argcnt=2;	} 	// 2nd argument
				cmdptr = cmdptrend + 1;
			}
		}


		// evaluate data
		// ****************************************************************************************

		// set target position
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		if(memcmp(APP_PRE_SETTARGETPOS,&cmd[0],5) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);

			if(argcnt == 2) {				
				COMM_SER.println("comm: set target position");
				if (y >= 0 && y <= 100 && x < ACTUATOR_COUNT) 
				{
					myvalves[x].target_position = y;
				}
			}
			else COMM_SER.println("to few arguments");

			//return CMD_LEARN;
		}

		// get valve data (position, meancurrent, status)
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		else if(memcmp(APP_PRE_GETVLVDATA,&cmd[0],5) == 0) {
			x = atoi(arg0ptr);
			y = atoi(arg1ptr);

			if(argcnt == 1) {				
				COMM_SER.println("comm: get valve data");
				if (x < ACTUATOR_COUNT) 
				{
					sendbuffer[0] = '\0';
					//Serial.println("sending new target value");
					strcat(sendbuffer, APP_PRE_GETVLVDATA);
					strcat(sendbuffer, " ");
					itoa(x, valbuffer, 10);		// valve nr
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");       
					itoa(myvalves[x].actual_position, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");
					itoa(myvalves[x].meancurrent, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");
					itoa(myvalves[x].status, valbuffer, 10);      
					strcat(sendbuffer, valbuffer);
					strcat(sendbuffer, " ");
					COMM_SER.println(sendbuffer);
					Serial3.println(sendbuffer);		// debug				
				}
			}
			else COMM_SER.println("to few arguments");

			//return CMD_LEARN;
		}

	}
	else return -1;


return 0;
}



/*----------------------------------------------------------------------------

------------------------------------------------------------------------------*/
#ifndef _TERMINAL_H
	#define _TERMINAL_H


#define 	CMD_NONE			0x00
#define 	CMD_HELP			0x01
#define 	CMD_CLOSE			0x02
#define 	CMD_OPEN			0x03
#define		CMD_LEARN			0x04


// public
extern int16_t Terminal_Init (void);
extern int16_t Terminal_Serve (void);



#endif //_TERMINAL_H



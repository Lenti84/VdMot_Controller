/*----------------------------------------------------------------------------

------------------------------------------------------------------------------*/
#ifndef _COMMUNICATION_H
	#define _COMMUNICATION_H

#include <Arduino.h>

// public
extern void communication_setup (void);
extern int16_t communication_loop (void);

#define APP_PRE_SETTARGETPOS       	"stgtp"
#define APP_PRE_GETACTUALPOS       	"gactp"
#define APP_PRE_GETMEANCURR        	"gmenc"
#define APP_PRE_GETSTATUS          	"gstat"
#define APP_PRE_GETVLVDATA			"gvlvd"
#define APP_PRE_SETTXENA			"stxen"

#endif //_COMMUNICATION_H



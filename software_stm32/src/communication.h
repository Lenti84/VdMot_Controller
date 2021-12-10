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
#define APP_PRE_GETSUPPLYSENS		"gspst"
#define APP_PRE_GETONEWIREDATA		"goned"
#define APP_PRE_SETSENSORINDEX		"stsnx"
#define APP_PRE_SETALLVLVOPEN		"staop"

#endif //_COMMUNICATION_H



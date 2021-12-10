#ifndef _APP_H_
#define _APP_H_

#include <Syslog.h>

typedef struct  {
	unsigned char actual_position;      // from controller
    unsigned char target_position;      // to controller
    unsigned char state;                // from controller
    unsigned int  meancurrent;          // from controller
    int           temperature;          // temperature of assigned sensor
} actuator_str;


// #define APP_CMD_NONE                0x00
// #define APP_CMD_SETTARGETPOS        0x01            // ESP -> STM - set target value
// #define APP_CMD_GETACTUALPOS        0x02            // ESP <- STM - get actual value
// #define APP_CMD_SETALLVLVOPEN       0x03            // ESP -> STM - open all valves


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



#define COMM_ALIVE_CYCLE            30          // send "Alive" cycle in 100 ms cycles

#define MAINTOPIC_LEN               30


void app_setup();
void app_loop();
void app_cmd(String command);

extern Syslog syslog;

extern char mqtt_maintopic[];
extern actuator_str actuators[];

extern uint8_t     stm32alive;


#endif // #ifndef 
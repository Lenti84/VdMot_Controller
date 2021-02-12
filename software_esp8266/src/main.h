#ifndef _MAIN_H_
#define _MAIN_H_

#include "app.h"

#define ACTUATOR_COUNT  (unsigned int) 12



// Syslog server connection info
#define SYSLOG_SERVER "192.168.4.20"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "my-device"
#define APP_NAME "my-app"
#define ANOTHER_APP_NAME "my-another-app"

// MQTT settings
#define DEFAULT_MAINTOPIC   "/VdMotFBH/valve/"           //  /VdMotFBH/valve/1/actual


extern const char* host;
extern const char* ssid;
extern const char* password;
extern const char* MQTT_BROKER;
extern actuator_str    actuators[ACTUATOR_COUNT];
extern char mqtt_maintopic[30];



#endif // #ifndef 
#ifndef _HARDWARE_H_
#define _HARDWARE_H_
#endif
#include <Arduino.h>
#include "app.h"

#define MAJORVERSION        "0"
#define MINORVERSION        "01"   

#define ACTUATOR_COUNT  (unsigned int) 12

// debug mode
#define VISMODE_DEFAULT     VISMODE_DETAIL               
#define VISMODE_OFF         0               // no messages at all
#define VISMODE_ON          1               // minimum debug messages
#define VISMODE_DETAIL      2               // detailed debug messages
#define VISMODE_ATOMIC      3               // atomic debug messages
extern uint8_t vismode;

// Syslog server connection info
#define SYSLOG_SERVER       "192.168.4.20"
#define SYSLOG_PORT         514

// This device info
#define DEVICE_HOSTNAME     "my-device"
#define APP_NAME            "my-app"
#define ANOTHER_APP_NAME    "my-another-app"

// hardware
#define NRST                15              // IO15
#define BOOT0               14              // IO14
//#define LED               2               // not available on WT32-ETH01
#define UART_STM32          Serial2         // serial interface connected to STM32
#define UART_DBG			Serial		    // serial port for debugging

// on WT32-ETH01 V1.2 use for Serial2 pins: RX2 IO5 and TX2 IO17
#define STM32_RX            5
#define STM32_TX            17


// MQTT settings
#define DEFAULT_MAINTOPIC   "/VdMotFBH/valve/"           //  /VdMotFBH/valve/1/actual
#define MQTT_BROKER_IP      "192.168.4.201"

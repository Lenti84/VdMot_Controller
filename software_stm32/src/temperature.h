/*----------------------------------------------------------------------------

------------------------------------------------------------------------------*/
#ifndef _TEMPERATURE_H
	#define _TEMPERATURE_H

#include <Arduino.h>
#include <DallasTemperature.h>


extern void temperature_setup();
extern void temperature_loop();
extern void get_sensordata (char *buffer, int buflen);

void printAddress(DeviceAddress deviceAddress);

#define MAXSENSORCOUNT 		(unsigned int)	25
#define CONV_INTERVALL		(unsigned int)	2000			// conversion intervall in ms

typedef struct {
  int 			temperature;
  DeviceAddress address;  
} tempsensor;

extern tempsensor tempsensors[];
extern uint8_t numberOfDevices;

extern DallasTemperature sensors;







#endif //_TEMPERATURE_H



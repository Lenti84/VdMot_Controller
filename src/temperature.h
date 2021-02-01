/*----------------------------------------------------------------------------

------------------------------------------------------------------------------*/
#ifndef _TEMPERATURE_H
	#define _TEMPERATURE_H


void temperature_setup();
void temperature_loop();

#define SENSORCOUNT 		(unsigned int)	256
#define CONV_INTERVALL		(unsigned int)	200			// conversion intervall in 10 ms cycles


extern int tempsensors[SENSORCOUNT];


#endif //_TEMPERATURE_H



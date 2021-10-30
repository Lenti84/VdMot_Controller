/*----------------------------------------------------------------------------

------------------------------------------------------------------------------*/
#ifndef _EEPROM_H
	#define _EEPROM_H

#include <Arduino.h>

int16_t eepromsetup();
int16_t eepromloop();
void eeprom_fill (void);
int16_t eeprom_write_layout (struct eeprom_layout* lay);
int16_t eeprom_read_layout (struct eeprom_layout* lay);

extern struct eeprom_layout eep_content;


#endif
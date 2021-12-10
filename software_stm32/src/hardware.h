#include "motor.h"

// defines used hardware

// controller / board
// STM32 blackpill
// with STM32F401CCU

// serial ports
// Serial1 - communication to ESP32 - PA9/PA10
// Serial2 - RS485 - PA2/PA3
// Serial6 - debug / terminal - PA11/PA12
//

// status LED 
#define LED         PC13

// button
#define BUTTON		PB2

// valve MUX relay
#define POWER_ENA   PB9

// valve MUX relay
#define CTRL_MUX    PB1

// L293 enable pins
#define CTRL_ENA0   PA5
#define CTRL_ENA1   PA6
#define CTRL_ENA2   PA7
#define CTRL_ENA3   PB0
#define CTRL_ENA4   PA15
#define CTRL_ENA5   PB3

// L293 direction pin
#define CTRL_DIRECTION PA8

// input for counting revs
#define REVINPIN    PA4

// analog input reference/2
#define ANINREFHALF		PA1

// analog input motor current
#define ANINCURRENT		PA0

// analog frontend for current measurement
#define SHUNT_RES		3900				// shunt resistance in milliohm
#define SHUNT_OPGAIN	1500				// gain of opamp stage for current measurement in milli
#define ADC_REF_VOLT	3300				// adc reference voltage in mV	
//#define ANINCURRENTGAIN    (int32_t) (((ADC_REF_VOLT*1e9)/SHUNT_RES)/SHUNT_OPGAIN)/4096     // µA/digit - gain analog input valve current 
#define ANINCURRENTGAIN    (int32_t) 138     // µA/digit - gain analog input valve current 

// SPI bus
#define SPI_CS_PIN      PB12
#define SPI_MOSI_PIN    PB15
#define SPI_MISO_PIN    PB14
#define SPI_CLK_PIN     PB13

// I2C
#define I2C_SCL_PIN		PB6
#define I2C_SDA_PIN		PB7

// 1-wire
#define ONEW_PIN		PB10

// CAN INT
#define CAN_INT_PIN		PB8

// RS485
#define RS485_TX_PIN	PA2
#define RS485_RX_PIN	PA3
#define RS485_RXENA_PIN	PB4
#define RS485_TXENA_PIN	PB5

// macros  
#define ENA0_ON()    digitalWrite(CTRL_ENA0, HIGH)
#define ENA0_OFF()    digitalWrite(CTRL_ENA0, LOW)
#define ENA1_ON()    digitalWrite(CTRL_ENA1, HIGH)
#define ENA1_OFF()    digitalWrite(CTRL_ENA1, LOW)
#define ENA2_ON()    digitalWrite(CTRL_ENA2, HIGH)
#define ENA2_OFF()    digitalWrite(CTRL_ENA2, LOW)
#define ENA3_ON()    digitalWrite(CTRL_ENA3, HIGH)
#define ENA3_OFF()    digitalWrite(CTRL_ENA3, LOW)
#define ENA4_ON()    digitalWrite(CTRL_ENA4, HIGH)
#define ENA4_OFF()    digitalWrite(CTRL_ENA4, LOW)
#define ENA5_ON()    digitalWrite(CTRL_ENA5, HIGH)
#define ENA5_OFF()    digitalWrite(CTRL_ENA5, LOW)


#define DIR_ON()     digitalWrite(CTRL_DIRECTION, HIGH)
#define DIR_OFF()    digitalWrite(CTRL_DIRECTION, LOW)

#define MUX_ON()     digitalWrite(CTRL_MUX, HIGH)
#define MUX_OFF()    digitalWrite(CTRL_MUX, LOW)

#define PSU_ON()     digitalWrite(POWER_ENA, LOW);   // enable PSU for valves   
#define PSU_OFF()    digitalWrite(POWER_ENA, HIGH);   // disable PSU for valves   



// defines 

#define SYSTEM_NAME         "VdMot Controller"

#define EEPROM_MARK_ADD		0		// address 4 byte mark: 0x1F2F3F4F
#define EEPROM_VERS1_ADD	4		// address 1 byte Version 1.x.x
#define EEPROM_VERS2_ADD	5		// address 1 byte Version x.2.x
#define EEPROM_VERS3_ADD	6		// address 1 byte Version x.x.3


// EEPROM layout
//
#define EE_GENERALDATA_ADR		0x0007		// start of general configuration data
//#define EE_SENSORDATA_ADR		0x0800		// start of sensor configuration data
// #if EE_SENSORDATA_ADR < EE_GENERALDATA_ADR
// #error "EEPROM: adress for EE_SENSORDATA_ADR collides with EE_GENERALDATA_ADR"
// #endif

struct ds1820_eeprom_layout {
	uint8_t		familycode;				// family code
	uint8_t		romcode[6];				// rom code
	uint8_t		crc;					// crc
};

struct eeprom_layout {
	uint8_t     b_slave;                // 0 -> master, >0 -> slave
    char		descr[25];				// string for system description
	uint8_t		OneWireCfg[3];		    // 3 bytes for One Wire Gateway Configuration						
										// byte 0 - conversion interval

	struct ds1820_eeprom_layout owsensors[ACTUATOR_COUNT];	// a lot of ds1820 sensors
};


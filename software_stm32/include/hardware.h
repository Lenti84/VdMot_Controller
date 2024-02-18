/**HEADER*******************************************************************
  project : VdMot Controller
  author : Lenti84
  Comments:
  Version :
  Modifcations :
***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE DEVELOPER OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller
*END************************************************************************/

#ifndef _HARDWARE_H
	#define _HARDWARE_H

#include "compile_time.h"

// version info
#ifndef FIRMWARE_BUILD
  #define FIRMWARE_BUILD __TIME_UNIX__
#endif


// defines used hardware

// controller / board
// STM32 blackpill
// with STM32F401CCU

// serial ports
// Serial1 - communication to ESP32 - PA9/PA10
// Serial2 - RS485 - PA2/PA3
// Serial6 - debug / terminal - PA11/PA12

#define COMM_DBG				Serial6		// serial port for debugging

// status LED 
#define LED         PC13

// button
#define BUTTON		PB2

// valve MUX relay
#define POWER_ENA   PB9

// valve MUX relay
#define CTRL_MUX    PB1

// L293 enable pins
#define CTRL_ENA0   PA5         // TIM2_CH1
#define CTRL_ENA1   PA6         // TIM3_CH1
#define CTRL_ENA2   PA7         // TIM3_CH2
#define CTRL_ENA3   PB0         // TIM3_CH3
#define CTRL_ENA4   PA15        // TIM2_CH1
#define CTRL_ENA5   PB3         // TIM2_CH2

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


#ifdef HARDWARE_REVISION_C1
  // MUX pin definition for C1-sample
  #define MUX_ON()     digitalWrite(CTRL_MUX, HIGH)
  #define MUX_OFF()    digitalWrite(CTRL_MUX, LOW)
#elif HARDWARE_REVISION_C2
  // MUX pin definition for C2-sample
  #define MUX_ON()     digitalWrite(CTRL_MUX, LOW)
  #define MUX_OFF()    digitalWrite(CTRL_MUX, HIGH)
#endif

#define PSU_ON()     digitalWrite(POWER_ENA, LOW);   // enable PSU for valves   
#define PSU_OFF()    digitalWrite(POWER_ENA, HIGH);   // disable PSU for valves   



// defines 

#define ACTUATOR_COUNT      		12    	// how many valves are supported
#define ADDITIONAL_SENSOR_COUNT		10 		// some additional sensors for x
#define MAXONEWIRECNT		(ACTUATOR_COUNT*2)+ADDITIONAL_SENSOR_COUNT				// max count of usable 1-wire sensors

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

enum EEP_STATE { EEP_INIT, EEP_VALID, EEP_CHANGED };

struct ds1820_eeprom_layout {
	uint8_t		familycode;				// family code
	uint8_t		romcode[6];				// rom code
	uint8_t		crc;					    // crc
};

struct eeprom_layout {
  enum EEP_STATE status;          // status of eeprom content
	uint8_t   b_slave;              // 0 -> master, >0 -> slave
  char		  descr[25];				    // string for system description
	uint8_t		OneWireCfg[3];		    // 3 bytes for One Wire Gateway Configuration						
										              // byte 0 - conversion interval
  uint8_t currentbound_low_fac;   
  uint8_t currentbound_high_fac;
  uint16_t numberOfMovements;
	struct ds1820_eeprom_layout owsensors1[ACTUATOR_COUNT];	// a lot of ds1820 sensors - first sensor of valve
	struct ds1820_eeprom_layout owsensors2[ACTUATOR_COUNT];	// a lot of ds1820 sensors - second sensor of valve
	struct ds1820_eeprom_layout owsensors[ADDITIONAL_SENSOR_COUNT];	// some other ds1820 sensors
  uint8_t startOnPower;
  uint16_t noOfMinCounts;
  uint8_t maxCalibRetries;
};



#endif		//_HARDWARE_H
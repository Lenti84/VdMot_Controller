# VdMot_Controller
This is a controller for HmIP-VdMot actuators.

It controls up to 12 HmIP-VdMot actuators or other DC motor valves and is based on custom hardware/software.

It's an alternative to the original HmIP-FALMOT-C12 hardware.
My intention was to create a cheap solution to operate my 18 valves (floor heating + radiators).
I wanted a pure offline system (no cloud connection) and to be free of the constraint to use further components like CCU2/CCU3.

## Features:
- controls up to 12 HmIP-VdMot actuators or other DC motor valves
- learns valve movement by counting motor revolutions based on Back EMF
  - so positioning accuracy should be (very) good
- valve current is evaluated and can be monitored
- interfaces
  - actual: MQTT
  - future: MODBUS, CAN
- integrated 1-wire master
  - a lot of additional temperature sensors like DS18B20 could be connected
  - sensor values can be linked to a valve for combined data evaluation
  - usefull for hydraulic balancing ("Hydraulischer Abgleich")
  
## Status
- b-sample hardware is working with original HmIP-VdMot actuator
- c-sample hardware in development (see hardware section)
- valve learning, opening and closing is working
- connection to [FHEM](https://fhem.de/) established via MQTT
  - topics:
    - /VdMotFBH/valve/x/target      --> set target position for valve (0...100 %)
    - /VdMotFBH/valve/x/actual      --> get actual position of valve (0...100 %) 
    - /VdMotFBH/valve/x/state       --> get actual state of valve (IDLE, OPENING, CLOSING, BLOCKING, OPENCIRCUIT, UNKNOWN)
    - /VdMotFBH/valve/x/meancur     --> get mean current of valve (mA)
    - /VdMotFBH/valve/x/temperature --> get temperature of linked 1-wire sensor (1/10 °C)
    - x = valve number
- first productive test still pending
- tests of 2 actuators done, see [system/actuators.md](./system/actuators.md)
- one wire sensors working
  - each sensor temperature can be coupled to a valve
  - therefore the sensor adresses are stored and assigned at startup
- eeprom working (and changed to I2C type)
- programming of STM32 by ESP32 first tests successfully performed
- change from STM32F103 to STM32F401 due to procurement issues
- simple test of RS485 interface with modbus master sucessfully performed
- simple test of CAN interface via MCP2515 sucessfully performed

## Hardware
- find description of hardware here [hardware/hardware.md](./hardware/hardware.md)

## Software
- written in C / C++
- not pretty (yet) ... i'm more the hardware guy ;-)
- uses great arduino libraries
- developed using PlatformIO
- STM32 BluePill
  - controls the valves / dc-motors
    - endstop by real current measurement
    - counting motor revolutions by 
  - controlls DS2482-100 1-wire bus master ic
  - serving simple terminal for debugging purposes
  - future
    - so many things, tbd
- ESP8266
  - communication with MQTT broker
  - future
    - visualize system status
    - json interface
    - OTA software update
    - flash STM32


## License
This project is licensed under the terms of the GNU General Public License v3.0 license.

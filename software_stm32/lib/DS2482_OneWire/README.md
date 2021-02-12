DS2482_OneWire
==============

A OneWire library using the DS2482 I2C->1-Wire bridge. Released under GPL.

This is designed to use Dallas DS18B20 temperature sensors connected to a DS2482 I2C->1-Wire bridge as part of a temperature monitoring system.

The code heavily borrows from two sources:
* Paeaetech's DS2482 library (https://github.com/paeaetech/paeae/tree/master/Libraries/ds2482) - the bulk of the functionality if from here, slighty changed.
* OneWire library (http://www.pjrc.com/teensy/td_libs_OneWire.html) - the CRC code is from here, along with all method names so we can use new library in place of this.

DS18B20 is a 1-Wire temperature sensor (http://www.maximintegrated.com/datasheet/index.mvp/id/2812). These are commonly driven directly from an I/O pin using the OneWire library. This is fine for short and simple networks, but errors start to occur with large numbers (6+ devices) and networks longer than 5m or so.

The DS2482-100 is a I2C->1-Wire bridge (http://www.maximintegrated.com/datasheet/index.mvp/id/4382), containly functionality that means it is better at driving long and complex networks. 

Breakouts for the Raspberry Pi, which can be connected to Arduino, are available from Sheepwalk Electronics:
http://www.sheepwalkelectronics.co.uk/product_info.php?cPath=22&products_id=30



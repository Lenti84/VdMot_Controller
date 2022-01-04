This page will document the software part of the VdMot_Controller.

# ESP32
## Feature list software
- responsive design
- user login
- status page
  - valves: index, description, position, mean current, status, assigned temperature sensor with temperature value
  - temperature sensors: description, temperature, index
- setup page
  - network interface: wifi or ethernet (RJ45)
  - wifi: user and password
  - valves: description/name, assignment of temperature sensor, calibration request
  - temperature sensors: description/name
  - mqtt: server ip, server port, user name and password
  - settings: save, recover
- flash / ota page
  - flash STM32 by file upload
  - flash ESP32 by direct send chunks

## ESP32 user interface
- home page  
  ![-](./software_esp32/media/home.png "home page")
- status page  
  ![-](./software_esp32/media/status_page.png "status page")
- network page  
  ![-](./software_esp32/media/network.png "network page")
- config page  
  ![-](./software_esp32/media/config.png "config page")
- about page  
  ![-](./software_esp32/media/about.png "about page")
- update page  
  ![-](./software_esp32/media/update.png "update page")
- esp32 update page  
  ![-](./software_esp32/media/wt32Update.png "update esp32 page")
- stm32 update page  
  ![-](./software_esp32/media/stm32Update.png "update stm32 page")


# command list
## STM32 supports following commands
|command|description|answer|units|comment|
|---|---|---|---|---|
|stgtp x y|set target position of valve *x* to *y* %|-|x=0...11, y=0...100%||
|gtgtp x|get target position of valve *x*|gtgtp *target_position*|0...100%||
|gvlvd x|get data of valve *x*|gvlvd *position meancurrent status 1st_temperature 2nd_temperature*|*% mA - 1/10°C 1/10°C*||
|stons|start new 1-wire search|-|-|get result via *gonec* and *goned*|
|gonec|get count of 1-wire devices|gonec *count*|*-*||
|goned x|get data (adress and temperature) of 1-wire sensor with index *x*|goned *6ByteHexAddress temperature*|*0x 0x 0x 0x 0x 0x 1/10°C*||
|gvlon x|get 1-wire settings for valve *x*|gvlon *6ByteHexAddressFirstSensor* - *6ByteHexAddressSecondSensor*|*0x 0x 0x 0x 0x 0x 0x 0x 0x 0x 0x 0x*||
|stsnx x y|set first 1-wire sensor index *y* to valve *x*|-|x=0...11, y=0...35||
|stsny x y|set second 1-wire sensor index *y* to valve *x*|-|x=0...11, y=0...35||
|staop x|set valve *x* open|-|0...11, 255|usefull for installation, opens quickly one (x) or all valves (x=255) so they can be mounted|
|staln x|set learning for valve *x*|-|0...11, 255|performs learning cycle for one (x) or all valves (x=255)|
|stlnm x|set learning movement counter|-|0...32000|after x movement requests to valve a new learning cycle will be perfomed|
|stlnt x|set learning time|-|0...1000000|after x seconds a new learning cycle will be perfomed|
|gvers|get version|gvers *version_string*|tbd||



# OTA update of ESP32 and STM32
## STM32 OTA
- STM32 can be reset via ESP32 pin
- STM32 BOOT0 pin is not accessible by ESP32 cause of BlackPill board design
- the alternative approach: STM32 jumps to internal bootloader after receiving a magic sequence from ESP32 right after start-up
- idea of general procedure for STM32 update via ESP32
  - ESP32 switches to UART config 9600 8E1 - the even parity is important for STM32 bootloader
  - reset STM32 via ESP32 GPIO
  - STM32 starts up and listens for 2 seconds at UART for magic sequence (UART config: 9600 8E1)
  - ESP32 sends the magic sequence "DEADBEEF"
  - STM32 answers with "BEEFIT"
  - STM32 jumps to internal bootloader
  - ESP32 waits for some time
  - now ESP32 can use STM32 internal bootloader to program it
  - reset STM32 via ESP32 GPIO
  - ESP32 switches back to default UART config 115200 8N1
  - STM32 update finished

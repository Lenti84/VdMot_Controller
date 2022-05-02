# Hardware

![-](./hardware/tht_assembly.png "tht assembly drawing")



# Init Flash WT32

![-](./hardware/ftdi.jpg "FTDI")

X22 - Jumper set for init flash

X23 - male header for UART connection

    GND -> GND 
    TX  -> TX 
    RX  -> RX 

Configuration platformoio.ini

upload_port = Port of FTDI Board
upload_speed = 115200

Start upload direct from Platformio in Visual Studio Code.

# Init Flash STM

X20 - Jumper set for STM Reset, needs for flash STM

Option "boot partition" 
Press - BOOT0 Button at BlackPill Board

Start Init Flash
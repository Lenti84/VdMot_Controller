#include <mcp_can.h>
#include <SPI.h>
#include "hardware.h"

// just test code to validate hardware for can bus
// code snippets from library CAN_BUS_Shield from Seeed Technology Inc.
// examples used: "send" and "receive_interrupt"

//#define CAN_DBG				Serial3		// serial port for debugging
#define CAN_DBG				Serial6		// serial port for debugging

void MCP2515_ISR();

MCP_CAN CAN(SPI_CS_PIN);         // Set CS pin
unsigned char Flag_Recv = 0;

void mycan_setup() {
  CAN_DBG.println("CAN test started ...");

  SPI.setMOSI(SPI_MOSI_PIN);
  SPI.setMISO(SPI_MISO_PIN);
  SPI.setSCLK(SPI_CLK_PIN);
  
 START_INIT:

    if(CAN_OK == CAN.begin(CAN_500KBPS))               // init can bus : baudrate = 500k
    {
        CAN_DBG.println("CAN BUS Shield init ok!");
    }
    else
    {
        CAN_DBG.println("CAN BUS Shield init fail");
        CAN_DBG.println("Init CAN BUS Shield again");
        delay(100);
        goto START_INIT;
    }

    attachInterrupt(CAN_INT_PIN, MCP2515_ISR, FALLING); // start interrupt
}

void mycan_loop() {

  unsigned char stmp[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  static int can_timer = 0;
  unsigned char len = 0;
  unsigned char buf[8];

  if(can_timer<100) 
  {
    can_timer++;
  }
  else
  {
    can_timer=0;
 
    // send data:  id = 0x123, standrad flame, data len = 8, stmp: data buf
    CAN.sendMsgBuf(0x123, 0, 8, stmp);
  }

  if(Flag_Recv)                   // check if get data
  {
  
      Flag_Recv = 0;                // clear flag
      CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

      for(int i = 0; i<len; i++)    // print the data
      {
          CAN_DBG.print(buf[i]);Serial.print("\t");
      }
      CAN_DBG.println();
  }
}


// falling edge interrupt for MCP2515 INT pin
void MCP2515_ISR()
{
     Flag_Recv = 1;
}
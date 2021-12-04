
#include "stm32ota.h"

const String STM32_CHIPNAME[8] = {
  "Unknown Chip",
  "STM32F03xx4/6",
  "STM32F030x8/05x",
  "STM32F030xC",
  "STM32F103x4/6",
  "STM32F103x8/B",
  "STM32F103xC/D/E",
  "STM32F105/107"
};

void stm32SendCommand(unsigned char commd) {    // Tested
  Serial2.write(commd);
  Serial2.write(~commd);
}

unsigned char stm32Erase() {     // Tested
  stm32SendCommand(STM32ERASE);
  while (!Serial2.available());
  if (Serial2.read() == STM32ACK)
  {
    Serial2.write(0xFF);
    Serial2.write(0x00);
  }
  else return STM32ERR;
  while (!Serial2.available());
  return Serial2.read();
}

unsigned char stm32Erasen() {     // Tested
  stm32SendCommand(STM32ERASEN);
  while (!Serial2.available());
  if (Serial2.read() == STM32ACK)
  {
    Serial2.write(0xFF);
    Serial2.write(0xFF);
    Serial2.write(0x00);
  }
  else return STM32ERR;
  while (!Serial2.available());
  return Serial2.read();
}

// No test yet
unsigned char stm32Run()   {
  stm32SendCommand(STM32RUN);
  while (!Serial2.available());
  if (Serial2.read() == STM32ACK) {
    stm32Address(STM32STADDR);
    return STM32ACK;
  }
  else
    return STM32ERR;
}

// No test yet
unsigned char stm32Read(unsigned char * rdbuf, unsigned long rdaddress, unsigned char rdlen) {
  stm32SendCommand(STM32RD);
  while (!Serial2.available());
  if (Serial2.read() == STM32ACK)
    stm32Address(rdaddress);
  else return STM32ERR;
  while (!Serial2.available());
  if (Serial2.read() == STM32ACK)
    stm32SendCommand(rdlen);
  while (!Serial2.available());
  size_t getlen = Serial2.available();
  Serial2.readBytes(rdbuf, getlen);
  return STM32ACK;
}

unsigned char stm32Address(unsigned long addr) {    // Tested
  unsigned char sendaddr[4];
  unsigned char addcheck = 0;
  sendaddr[0] = addr >> 24;
  sendaddr[1] = (addr >> 16) & 0xFF;
  sendaddr[2] = (addr >> 8) & 0xFF;
  sendaddr[3] = addr & 0xFF;
  for (int i = 0; i <= 3; i++) {
    Serial2.write(sendaddr[i]);
    addcheck ^= sendaddr[i];
  }
  Serial2.write(addcheck);
  while (!Serial2.available());
  return Serial2.read();
}

unsigned char stm32SendData(unsigned char * data, unsigned char wrlen) {     // Tested
  Serial2.write(wrlen);
  for (int i = 0; i <= wrlen; i++) {
    Serial2.write(data[i]);
  }
  Serial2.write(getChecksum(data, wrlen));
  while (!Serial2.available());
  return Serial2.read();
}

char stm32Version() {     // Tested
  int x;
  unsigned char vsbuf[14];

  while(Serial2.available()) Serial2.read();

  Serial.println("GetVersion");
  
  stm32SendCommand(STM32GET);

  // while (!Serial2.available());
  for (x=0;x<100;x++) {
    if (Serial2.available()>0) {  
      vsbuf[0] = Serial2.read();
      if (vsbuf[0] != STM32ACK) {
        Serial.println("Error GetVersion: wrong answer");
        return STM32ERR;
      }
      else {
        Serial2.readBytesUntil(STM32ACK, vsbuf, 14);
        return vsbuf[1];
      }
    }
    else Serial.println("Error GetVersion: no data");
    delay(20);
  }
  return STM32ERR;
}

unsigned char stm32GetId() {     // Tested
  int x;
  int getid = 0;
  unsigned char sbuf[5];

  while(Serial2.available()) Serial2.read();

  Serial.println("GetId");
  
  stm32SendCommand(STM32ID);
  
//while (!Serial2.available());
  for (x=0;x<100;x++) {
    if (Serial2.available()>0) {          
      sbuf[0] = Serial2.read();
      if (sbuf[0] == STM32ACK) {
        Serial2.readBytesUntil(STM32ACK, sbuf, 4);
        getid = sbuf[1];
        getid = (getid << 8) + sbuf[2];
        Serial.print("Id: ");
        Serial.print(getid, HEX); 
        Serial.println(""); 
        if (getid == 0x444)
          return 1;
        if (getid == 0x440)
          return 2;
        if (getid == 0x442)
          return 3;
        if (getid == 0x412)
          return 4;
        if (getid == 0x410)
          return 5;
        if (getid == 0x414)
          return 6;
        if (getid == 0x418)
          return 7;
      }
      else {
        Serial.println("Error GetId: wrong answer");
        return 0;
      }
    }
    else Serial.println("Error GetId: no data");
    delay(20);
  }
  return 0;
}

unsigned char getChecksum( unsigned char * data, unsigned char datalen) {    // Tested
  unsigned char lendata = datalen;
  for (int i = 0; i <= datalen; i++)
    lendata ^= data[i];
  return lendata;
}

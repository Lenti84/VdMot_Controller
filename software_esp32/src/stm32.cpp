#include "stm32ota.h"
#include "globals.h"




void STM32ota_setup() {

  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  //UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  //while(UART_STM32.available()) UART_STM32.read();

  // BOOT and RESET Pin of STM32  
  pinMode(BOOT0, OUTPUT);
  pinMode(NRST, OUTPUT);

}


// has to be called prior to stm32 flash transactions
void STM32ota_begin() {
  UART_DBG.println("STM32 ota begin");
  // flash interface
  // ATTENTION: on WT32-ETH01 use for Serial2 pins: RX2 IO35 and TX2 IO17
  UART_STM32.begin(115200, SERIAL_8E1, STM32_RX, STM32_TX, false, 20000UL);
  while(UART_STM32.available()) UART_STM32.read();

}


void ResetSTM32() {
  UART_DBG.println("Reset STM32");
  delay(100);
  digitalWrite(NRST, HIGH);
  
  delay(150);
  digitalWrite(NRST, LOW);
  delay(500);
}


void FlashMode()  {    //Tested
  UART_DBG.println("Set Flash mode");
  digitalWrite(BOOT0, HIGH);
  delay(100);
  digitalWrite(NRST, HIGH);
  //digitalWrite(LED, LOW);
  delay(1500);
  digitalWrite(NRST, LOW);
  delay(500);
//   for ( int i = 0; i < 3; i++) {
//     //digitalWrite(LED, !digitalRead(LED));
//     delay(100);
//   }

//   while(UART_STM32.available()) UART_STM32.read();
//   delay(100);
}

void RunMode()  {    //Tested
  UART_DBG.println("Set Run mode");
  digitalWrite(BOOT0, LOW);
  delay(100);
  digitalWrite(NRST, HIGH);
//digitalWrite(LED, LOW);
  delay(150);
  digitalWrite(NRST, LOW);
  delay(500);
//   for ( int i = 0; i < 3; i++) {
//     //digitalWrite(LED, !digitalRead(LED));
//     delay(100);
//   }
}
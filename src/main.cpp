#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>

#include <LiquidCrystal_PCF8574.h>
//#include "valve_mgmt.h"
#include "motor.h"
#include "terminal.h"

void callback_app();

HardwareSerial Serial3(USART3);


//using Matthias Hertel driver https://github.com/mathertel/LiquidCrystal_PCF8574
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display


char inChar;
byte valvenr;
byte position;



void setup() {

   // status led
  pinMode(LED, OUTPUT);

  // Debug UART
  Serial3.begin(115200);
  while(!Serial3);
  Serial3.println("Arduino Menu Library");Serial3.flush();

  // LCD
  lcd.begin(16,2);  
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("Menu 4.x LCD");
  lcd.setCursor(0, 1);
  lcd.print("r-site.net");
  delay(2000);

  // valve app setup
  appsetup();

  Terminal_Init();
}

void loop() {
  static int x = 0;
  
  unsigned char len = 0;

  
  static uint32_t time = 0;
  static uint32_t loop_10ms = 0;
  static uint32_t loop_100ms = 0;

  int16_t recvcmd;

  

  if (millis() > (uint32_t) 1000 + time ) {    
    time = millis();

    //Serial3.println("ttt");

    //current_mA = ina219.getCurrent_mA();
    //Serial3.print("Current:       "); Serial3.print(current_mA); Serial3.println(" mA");
    //Serial3.print("Revs: "); Serial3.println(revcounter);

    
    digitalWrite(LED, !digitalRead(LED));   // turn the LED on (HIGH is the voltage level)
    //delay(1000);              // wait for a second

    //digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
    //delay(1000);              // wait for a second

    if (x<3) {
      x++;
      //digitalWrite(CTRL_MUX, !digitalRead(CTRL_MUX));
    }

  }

  // 100 ms loop
  if (millis() > (uint32_t) 100 + loop_100ms ) {
    loop_100ms = millis();  

    // len = Serial3.available();
    // if (len==1) {
    //   inChar = (char)Serial3.read(); 
    // }
    // else if (len==2) {
    //   inChar  = (char)Serial3.read(); 
    //   valvenr = (byte)Serial3.read() - 48;
    // }
    // else {
    //   while(Serial3.available()) { Serial3.read(); }
    // }
    //else inChar = 0;

    recvcmd = Terminal_Serve();
  }


  // if(inChar == 'a') {
  //   //ENA0_ON();
  //   Serial3.println("ENA0 on");
  //   Serial3.flush();
  // }
  // else if (inChar == 's') {
  //   //ENA0_OFF();
  //   Serial3.println("ENA0 off");
  //   Serial3.flush();
  // }
  // else if (inChar == 'q') {
  //   //DIR_ON();
  //   //MUX_ON();
  //   Serial3.println("DIR on");
  //   Serial3.flush();
  // }
  // else if (inChar == 'w') {
  //   //DIR_OFF();
  //   //MUX_ON();
  //   Serial3.println("DIR off");
  //   Serial3.flush();
  // }

  //delay(50);

  // 10 ms loop
  if (millis() > (uint32_t) 10 + loop_10ms ) {    
    loop_10ms = millis();  
    //valvecycle();

    appcycle(inChar, valvenr, position);
    
    inChar = 0;   // clear command for next cycle
  }
}


void callback_app(char cmd, byte valveindex, byte pos) {

  valvenr = valveindex;
  inChar = cmd;
  position = pos;

}
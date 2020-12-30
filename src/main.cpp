#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_PCF8574.h>


HardwareSerial Serial3(USART3);

Adafruit_INA219 ina219;

//using Matthias Hertel driver https://github.com/mathertel/LiquidCrystal_PCF8574
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display


void setup() {
  // Debug UART
  Serial3.begin(115200);
  while(!Serial3);
  Serial3.println("Arduino Menu Library");Serial3.flush();

  // status led
  pinMode(LED, OUTPUT);

  // valve MUX relay
  pinMode(CTRL_MUX, OUTPUT);

  // L293 enable pins
  pinMode(CTRL_ENA0, OUTPUT);
  pinMode(CTRL_ENA1, OUTPUT);
  pinMode(CTRL_ENA2, OUTPUT);
  pinMode(CTRL_ENA3, OUTPUT);
  pinMode(CTRL_ENA4, OUTPUT);
  pinMode(CTRL_ENA5, OUTPUT);

  // L293 direction pin
  pinMode(CTRL_DIRECTION, OUTPUT); 

  // Initialize the INA219
  if (! ina219.begin()) {
    Serial3.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  ina219.setCalibration_16V_320mA();  
  
  // LCD
  lcd.begin(16,2);  
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("Menu 4.x LCD");
  lcd.setCursor(0, 1);
  lcd.print("r-site.net");
  delay(2000);
}

void loop() {
  static int x = 0;
  float current_mA = 0;
  char inChar;
  static uint32_t time = 0;

  

  if (millis() > (uint32_t) 1000 + time ) {    
    time = millis();

    //Serial3.println("ttt");

    current_mA = ina219.getCurrent_mA();
    Serial3.print("Current:       "); Serial3.print(current_mA); Serial3.println(" mA");
    
    digitalWrite(LED, !digitalRead(LED));   // turn the LED on (HIGH is the voltage level)
    //delay(1000);              // wait for a second

    //digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
    //delay(1000);              // wait for a second

    if (x<3) {
      x++;
      digitalWrite(CTRL_MUX, !digitalRead(CTRL_MUX));
    }

  }


  if (Serial3.available()) {
    // get the new byte:
    inChar = (char)Serial3.read();   
  }
  else inChar = 0;

  if(inChar == 'a') {
    ENA0_ON();
    Serial3.println("ENA0 on");
    Serial3.flush();
  }
  else if (inChar == 's') {
    ENA0_OFF();
    Serial3.println("ENA0 off");
    Serial3.flush();
  }
  else if (inChar == 'q') {
    DIR_ON();
    Serial3.println("DIR on");
    Serial3.flush();
  }
  else if (inChar == 'w') {
    DIR_OFF();
    Serial3.println("DIR off");
    Serial3.flush();
  }

  delay(50);

}
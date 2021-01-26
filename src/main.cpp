#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include "motor.h"
#include "terminal.h"
#include "communication.h"


//using Matthias Hertel driver https://github.com/mathertel/LiquidCrystal_PCF8574
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display




unsigned char target_position_mirror[ACTUATOR_COUNT];


void setup() {

   // status led
  pinMode(LED, OUTPUT);

  // terminal for debug
  Terminal_Init();

  // serial communication to ESP8266
  communication_setup();

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

  // do after call appsetup();
  // for(int x = 0; x<ACTUATOR_COUNT; x++)
  // {
  //     target_position_mirror[x] = myvalves[x].target_position;
  // }


}

void loop() {
  static int x = 0;
  
  //unsigned char len = 0;

  
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

    //Serial.println("help");
    //Serial.println("gactp 0 14 ");

    Serial.println("STMalive");   // send alive to ESP8266

  }

  // 100 ms loop
  if (millis() > (uint32_t) 100 + loop_100ms ) {
    loop_100ms = millis();  

    // if application is idle search for new tasks
    if(appgetstate() == A_IDLE) {

      // check all valves
      for(int x=0; x<ACTUATOR_COUNT; x++)
      {
          // handle first found difference then break
          //if(target_position_mirror[x] != myvalves[x].target_position)
          if(myvalves[x].actual_position != myvalves[x].target_position)
          {
              Serial3.print("M: target pos changed for valve "); Serial3.println(x, 10);
              
              // check if valve was learned before
              if(myvalves[x].status == VLV_STATE_UNKNOWN) 
              {
                Serial3.print("M: learning started for valve "); Serial3.println(x, 10);
                appsetaction(CMD_A_LEARN,x,0);    
                //myvalves[x].status = VLV_STATE_IDLE;   // for test
              }
              else // valve was learned before
              {
                // should valve be opened
                //if(myvalves[x].target_position > target_position_mirror[x]) {
                if(myvalves[x].target_position > myvalves[x].actual_position) {
                  if(myvalves[x].target_position == 100) appsetaction(CMD_A_OPEN_END,x,0);
                  else appsetaction(CMD_A_OPEN,x,myvalves[x].target_position-myvalves[x].actual_position);
                }
                // valve should be closed
                else {
                  if(myvalves[x].target_position == 0) appsetaction(CMD_A_CLOSE_END,x,0);
                  else appsetaction(CMD_A_CLOSE,x,myvalves[x].actual_position-myvalves[x].target_position);
                }
              }
          }
      }
      
      // update target position mirror for next cycle
      // for(int x = 0; x<ACTUATOR_COUNT; x++)
      // {
      //     target_position_mirror[x] = myvalves[x].target_position;
      // }

    }

    recvcmd = Terminal_Serve();

  }


  // 10 ms loop
  if (millis() > (uint32_t) 10 + loop_10ms ) {    
    loop_10ms = millis();  
    
    appcycle();
    
    communication_loop();

    // while(Serial.available()>0) {
    //   int testchar;
    //   testchar = Serial.read();
    //   //Serial.write(testchar);    // echo for test
    //   Serial3.write(testchar);
    // }

  }
}


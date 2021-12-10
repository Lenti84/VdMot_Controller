#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>
//#include <LiquidCrystal_PCF8574.h>
#include "motor.h"
#include "terminal.h"
#include "communication.h"
#include "temperature.h"
#include "eeprom.h"
#include "mycan.h"
#include "rs485.h"

//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging

//using Matthias Hertel driver https://github.com/mathertel/LiquidCrystal_PCF8574
//LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// info
// changes in arduino core lib
// .platformio\packages\framework-arduinoststm32\cores\arduino/HardwareSerial.h:44:4:
// --> increased tx buffer size from default 64 to 1024


unsigned char target_position_mirror[ACTUATOR_COUNT];


void setup() {

  Wire.begin();
  Wire.setSDA(I2C_SDA_PIN); // 
  Wire.setSCL(I2C_SCL_PIN); // 

  // while(1){
  // Wire.beginTransmission(0x71);
  // Wire.write('v');
  // Wire.endTransmission();
  // delay(1000);
  // }

  // power enable for valves
  pinMode(POWER_ENA, OUTPUT_OPEN_DRAIN);
  digitalWrite(POWER_ENA, 1);   // high to disable valve PSU

  // status led
  pinMode(LED, OUTPUT);

  // button 
  pinMode(BUTTON, INPUT_PULLUP);

  // analog inputs
  analogReadResolution(12);
  pinMode(ANINCURRENT, INPUT_ANALOG);
  pinMode(ANINREFHALF, INPUT_ANALOG);

  // terminal for debug
  Terminal_Init();

  // serial communication to ESP32
  communication_setup();

  // LCD
  // lcd.begin(16,2);  
  // lcd.setBacklight(255);
  // lcd.setCursor(0, 0);
  // lcd.print("Menu 4.x LCD");
  // lcd.setCursor(0, 1);
  // lcd.print("r-site.net");
  delay(2000);

  // EEPROM
  eepromsetup();
  eeprom_read_layout (&eep_content);
 
  // setup onewire temperature sensor at DS2482
  temperature_setup();

  // valve app setup
  appsetup();

  // can
  //mycan_setup();          // can hardware testcode setup

  // rs485
  //rs485_setup();          // rs485 hardware testcode setup

  // just for testing
  //myvalves[0].sensorindex = 0;
}

void loop() {
  //static int x = 0;
  static int time10s = 0;

  //unsigned char len = 0;

  static uint32_t time = 0;
  static uint32_t loop_10ms = 0;
  static uint32_t loop_100ms = 0;

  static uint8_t buttontest = 0;

  int16_t recvcmd;
  

  if (millis() > (uint32_t) 200 + time ) {    
    time = millis();

    if(time10s>=10) {
      time10s = 0;
      app_10s_loop();
    }
    else time10s++;
    
    digitalWrite(LED, !digitalRead(LED));   // toggle LED
    
    // button test
    if (digitalRead(BUTTON) > 0 && buttontest == 0) 
    {
      buttontest = 1;
      COMM_DBG.println("Button pressed");
    }
    else buttontest = 0;
    //if (!digitalRead(BUTTON)) buttontest = 0;

    // analog test
    //COMM_DBG.print("A0 current ");
    //COMM_DBG.print(analogRead(ANINCURRENT),DEC);
    //COMM_DBG.println(" digits");


    //Serial.println("help");
    //Serial.println("gactp 0 14 ");

    Serial.println("STMalive ");   // send alive to ESP32   
    //Serial.println("help ");   // send alive to ESP32  
  }

  // 100 ms loop
  if (millis() > (uint32_t) 100 + loop_100ms ) {
    loop_100ms = millis();  

    // COMM_DBG.print(analogRead(ANINCURRENT));
    // COMM_DBG.print(" ");
    // COMM_DBG.print(analogRead(ANINREFHALF));
    // COMM_DBG.println("");

    // if application is idle search for new tasks
    if(appgetstate() == A_IDLE) {

      // check all valves
      for(unsigned int x=0; x<ACTUATOR_COUNT; x++)
      {
          // handle first found difference then break
          //if(target_position_mirror[x] != myvalves[x].target_position)
          if(myvalves[x].actual_position != myvalves[x].target_position)
          {
              COMM_DBG.print("M: target pos changed for valve "); COMM_DBG.println(x, 10);
              
              // check if valve was learned before
              if(myvalves[x].status == VLV_STATE_UNKNOWN || myvalves[x].status == VLV_STATE_OPENCIR) 
              {
                COMM_DBG.print("M: learning started for valve "); COMM_DBG.println(x, 10);
                appsetaction(CMD_A_LEARN,x,0);    
                //myvalves[x].status = VLV_STATE_IDLE;   // for test
              }
              // check if valve is not open circuit
              //else if(myvalves[x].status == VLV_STATE_OPENCIR) 
              //{
                //COMM_DBG.print("M: valve "); COMM_DBG.print(x, 10); COMM_DBG.println(" open circuit");
                //myvalves[x].target_position = myvalves[x].actual_position;
              //}
              else // valve was learned before
              {
                // should valve be opened
                //if(myvalves[x].target_position > target_position_mirror[x]) {
                if(myvalves[x].target_position > myvalves[x].actual_position) {
                  if(myvalves[x].target_position == 100) appsetaction(CMD_A_OPEN_END,x,(byte)0);
                  else appsetaction(CMD_A_OPEN,x,myvalves[x].target_position-myvalves[x].actual_position);
                }
                // valve should be closed
                else {
                  if(myvalves[x].target_position == 0) appsetaction(CMD_A_CLOSE_END,x,(byte)0);
                  else appsetaction(CMD_A_CLOSE,x,myvalves[x].actual_position-myvalves[x].target_position);
                }
              }
          }
      }
    }

    recvcmd = Terminal_Serve();
  }


  // 10 ms loop
  if (millis() > (uint32_t) 10 + loop_10ms ) {    
    loop_10ms = millis();  
    
    appcycle();   
    
    communication_loop();

    temperature_loop();

    eepromloop();

    //mycan_loop();         // can hardware testcode

    //rs485_loop();         // rs485 hardware testcode

    // while(Serial.available()>0) {
    //   int testchar;
    //   testchar = Serial.read();
    //   //Serial.write(testchar);    // echo for test
    //   COMM_DBG.write(testchar);
    // }

  }
}

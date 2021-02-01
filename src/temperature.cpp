#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>
//#include "..\lib\DS2482_OneWire\OneWire.h"
//#include "..\lib\Arduino-Temperature-Control-Library\DallasTemperature.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "temperature.h"


int tempsensors[SENSORCOUNT];

OneWire oneWire;
DallasTemperature sensors(&oneWire);


void printAddress(DeviceAddress deviceAddress)
{
  Serial3.print("{");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    //Serial3.print("0x");
    Serial3.print(" ");
    if (deviceAddress[i] < 16) Serial3.print("0");
    Serial3.print(deviceAddress[i], HEX);
    if (i<7) Serial.print(", ");
    
  }
  Serial3.print(" }");  
}


void temperature_setup() {

    sensors.begin();
  
    sensors.setWaitForConversion(false);

    DeviceAddress currAddress;
    uint8_t numberOfDevices = sensors.getDeviceCount();
    
    for (unsigned int i=0; i<numberOfDevices; i++)
    {
        sensors.getAddress(currAddress, i);
        printAddress(currAddress);
        Serial3.println();  
    }

    // init sensor array
    for (unsigned int i = 0; i<SENSORCOUNT; i++) {
      tempsensors[i] = -500;
    }

}



void temperature_loop() {
    #define T_INIT      0
    #define T_IDLE      1
    #define T_REQUEST   2
    #define T_OUTPUT    3
    #define T_WAIT      4
    
    
    static int tempstate = T_INIT;

    static uint8_t numberOfDevices;
    static unsigned int timer = 0;

    DeviceAddress currAddress;
    int temp;


  switch (tempstate) {
    case T_INIT:  
              numberOfDevices = sensors.getDeviceCount();
              Serial3.print("Found "); Serial3.print(numberOfDevices, 10); Serial3.println(" temp sensors");
              tempstate = T_IDLE;
              break;

    case T_IDLE:  
              timer = CONV_INTERVALL;
              tempstate = T_REQUEST;
              break;

    case T_REQUEST:
              if(timer) timer--;
              else {
                Serial3.println("Requesting temperatures...");
                sensors.requestTemperatures();

                timer = 20 + (sensors.millisToWaitForConversion(sensors.getResolution()) / 10);
                tempstate = T_WAIT;
              }
              break;

    case T_WAIT:
              if(timer) timer--;
              else tempstate = T_OUTPUT;
              break;
              
    case T_OUTPUT:  
              for (int i=0; i<numberOfDevices; i++)
              {
                temp = round(sensors.getTempCByIndex(i)*10);
                tempsensors[i] = temp;
                sensors.getTempCByIndex(i);
                sensors.getAddress(currAddress, i);
                printAddress(currAddress);
                Serial3.print(": ");
                Serial3.print(temp/10);
                Serial3.print(".");
                Serial3.print(temp%10);
                Serial3.println();  
              }

              tempstate = T_IDLE;
              break;

    default:  tempstate = T_IDLE;
              break;
  }


}
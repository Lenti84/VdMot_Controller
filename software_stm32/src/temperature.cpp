#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>            // library https://github.com/bblanchon/ArduinoJson
#include "temperature.h"


tempsensor  tempsensors[MAXSENSORCOUNT];
uint8_t     numberOfDevices;

//OneWire     oneWire;
OneWire     oneWire(ONEW_PIN);

DallasTemperature sensors(&oneWire);


//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging


void printAddress(DeviceAddress deviceAddress)
{
  COMM_DBG.print("{");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    //COMM_DBG.print("0x");
    COMM_DBG.print(" ");
    if (deviceAddress[i] < 16) COMM_DBG.print("0");
    COMM_DBG.print(deviceAddress[i], HEX);
    if (i<7) COMM_DBG.print(", ");
    
  }
  COMM_DBG.print(" }");  
}


void temperature_setup() {

    COMM_DBG.print("starting 1-wire setup"); 

    sensors.begin();
  
    sensors.setWaitForConversion(false);

    COMM_DBG.print("search for devices"); 

    //DeviceAddress currAddress;
    //uint8_t numberOfDevices = sensors.getDeviceCount();
    numberOfDevices = sensors.getDeviceCount();
    COMM_DBG.print("Found "); COMM_DBG.print(numberOfDevices, 10); COMM_DBG.println(" temp sensors:");
    
    for (unsigned int i=0; i<numberOfDevices; i++)
    {
        sensors.getAddress(tempsensors[i].address, i);
        //tempsensors[i].address = currAddress;
        printAddress(tempsensors[i].address);
        COMM_DBG.println();
    }

    // init sensor array
    for (unsigned int i = 0; i<MAXSENSORCOUNT; i++) {
      tempsensors[i].temperature = -500;
    }

}



void temperature_loop() {
    #define T_INIT      0
    #define T_IDLE      1
    #define T_REQUEST   2
    #define T_OUTPUT    3
    #define T_WAIT      4
    
    
    static int tempstate = T_INIT;

    //static uint8_t numberOfDevices;
    static unsigned int timer = 0;

    DeviceAddress currAddress;
    int temp;


  switch (tempstate) {
    case T_INIT:  
              //numberOfDevices = sensors.getDeviceCount();
              //COMM_DBG.print("Found "); COMM_DBG.print(numberOfDevices, 10); COMM_DBG.println(" temp sensors");
              timer = CONV_INTERVALL / 10;                
              tempstate = T_IDLE;
              break;

    case T_IDLE:                  
              tempstate = T_REQUEST;
              #warning fixme
              break;

    case T_REQUEST:
              if(timer) timer--;
              else {
                COMM_DBG.println("Requesting temperatures...");
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
                tempsensors[i].temperature = temp;
                                
                sensors.getTempCByIndex(i);                
                //sensors.getAddress(currAddress, i);
                //printAddress(currAddress);
                COMM_DBG.print("Sensor ");
                COMM_DBG.print(i);
                COMM_DBG.print(": ");
                COMM_DBG.print(temp/10);
                COMM_DBG.print(".");
                COMM_DBG.print(temp%10);
                COMM_DBG.println();  
              }

              //get_sensordata();

              tempstate = T_IDLE;
              break;

    default:  tempstate = T_IDLE;
              break;
  }

}


void get_sensordata (char *buffer, int buflen) {

  DynamicJsonDocument doc(1024);      // buffer size should be enough for about 22 sensors
  String testjson;
  String AddressStr;
 
  doc["cnt"] = numberOfDevices;

  for (int i=0; i<(int)numberOfDevices; i++)
  {     
    doc["sns"][i]["temp"] = tempsensors[i].temperature;

    AddressStr = "";
    for (uint8_t x = 0; x < 8; x++)
    {
      if (tempsensors[i].address[x] < 16) AddressStr += '0';
      AddressStr += String(tempsensors[i].address[x], HEX);
      if (x<7) AddressStr += ' ';
    }
    doc["sns"][i]["add"] = AddressStr;
  }

  serializeJson(doc, buffer, buflen);
}



// void get_sensordata (char *buffer, int buflen) {

//   DynamicJsonDocument doc(1024);
//   String testjson;
//   uint8_t numberOfDevices;
//   DeviceAddress currAddress;
//   float ttemp;
//   String AddressStr;

//   numberOfDevices = sensors.getDeviceCount();
 
//   doc["cnt"] = numberOfDevices;

//   for (int i=0; i<(int)numberOfDevices; i++)
//   { 
//     ttemp = sensors.getTempCByIndex(i);
//     doc["sns"][i]["temp"] = ttemp;

//     sensors.getAddress(currAddress, i);
//     AddressStr = "";
//     for (uint8_t x = 0; x < 8; x++)
//     {
//       if (currAddress[x] < 16) AddressStr += '0';
//       AddressStr += String(currAddress[x], HEX);
//       if (x<7) AddressStr += ' ';
//     }
//     doc["sns"][i]["add"] = AddressStr;
//   }

//   //serializeJson(doc, testjson);  
//   //COMM_DBG.print(testjson);

//   serializeJson(doc, buffer, buflen);
// }
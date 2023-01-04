/**HEADER*******************************************************************
  project : VdMot Controller
  author : Lenti84
  Comments:
  Version :
  Modifcations :
***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE DEVELOPER OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller
*END************************************************************************/

#include <Arduino.h>
#include "hardware.h"
#include <Wire.h>
#include "..\lib\OneWire\OneWire.cpp"
#include <DallasTemperature.h>
#include <ArduinoJson.h>            // library https://github.com/bblanchon/ArduinoJson
#include "temperature.h"


tempsensor  tempsensors[MAXONEWIRECNT];
uint8_t     numberOfDevices;

//OneWire     oneWire;
OneWire     oneWire(ONEW_PIN);

DallasTemperature sensors(&oneWire);

enum t_state {T_INIT, T_IDLE, T_REQUEST, T_OUTPUT, T_WAIT, T_SEARCH};

volatile int temp_cmd = 0;
volatile int lock = 0;

//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging


void printAddress(DeviceAddress deviceAddress)
{
  #if defined tempDebug || defined appDebug
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
  #endif
}


void temperature_setup() {
    #ifdef tempDebug
      COMM_DBG.println("starting 1-wire setup"); 
    #endif
    for (uint8_t i=0; i<MAXONEWIRECNT; i++)
    {
      memset (tempsensors[i].address,0x0,8);
    }

    sensors.begin();
  
    sensors.setWaitForConversion(false);
    #ifdef tempDebug
      COMM_DBG.println("search for devices"); 
    #endif
    //DeviceAddress currAddress;
    //uint8_t numberOfDevices = sensors.getDeviceCount();
    numberOfDevices = sensors.getDeviceCount();
    #ifdef tempDebug
      COMM_DBG.print("Found "); 
      COMM_DBG.print(numberOfDevices, 10); 
      COMM_DBG.println(" temp sensors:");
    #endif
    for (unsigned int i=0; i<numberOfDevices; i++)
    {
        sensors.getAddress(tempsensors[i].address, i);
        #ifdef tempDebug
          printAddress(tempsensors[i].address);       
          COMM_DBG.println();
        #endif
    }

    // init sensor array
    for (unsigned int i = 0; i<MAXONEWIRECNT; i++) {
      tempsensors[i].temperature = -500;
    }

}



void temperature_loop() {
    static enum t_state tempstate = T_INIT;
    static int substate = 0;
    static int devcnt;
    //static int lock = 0;
  
    //static uint8_t numberOfDevices;
    static unsigned int timer = 0;

    DeviceAddress currAddress;
    float temp;


  switch (tempstate) {
    case T_INIT:  
              //numberOfDevices = sensors.getDeviceCount();
              #ifdef tempDebug
                COMM_DBG.print("Found "); 
                COMM_DBG.print(numberOfDevices, 10); 
                COMM_DBG.println(" temp sensors");
              #endif
              timer = CONV_INTERVALL / 10;                
              tempstate = T_IDLE;
              break;

    case T_IDLE:    

              if(temp_cmd == TEMP_CMD_NEWSEARCH) {
                substate = 0;
                tempstate = T_SEARCH;
              }
              else {
                if (lock == 0) {
                  tempstate = T_REQUEST;
                } 
              }

              break;

    case T_REQUEST:
              #ifdef tempDebug
                COMM_DBG.println("Requesting temperatures...");
              #endif
              sensors.requestTemperatures();

              timer = 20 + (sensors.millisToWaitForConversion(sensors.getResolution()) / 10);
              tempstate = T_WAIT;

              break;

    case T_WAIT:
              if(timer) timer--;
              else {
                devcnt=0;
                tempstate = T_OUTPUT;
              }
              break;
              
    case T_OUTPUT:  
              if(devcnt < numberOfDevices) {
                temp = sensors.getTempCByIndex(devcnt);
                tempsensors[devcnt].temperature = round(temp*10);
                                
                //sensors.getTempCByIndex(devcnt);                
                //sensors.getAddress(currAddress, devcnt);
                //printAddress(currAddress);
                #ifdef tempDebug
                  COMM_DBG.print("Sensor ");
                  COMM_DBG.print(devcnt);
                  COMM_DBG.print(": ");
                  COMM_DBG.print(tempsensors[devcnt].temperature/10);
                  COMM_DBG.print(".");
                  COMM_DBG.print(tempsensors[devcnt].temperature%10);
                  COMM_DBG.println();  
                #endif
              }
              else tempstate = T_IDLE;
              devcnt++;

              //get_sensordata();

              break;

    case T_SEARCH:
              // start new search
              if (substate == 0) {
                #ifdef tempDebug
                  COMM_DBG.println("New 1-wire search");
                #endif
                sensors.begin();
                substate = 1;
              }
              // read device count
              else if (substate == 1) {
                numberOfDevices = sensors.getDeviceCount();
                #ifdef tempDebug
                  COMM_DBG.print("Found "); 
                  COMM_DBG.print(numberOfDevices, 10); 
                  COMM_DBG.println(" temp sensors:");
                #endif
                devcnt=0;
                substate = 2;
              }
              // get adress and values of all sensors
              else if (substate == 2) {
                if(devcnt < numberOfDevices) {
                  sensors.getAddress(tempsensors[devcnt].address, devcnt);
                  printAddress(tempsensors[devcnt].address);
                  #ifdef tempDebug
                    COMM_DBG.print(" ");
                  #endif
                  temp = sensors.getTempCByIndex(devcnt);
                  tempsensors[devcnt].temperature = round(temp*10);
                  #ifdef tempDebug
                    COMM_DBG.print(temp,DEC);
                    COMM_DBG.println();
                  #endif
                }
                else {
                  temp_cmd = TEMP_CMD_NONE;
                  tempstate = T_IDLE;
                }
                devcnt++;
              }
              
              break;

    default:  tempstate = T_IDLE;
              break;
  }

}


void get_sensordata (unsigned int index, char *buffer, int buflen) {

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

void temp_command(int command) {
  
  if (command == TEMP_CMD_LOCK) {
    lock = 1;
  }
  else if (command == TEMP_CMD_UNLOCK) {
    lock = 0;
  }
  else if (temp_cmd == TEMP_CMD_NONE) temp_cmd = command;

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
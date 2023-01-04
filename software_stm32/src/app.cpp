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
#include "app.h"
#include "hardware.h"
#include "motor.h"
#include "temperature.h"
#include "eeprom.h"


struct valve myvalves[ACTUATOR_COUNT];

//unsigned char target_position_mirror[ACTUATOR_COUNT];
unsigned int learning_time = LEARN_AFTER_TIME_DEFAULT;
unsigned int learning_movements = LEARN_AFTER_MOVEMENTS_DEFAULT;

unsigned int reset_request = 0;

int16_t app_setup (void) { 

  // init valve data
  for (unsigned int x = 0;x<ACTUATOR_COUNT;x++) {    
      myvalvemots[x].target_position = 50;
      myvalvemots[x].actual_position = 50;
      myvalvemots[x].status = VLV_STATE_UNKNOWN;
      myvalves[x].sensorindex1 = VALVE_SENSOR_UNKNOWN;        // marks that no slot is selected
      myvalves[x].sensorindex2 = VALVE_SENSOR_UNKNOWN;        // marks that no slot is selected
      myvalves[x].learn_movements = LEARN_AFTER_MOVEMENTS_DEFAULT;
      // distribute learn timing equaly over valve slots
      myvalves[x].learn_time = (unsigned int) (((long)LEARN_AFTER_TIME_DEFAULT * ((long)x+1)) / (long)ACTUATOR_COUNT);
  }

  // match sensor address from eeprom with found sensors and set index/slot to valve struct
  app_match_sensors();

  if ((eep_content.numberOfMovements>=50) && (eep_content.numberOfMovements<65535))
    learning_movements=eep_content.numberOfMovements;
  #ifdef appDebug
    COMM_DBG.print("learning_movements: "); 
    COMM_DBG.println(learning_movements, DEC);
  #endif
  return 0;
}

int16_t app_loop (void) {
  static byte firstchange = 0;
  static unsigned int lastvalve = 0;
  static unsigned int testvlvindex = 0;

  reset_check();

    // if valve machine is idle search for new tasks
    if(valve_getstate() == A_IDLE) 
    {
        // find unknown states and try to find out whats on with the valve        
        if(myvalvemots[testvlvindex].status == VLV_STATE_UNKNOWN) 
        {
          #ifdef appDebug
            COMM_DBG.print("App: valve "); COMM_DBG.print(testvlvindex, 10);
            COMM_DBG.println(" unknown, try to find out...");
          #endif
          appsetaction(CMD_A_TEST,testvlvindex,0);    
        }
        
        else
        {        
          // fully open valves if needed
          if(myvalvemots[lastvalve].status == VLV_STATE_FULLOPEN) {
            appsetaction(CMD_A_OPEN_END,lastvalve,(byte)0);        
          }

          // learn all present valves if any target change happened before
          // this keeps controller calm right after startup, otherwise controller would be busy for up to 12 valve learning times (10 min ?!)
          else if(firstchange > 0 && myvalvemots[lastvalve].status == VLV_STATE_PRESENT) {
            #ifdef appDebug
              COMM_DBG.print("App: learning started for valve "); 
              COMM_DBG.println(lastvalve, 10);
            #endif
            appsetaction(CMD_A_LEARN,lastvalve,0);        
          }

          // handle first found difference then break
          if(myvalvemots[lastvalve].actual_position != myvalvemots[lastvalve].target_position)
          {
              firstchange = 1;

              #ifdef appDebug
                COMM_DBG.print("App: target pos changed for valve "); 
                COMM_DBG.println(lastvalve, 10);
              #endif

              // check if valve was learned before              
              if(myvalvemots[lastvalve].status == VLV_STATE_PRESENT)             
              {
                #ifdef appDebug
                  COMM_DBG.print("App: learning started for valve "); 
                  COMM_DBG.println(lastvalve, 10);
                #endif
                appsetaction(CMD_A_LEARN,lastvalve,0);                  
              }
              else if(myvalvemots[lastvalve].status != VLV_STATE_BLOCKS)
              {
                // should valve be opened
                if(myvalvemots[lastvalve].target_position > myvalvemots[lastvalve].actual_position) {                  
                  if(myvalvemots[lastvalve].target_position == 100) appsetaction(CMD_A_OPEN_END,lastvalve,(byte)0);
                  else appsetaction(CMD_A_OPEN,lastvalve,myvalvemots[lastvalve].target_position-myvalvemots[lastvalve].actual_position);
                }
                // valve should be closed
                else {
                  if(myvalvemots[lastvalve].target_position == 0) appsetaction(CMD_A_CLOSE_END,lastvalve,(byte)0);
                  else appsetaction(CMD_A_CLOSE,lastvalve,myvalvemots[lastvalve].actual_position-myvalvemots[lastvalve].target_position);
                }
              }
              else {
                // do nothing and clear request
                myvalvemots[lastvalve].actual_position = myvalvemots[lastvalve].target_position;
              }
          }

          lastvalve++;
          if (lastvalve>=ACTUATOR_COUNT) lastvalve = 0;
        }

        testvlvindex += 2;
        // vary startindexes to always get the even and the odd valves in one flow
        // helps reducing relay rattle (only C1 revision)
        if (testvlvindex == ACTUATOR_COUNT) testvlvindex = 1;
        else if (testvlvindex >= ACTUATOR_COUNT + 1) testvlvindex = 0;

    }

return 0;
}





byte app_10s_loop () {

  unsigned int x = 0;

  // walk through valves and evaluate learning values

  // learning times
  if (learning_time > 0) {
    for (x=0; x< ACTUATOR_COUNT; x++) { 
      if(myvalves[x].learn_time <= 10) {
        myvalves[x].learn_time = learning_time;
        myvalvemots[x].status = VLV_STATE_PRESENT;     // mark state as unknown, next set target req will do a learning cycle
        #ifdef appDebug
          COMM_DBG.print("App: Valve "); 
          COMM_DBG.print(x, 10); 
          COMM_DBG.println(" will be learned soon");
        #endif
      }
      else myvalves[x].learn_time -= 10;    
    }
  }

  // learning movements
  if (learning_movements > 0) { 
    for (x=0; x< ACTUATOR_COUNT; x++) {  
      if(myvalves[x].learn_movements == 0) {
        myvalves[x].learn_movements = learning_movements;
        myvalvemots[x].status = VLV_STATE_PRESENT;     // mark state as unknown, net set target req will do a learning cycle
        #ifdef appDebug
          COMM_DBG.print("App: Valve "); 
          COMM_DBG.print(x, 10); 
          COMM_DBG.println(" will be learned soon");
        #endif
      }   
    } 
  }

return 0;
}


// sets learning movements
// after number of movements a learning cycle will be executed
int16_t app_set_learnmovements(uint16_t movements) {

  // update reload value
  learning_movements = movements;
    
  // update all valve memories 
  for (unsigned int x = 0;x<ACTUATOR_COUNT;x++) {          
    myvalves[x].learn_movements = learning_movements;
  }

  return 0;

}


// sets learning time
// after time seconds a learning cycle will be executed
int16_t app_set_learntime(uint32_t time) {
 
  // update reload value
  learning_time = time;
    
  // update all valve memories 
  for (unsigned int x = 0;x<ACTUATOR_COUNT;x++) {          
    // distribute learn timing equaly over valve slots
    myvalves[x].learn_time = (unsigned int) (((long)learning_time * ((long)x+1)) / (long)ACTUATOR_COUNT);
  }

  return 0;

}


// sets learning of valve 
// a learning cycle for valve will be executed
// if valve = 255, all valves will be learned
int16_t app_set_valvelearning(uint16_t valve) {

  if(valve < ACTUATOR_COUNT) {
    myvalvemots[valve].actual_position = 0;     // fake some position deviation
    myvalvemots[valve].status = VLV_STATE_UNKNOWN;
    return 0;
  }
  else if (valve == 255) {
    // update all valves
    for(unsigned int xx=0;xx<ACTUATOR_COUNT;xx++){
      myvalvemots[xx].actual_position = 0;      // fake some position deviation
      myvalvemots[xx].status = VLV_STATE_UNKNOWN;
    }
    return 0;
  }

  return -1;
}


// sets valve full open
// valve will be opened fully
// if valve = 255, all valves will be opened fully
int16_t app_set_valveopen(int16_t valve) {

  if(valve < ACTUATOR_COUNT) {
    myvalvemots[valve].target_position = 100;
		myvalvemots[valve].status = VLV_STATE_FULLOPEN;
    return 0;
  }
  else if (valve == 255) {
    // update all valves
    for(unsigned int xx=0;xx<ACTUATOR_COUNT;xx++){
      myvalvemots[xx].target_position = 100;
			myvalvemots[xx].status = VLV_STATE_FULLOPEN;
    }
    return 0;
  }

  return -1;
}


// match sensor address from eeprom with found sensors and set index/slot to valve struct
int16_t app_match_sensors() {
  
  uint8_t   numberOfDevices;
  DeviceAddress currAddress;
  uint8_t   found1, found2;
  uint8_t   valveindexlast;
  #ifdef appDebug
    COMM_DBG.println("Read 1-wire sensor addresses from eeprom");
  #endif
  numberOfDevices = sensors.getDeviceCount();  
    
  for (unsigned int owsensorindex=0; owsensorindex<numberOfDevices; owsensorindex++)
  {
      sensors.getAddress(currAddress, owsensorindex);
      //tempsensors[i].address = currAddress;

      //COMM_DBG.print("owsensorindex ");
      //COMM_DBG.println(owsensorindex, DEC);
      #ifdef appDebug
        printAddress(currAddress);
        //COMM_DBG.println("");
      #endif

      // first sensor of valve
      // step through all possible valves
      for (unsigned int valveindex1 = 0;valveindex1<ACTUATOR_COUNT;valveindex1++) {
        valveindexlast = valveindex1;

        //COMM_DBG.print("valveindex ");
        //COMM_DBG.println(valveindex, DEC);

        found1 = 0;
        if (eep_content.owsensors1[valveindex1].crc == currAddress[7]) 
        {
            found1++;
            for (unsigned z = 0;z<6;z++) {
                if (eep_content.owsensors1[valveindex1].romcode[z] == currAddress[1+z]) {
                    found1++;
                }
            }           
        }
        if (found1 == 7) 
        {          
          valveindex1 = ACTUATOR_COUNT; // end for loop
          break;
        }
      }
      if (found1 == 7)
      {
          #ifdef appDebug
            COMM_DBG.print(" found as 1st sensor at valve: ");
            // COMM_DBG.print(owsensorindex, DEC);
            COMM_DBG.println(valveindexlast, DEC);
          #endif
          myvalves[valveindexlast].sensorindex1 = owsensorindex;
      } else {
        myvalves[valveindexlast].sensorindex1 = VALVE_SENSOR_UNKNOWN;
      }
      // else 
      // {
      //     COMM_DBG.println(" not found");
      //     //COMM_DBG.println(found, DEC);
      // }

      // second sensor of valve
      // step through all possible valves
      for (unsigned int valveindex = 0;valveindex<ACTUATOR_COUNT;valveindex++) {
        valveindexlast = valveindex;
         
        found2 = 0;
        if (eep_content.owsensors2[valveindex].crc == currAddress[7]) 
        {
            found2++;
            for (unsigned z = 0;z<6;z++) {
                if (eep_content.owsensors2[valveindex].romcode[z] == currAddress[1+z]) {
                    found2++;
                }
            }           
        }
        if (found2 == 7) 
        {          
          valveindex = ACTUATOR_COUNT; // end for loop
          break;
        }
      }
      if (found2 == 7)
      {     
          #ifdef appDebug     
            COMM_DBG.print(" found as 2nd sensor at valve: ");
            // COMM_DBG.print(owsensorindex, DEC);
            COMM_DBG.println(valveindexlast, DEC);
          #endif
          myvalves[valveindexlast].sensorindex2 = owsensorindex;
      }  else {
        myvalves[valveindexlast].sensorindex2 = VALVE_SENSOR_UNKNOWN;
      }
      // else 
      // {
      //     COMM_DBG.println(" not found");
      //     //COMM_DBG.println(found, DEC);
      // }

      if(found1==0 && found2==0) {
        #ifdef appDebug
          COMM_DBG.println(" not found");
        #endif
      }

      
      // else 
      // {
      //     COMM_DBG.print("not found ");
      //     COMM_DBG.print(owsensorindex, DEC);
      //     COMM_DBG.println(valveindexlast, DEC);
      //     myvalvemots[valveindexlast].sensorindex1 = VALVE_SENSOR_UNKNOWN;
      // }
  }

  return 0;

}


// set soft reset request
void reset_STM32 () {
  reset_request = 1;
}


// soft reset STM32
// waits until eeprom is written completely
void reset_check () {
  if(reset_request && eeprom_free()) {
    #define AIRCR_VECTKEY_MASK    (0x05FA0000)    
      SCB->AIRCR = AIRCR_VECTKEY_MASK | 0x04;
    while(1);   
  }
}
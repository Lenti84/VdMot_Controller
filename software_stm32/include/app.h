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

#ifndef _APP_H
	#define _APP_H

#include <Arduino.h>
#include "hardware.h"
#include "motor.h"



#define LEARN_AFTER_MOVEMENTS_DEFAULT     100          // after x movements a learning cycle will be executed
#define LEARN_AFTER_TIME_DEFAULT          7*24*3600   // after x seconds a learning cycle will be executed


#define VALVE_INIT_TEMPERATURE            -2000   // init value for struct value of temperature
#define VALVE_SENSOR_UNKNOWN              65535   // marks that no sensor slot is selected


int16_t app_setup (void);
int16_t app_loop (void);
byte app_10s_loop ();
int16_t app_set_learnmovements(uint16_t cycles);
int16_t app_set_learntime(uint32_t time);
int16_t app_set_valvelearning(uint16_t valve);
int16_t app_set_valveopen(int16_t valve);
int16_t app_match_sensors();
void reset_check();
void reset_STM32();

// struct valvemotor {
// //typedef struct valves {
//   unsigned int closing_count;  
//   unsigned int opening_count;
//   unsigned int deadzone_count;  
//   unsigned int scaler;
//   unsigned int meancurrent;
//   // unsigned int sensorindex1;
//   // unsigned int sensorindex2;
//   // unsigned int learn_time;
//   // unsigned int learn_movements;
//   byte target_position;
//   byte actual_position;
//   byte status;
// };



struct valve {
  unsigned int sensorindex1;
  unsigned int sensorindex2;
  unsigned int learn_time;
  unsigned int learn_movements;
//   byte target_position;
//   byte actual_position;
  byte statusm;
  //struct valvemotor valvemot;
};

extern struct valve myvalves[ACTUATOR_COUNT];
extern unsigned int learning_movements;

#endif //_APP_H



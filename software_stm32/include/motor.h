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

#ifndef _MOTOR_H
	#define _MOTOR_H

#include "app.h"

#define CMD_A_OPEN      'o'
#define CMD_A_OPEN_END  'p'
#define CMD_A_CLOSE     'c'
#define CMD_A_CLOSE_END 'v'
#define CMD_A_LEARN     'l'
#define CMD_A_TARGET    't'
#define CMD_A_TEST      'x'

#define VLV_STATE_IDLE      (byte) 0x01       // nothing to do
#define VLV_STATE_OPENING   (byte) 0x02       // opens
#define VLV_STATE_CLOSING   (byte) 0x03       // closes
#define VLV_STATE_BLOCKS    (byte) 0x04       // valve is blocked
#define VLV_STATE_UNKNOWN   (byte) 0x05       // initial state
#define VLV_STATE_OPENCIR   (byte) 0x06       // open circuit detected
#define VLV_STATE_FULLOPEN  (byte) 0x07       // go directly full open
#define VLV_STATE_PRESENT   (byte) 0x08       // connected


struct valvemotor {
//typedef struct valves {
  unsigned int closing_count;  
  unsigned int opening_count;
  unsigned int deadzone_count;  
  unsigned int scaler;
  unsigned int meancurrent;
  // unsigned int sensorindex1;
  // unsigned int sensorindex2;
  // unsigned int learn_time;
  // unsigned int learn_movements;
  byte target_position;
  byte actual_position;
  byte status;
};

extern valvemotor myvalvemots[ACTUATOR_COUNT];

enum ASTATE {
A_INIT, A_IDLE, A_CLOSE, A_OPEN1, A_OPEN2, A_LEARN1, 
A_LEARN2, A_LEARN3, A_LEARN4, A_CLOSE1, A_CLOSE2, A_TEST };

extern enum ASTATE valvestate;




byte valve_loop ();
//byte valve_setup (struct valve *valvedata);
//byte valve_setup (struct valvemotor *valvedata);
byte valve_setup ();

enum ASTATE valve_getstate ();
int16_t appsetaction(char cmd, unsigned int valveindex, byte pos);

extern uint8_t currentbound_low_fac;      // lower current limit factor for detection of end stop
extern uint8_t currentbound_high_fac;     // upper current limit factor for detection of end stop
extern uint8_t startOnPower;              // valve % on power start 

#endif     //_MOTOR_H
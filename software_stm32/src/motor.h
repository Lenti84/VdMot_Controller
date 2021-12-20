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

/*
 
*/
#ifndef _MOTOR_H
	#define _MOTOR_H


#define LEARN_AFTER_MOVEMENTS_DEFAULT     50      // after x movements a learning cycle will be executed
#define LEARN_AFTER_TIME_DEFAULT          3600    // after x seconds a learning cycle will be executed

#define ACTUATOR_COUNT     (unsigned int) 12      // how many valves are supported
#define VALVE_INIT_TEMPERATURE            -2000   // init value for struct value of temperature
#define VALVE_SENSOR_UNKNOWN              65535   // marks that no sensor slot is selected

#define CMD_A_OPEN      'o'
#define CMD_A_OPEN_END  'p'
#define CMD_A_CLOSE     'c'
#define CMD_A_CLOSE_END 'v'
#define CMD_A_LEARN     'l'
#define CMD_A_TARGET    't'

#define VLV_STATE_IDLE      (byte) 0x01
#define VLV_STATE_OPENING   (byte) 0x02
#define VLV_STATE_CLOSING   (byte) 0x03
#define VLV_STATE_BLOCKS    (byte) 0x04
#define VLV_STATE_UNKNOWN   (byte) 0x05
#define VLV_STATE_OPENCIR   (byte) 0x06


typedef struct {
//typedef struct valves {
  unsigned int closing_count;  
  unsigned int opening_count;
  unsigned int deadzone_count;  
  unsigned int scaler;
  unsigned int meancurrent;
  unsigned int sensorindex;
  unsigned int learn_time;
  unsigned int learn_movements;
  byte target_position;
  byte actual_position;
  byte status;
} valves;

extern valves myvalves[ACTUATOR_COUNT];

enum ASTATE {
A_INIT, A_IDLE, A_CLOSE, A_OPEN1, A_OPEN2, A_LEARN1, 
A_LEARN2, A_LEARN3, A_LEARN4, A_CLOSE1, A_CLOSE2 };

extern enum ASTATE appstate;




byte appcycle ();
byte appsetup ();
byte app_10s_loop ();
enum ASTATE appgetstate ();
int16_t appsetaction(char cmd, unsigned int valveindex, byte pos);


#endif     //_MOTOR_H
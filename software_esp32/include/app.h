/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano, Lenti84

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




#ifndef _APP_H_
#define _APP_H_

#include <Syslog.h>

typedef struct  {
	  unsigned char actual_position;      // from controller
    unsigned char target_position;      // to controller
    unsigned char state;                // from controller
    unsigned int  meancurrent;          // from controller
    int           temperature;          // temperature of assigned sensor
} actuator_str;


#define APP_CMD_NONE               0x00
#define APP_CMD_SETTARGETPOS       0x01            // ESP -> STM - set target value
#define APP_CMD_GETACTUALPOS       0x02            // ESP <- STM - get actual value

#define APP_PRE_SETTARGETPOS       	"stgtp"
#define APP_PRE_GETACTUALPOS       	"gactp"
#define APP_PRE_GETMEANCURR        	"gmenc"
#define APP_PRE_GETSTATUS          	"gstat"
#define APP_PRE_GETVLVDATA			"gvlvd"



#define COMM_ALIVE_CYCLE            30          // send "Alive" cycle in 100 ms cycles

#define MAINTOPIC_LEN               30


void app_setup();
void app_loop();
void app_cmd(int command);

extern Syslog syslog;

extern char mqtt_maintopic[];
extern actuator_str actuators[];

extern uint8_t     stm32alive;


#endif // #ifndef 
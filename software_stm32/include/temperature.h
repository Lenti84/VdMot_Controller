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

#ifndef _TEMPERATURE_H
	#define _TEMPERATURE_H

#include <Arduino.h>
#include <DallasTemperature.h>


extern void temperature_setup();
extern void temperature_loop();
extern void get_sensordata (unsigned int index, char *buffer, int buflen);
extern void temp_command(int command);

void printAddress(DeviceAddress deviceAddress);

//#define MAXSENSORCOUNT 		(unsigned int)	34
#define CONV_INTERVALL		(unsigned int)	2000			// conversion intervall in ms

#define TEMP_CMD_NONE         0x00
#define TEMP_CMD_NEWSEARCH    0x01
#define TEMP_CMD_LOCK         0x02
#define TEMP_CMD_UNLOCK       0x03

typedef struct {
  int 			temperature;
  DeviceAddress address;  
} tempsensor;

extern tempsensor tempsensors[];
extern uint8_t numberOfDevices;

extern DallasTemperature sensors;


#endif //_TEMPERATURE_H


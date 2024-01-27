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

#include <Arduino.h>
#include <ExecWithParameter.h>
#include "globals.h"
#include "VdmTask.h"

#define piControlManual     0
#define piControlOnHeating  1
#define piControlOnCooling  2
#define piControlOff        3

#define piControlAllowedHeatingCooling 0
#define piControlAllowedHeating 1
#define piControlAllowedCooling 2

#define windowActionCloseRestore  0
#define windowActionOpenLock      1
#define windowActionOpen          2
#define windowActionClose         3


enum CHECKACTION {nothing,gotoMin,gotoPark,control};
enum WINDOWSTATE {windowIdle,windowOpenLock,windowCloseRestore};

class CPiControl: public Executable
{
public:
  CPiControl() {
    valveIndex=255;
    start=false;
    dynOffset=0;
    scheme=0;
    startActiveZone=0;
    endActiveZone=100;
    windowOpenTarget=0;
    windowControlState=windowIdle;
    windowState=0;
    controlActive=true;
    failed=false;
    esum = 0;
  };
  void exec() override {
    controlValve();
	}
  float piCtrl(float target,float value);
  void updateControllerGains(float observedError);
  uint8_t calcValve();
  void controlValve();
  void setWindowAction(uint8_t windowControl);
  void setValveAction(uint8_t valvePosition);
  void setPosition(uint8_t thisPosition);
  void setControlActive(uint8_t thisControl);
  void setFailed(uint8_t thisPosition);
  bool getValueFromSource();
  uint32_t ta;
  uint32_t ti;
  volatile uint16_t xp;
  float kp;
  volatile float ki;
  volatile int8_t offset;
  volatile int8_t dynOffset;
  volatile float target;
  volatile float value; 
  uint8_t valveIndex;
  uint8_t scheme;
  uint8_t startActiveZone;
  uint8_t endActiveZone;
  uint8_t startValvePos;
  uint8_t windowState;
  WINDOWSTATE windowControlState;
  uint8_t  windowOpenTarget;
  bool controlActive;
  bool failed;
  float esum;
private:
  void doControlValve(); 
  CHECKACTION checkAction(uint8_t idx);
  
  float iProp;
  time_t ts;
  bool start;
  
  uint8_t lastControlPosition;
};

extern CPiControl PiControl[ACTUATOR_COUNT];

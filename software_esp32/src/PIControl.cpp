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


#include "PIControl.h"
#include "time.h"
#include "globals.h"
#include "stmApp.h"
#include "VdmConfig.h"
#include "VdmNet.h"
#include <Syslog.h>


CPiControl PiControl[ACTUATOR_COUNT];

float CPiControl::piCtrl(float e) {
  float p;
  float y;
  float kp;
  if (xp==0) return (50);
  kp = 100/xp;
  time_t now;
  if (!start) {
    time(&ts);
    iProp=0;
    start=true;
    esum=0;
  }
  time(&now);
  ta=difftime(now,ts);      // in sec
  time(&ts);

  // p - proportion
  y = kp * e;
  p = y + 50 + offset + dynOffset;
  if (p > 100.0) p = 100.0;
  if (p < 0.0) p = 0.0;
  if (ti==0) return (50);
  if (scheme==0) {
    // i - proportion
    iProp += ((float)ta / (float)ti) * y;
  } else {
    esum += e;
    iProp = ki * ((float)ta / (float)ti) * esum;
  }

  // pi
  y = p + iProp;
  if (y > 100.0) {
    y = 100.0;
    iProp = 100.0 - p;
  }
  if (y < 0.0) {
    y = 0.0;
    iProp = 0.0 - p;
  }
  
  return y;
}

uint8_t CPiControl::calcValve()
{
    float piValue=piCtrl(target-value);
    return (round(piValue));
}

void CPiControl::controlValve() 
{
  if (VdmConfig.configFlash.valvesControlConfig.heatControl==piControlOff) {
     StmApp.actuators[valveIndex].target_position=VdmConfig.configFlash.valvesControlConfig.parkingPosition;
     return;
  }
  
  if (VdmConfig.configFlash.valvesControlConfig.heatControl==piControlOn) {
    uint8_t valvePosition;
    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].valueSource==1) {
      value=((float)StmApp.actuators[valveIndex].temp1)/10;
    }
    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].valueSource==2) {
      value=((float)StmApp.actuators[valveIndex].temp2)/10;
    }
    float newValveValue=calcValve();
    if (endActiveZone>startActiveZone) {
      valvePosition=round((((float)(endActiveZone-startActiveZone))*newValveValue/100)+startActiveZone); 
    } else valvePosition=round(newValveValue);
    if (valvePosition>100) {
      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
        syslog.log(LOG_DEBUG, "pic:valve position exceeds 100 : (#"+String(valveIndex+1)+") = "+String(valvePosition));
      }
      valvePosition=100;
    }

    StmApp.actuators[valveIndex].target_position=valvePosition;
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
      syslog.log(LOG_DEBUG, "pic:valve position has changed : "+String(VdmConfig.configFlash.valvesConfig.valveConfig[valveIndex].name)+"(#"+String(valveIndex+1)+") = "+String(valvePosition)+" Ttarget "+String(target)+" Tvalue "+String(value));
    }
    // check if there are links
    for (uint8_t i=0; i< ACTUATOR_COUNT; i++) {
      if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].active) {
        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].link==(valveIndex+1)) {
          StmApp.actuators[i].target_position=valvePosition;
          if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
            syslog.log(LOG_DEBUG, "pic:link valve position has changed : "+String(VdmConfig.configFlash.valvesConfig.valveConfig[i].name)+"(#"+String(i+1)+") = "+String(valvePosition));
          }
        }
      }
    }
  }
}
  
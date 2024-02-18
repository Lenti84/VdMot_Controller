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


float CPiControl::piCtrl(float target,float value) {
  float p;
  float y;
  float e;
  
  if ((xp==0) || (ti==0)) return (startValvePos);
 
  time_t now;
  if (!start) {
    time(&ts);
    iProp=startValvePos-50;
    start=true;
    esum=0;
  }
  time(&now);
  ta=difftime(now,ts);      // in sec
  time(&ts);
 
  if (windowControlState == windowCloseRestore) {
    ta = VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].ts;
    windowControlState = windowIdle;
  }
  
  if (VdmConfig.heatValues.heatControl==piControlOnHeating)
  {
    e=target-value;
  } else {
    e=value-target;
  } 

  // p - proportion
  y = kp * e;
  p = y + 50 + offset + dynOffset;
  if (p > 100.0) p = 100.0;
  if (p < 0.0) p = 0.0;
  

  if (scheme==0) {
    // i - proportion
    iProp += ((float)ta / (float)ti) * y;
  } else {
    esum += e;
    iProp = ki * ((float)ta / (float)ti) * esum;
  }

  // pi
  y = p + iProp;
  if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "pic:calc valve position : #"+String(valveIndex+1)+" ("+String(VdmConfig.configFlash.valvesConfig.valveConfig[valveIndex].name)+") tTarget = "+String(target)+" tValue = "+String(value)+" diff = "+String(e)+" iProp = "+String(iProp)+" kp = "+String(kp)+" esum = "+String(esum)+" p = "+String(p)+" y = "+String(y));
  }
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

void CPiControl::setWindowAction(uint8_t windowControl)
{
 if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].controlFlags.windowInstalled && controlActive && 
    (VdmConfig.heatValues.heatControl==1 || VdmConfig.heatValues.heatControl==2))
 {
  windowState = windowControl;
  switch (windowControl) {
    case windowActionCloseRestore: {
      windowControlState = windowCloseRestore;
      setValveAction(lastControlPosition);
      break;
    }
    case windowActionOpenLock: {
      windowControlState = windowOpenLock;
      setValveAction(windowOpenTarget);
      break;
    }
  }
 } else {
  if (windowControlState == windowOpenLock) {
    windowControlState = windowCloseRestore;
    setValveAction(lastControlPosition);
  }
 }
}

void CPiControl::setValveAction(uint8_t valvePosition) {
  if (!(failed) && (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].controlFlags.active && controlActive)) {
    StmApp.actuators[valveIndex].target_position=valvePosition;
    if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
      syslog.log(LOG_DEBUG, "pic: setValveAction valve position has changed : "+String(VdmConfig.configFlash.valvesConfig.valveConfig[valveIndex].name)+"(#"+String(valveIndex+1)+") = "+String(valvePosition)+" Ttarget "+String(target)+" Tvalue "+String(value));
    }
    // check if there are links
    for (uint8_t i=0; i< ACTUATOR_COUNT; i++) {
      if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].controlFlags.active) {
        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].link==(valveIndex+1)) {
          StmApp.actuators[i].target_position=valvePosition;
          if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_ATOMIC) {
            syslog.log(LOG_DEBUG, "pic:setValveAction link valve position has changed : "+String(VdmConfig.configFlash.valvesConfig.valveConfig[i].name)+"(#"+String(i+1)+") = "+String(valvePosition));
          }
        }
      }
    }
  }
}


CHECKACTION CPiControl::checkAction(uint8_t idx)
{
  if (!controlActive) return(nothing);
  if (!VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].controlFlags.active)  return(nothing);
  if (failed) return(nothing);

  if (!VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].controlFlags.windowInstalled) {
    windowControlState = windowIdle;  
  }
  if (windowControlState == windowOpenLock) {
    setPosition(windowOpenTarget);
    return(nothing);
  }
  
  switch (VdmConfig.heatValues.heatControl)
  {
    case piControlManual:
      return(nothing);
    case piControlOnHeating:
      if ((VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.allow==piControlAllowedHeatingCooling) || (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.allow==piControlAllowedHeating))
      {
        return(control); 
      }
      else {
        return(gotoMin);  
      }
    case piControlOnCooling:
      if ((VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.allow==piControlAllowedHeatingCooling) || (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[idx].controlFlags.allow==piControlAllowedCooling))
      {
        return(control); 
      }
      else {
        return(gotoMin);  
      }
    case piControlOff:
      return (gotoPark);
  } 
  return (nothing);
}


uint8_t CPiControl::calcValve()
{
  float piValue=0;
  piValue=piCtrl(target,value);
  return (round(piValue));
}

void CPiControl::controlValve() 
{
  CHECKACTION check=checkAction(valveIndex);
 
  switch (check) {
    case nothing:
      break;
    case gotoMin:
      setPosition(VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].startActiveZone);
      break;
    case gotoPark:
      setPosition(VdmConfig.heatValues.parkPosition);
      break;
    case control:
      doControlValve();
      break;
  }
}

void CPiControl::setPosition(uint8_t thisPosition)
{
  StmApp.actuators[valveIndex].target_position=thisPosition;
     // check if there are links
     for (uint8_t i=0; i< ACTUATOR_COUNT; i++) {
      if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].controlFlags.active) {
        if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].link==(valveIndex+1)) {
          StmApp.actuators[i].target_position=thisPosition;
          if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
            syslog.log(LOG_DEBUG, "pic:link valve position has changed to : "+String(VdmConfig.configFlash.valvesConfig.valveConfig[i].name)+"(#"+String(i+1)+") = "+String(thisPosition));
          }
        }
      }
    }
}
  
bool CPiControl::getValueFromSource()
{
  if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].valueSource==1) {
      if (StmApp.actuators[valveIndex].temp1>-500) 
        value=((float)StmApp.actuators[valveIndex].temp1)/10;
      else return (false);
    }
    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[valveIndex].valueSource==2) {
      if (StmApp.actuators[valveIndex].temp2>-500) 
        value=((float)StmApp.actuators[valveIndex].temp2)/10;
      else return (false);
    }
    return (true);
}

void CPiControl::doControlValve() 
{
    uint8_t valvePosition;
    if (!getValueFromSource()) return;
    float newValveValue=calcValve();
    if (endActiveZone>startActiveZone) {
      valvePosition=round((((float)(endActiveZone-startActiveZone))*newValveValue/100.0)+startActiveZone); 
    } else valvePosition=round(newValveValue);
    if (valvePosition>100) {
      if (VdmConfig.configFlash.netConfig.syslogLevel>=VISMODE_DETAIL) {
        syslog.log(LOG_DEBUG, "pic:valve position exceeds 100 : (#"+String(valveIndex+1)+") = "+String(valvePosition));
      }
      valvePosition=100;
    }
    if (controlActive==0) valvePosition=0; 
    setPosition(valvePosition);
    lastControlPosition=valvePosition;
}
  
void CPiControl::setControlActive(uint8_t thisControl)
{
  controlActive=thisControl;
  // check if there are links
   for (uint8_t i=0; i< ACTUATOR_COUNT; i++) {
    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].controlFlags.active) {
      if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].link==(valveIndex+1)) {
        PiControl[i].controlActive=thisControl;
      }
    }
  }
}

void CPiControl::setFailed(uint8_t thisPosition)
{
  failed=true;
  StmApp.actuators[valveIndex].target_position=thisPosition;
  // check if there are links
  for (uint8_t i=0; i< ACTUATOR_COUNT; i++) {
    if (VdmConfig.configFlash.valvesControlConfig.valveControlConfig[i].link==(valveIndex+1)) {
      StmApp.actuators[i].target_position=thisPosition;
    }
  }
}
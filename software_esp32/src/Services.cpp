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


#include "Services.h"
#include "globals.h"
#include "VdmTask.h"
#include "VdmConfig.h"
#include "VdmSystem.h"
#include "stmApp.h"
#include "Queue.h"

CServices Services;

CServices::CServices()
{
  serviceValvesStarted=false;
  restartSTM=false;
}

void CServices::checkServiceValves()
{
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  uint8_t i = 1<<timeinfo.tm_wday;

  if ((VdmConfig.configFlash.valvesConfig.dayOfCalib & i) !=0) { // day fits
    if (VdmConfig.configFlash.valvesConfig.hourOfCalib==timeinfo.tm_hour) { // hour fits
        if (!serviceValvesStarted) {
          serviceValvesStarted=true;
          // start service valves
          StmApp.valvesCalibration();
        }
    } else serviceValvesStarted=false;
  }
}

void CServices::checkGetNtp()
{
  // check if time is 3:05 pm
  getLocalTime(&VdmNet.startTimeinfo);
  if ((VdmNet.startTimeinfo.tm_hour==getNtpHour) && (VdmNet.startTimeinfo.tm_min==getNtpMin)) {
    VdmNet.setupNtp();
  }
}

void CServices::servicesLoop()
{
  checkServiceValves();  
  checkGetNtp();
}

void CServices::runOnce() 
{
    StmApp.getParametersFromSTM();
}

void CServices::runOnceDelayed()
{
  VdmTask.startPIServices();
  VdmNet.startBroker();
}

void CServices::restartSystem(TRestartMode thisRestartMode,bool waitQueueFinished) {
  restartMode=thisRestartMode;
  StmApp.waitForFinishQueue=waitQueueFinished;
  #ifdef forceHardReset
    restartMode=hard;
    restartSTM=true;
  #endif
  #ifdef EnvDevelop
    UART_DBG.println("restart System "+String(restartMode));
  #endif
  if (restartMode==soft) {
    StmApp.waitForFinishQueue=true;
    StmApp.softReset();
  }
  if (StmApp.waitEEPFinished) {
    StmApp.eepState=EEP_REQUEST; 
    restartSTM=true;
  }
  if (StmApp.waitForFinishQueue || StmApp.waitEEPFinished) {
        #ifdef EnvDevelop
          UART_DBG.println("wait for finish queue");
        #endif
        VdmTask.taskIdResetSystem = taskManager.scheduleOnce(60*1000, [] {
                #ifdef EnvDevelop
                  UART_DBG.println("wait for finish queue timeout, restart now");
                #endif
                if (Services.restartSTM) {
                  syslog.log(LOG_DEBUG,"Services: queue restart hard STM32 after 30s");
                  Stm32.ResetSTM32(true);
                }
                delay (1000);
                ESP.restart();
            });
        VdmTask.taskIdwaitForFinishQueue = taskManager.scheduleFixedRate(500, [] {
          int restartNow = 0;
          if (StmApp.waitForFinishQueue) restartNow=Queue.available();
          if (StmApp.waitEEPFinished) {
            if ((StmApp.eepState!=EEP_DONE) && (StmApp.eepState!=EEP_IDLE)) restartNow++; 
            #ifdef EnvDevelop
              UART_DBG.println("waiting eep finished "+String(StmApp.eepState));
            #endif
          } 
          if (restartNow==0) {
              #ifdef EnvDevelop
                UART_DBG.println("queue/eeprom finished, restart now");
              #endif
              StmApp.eepState=EEP_IDLE;
                VdmTask.taskIdResetSystem = taskManager.scheduleOnce(3000, [] {
                  if (Services.restartSTM) {
                    syslog.log(LOG_DEBUG,"Services: queue restart STM32 after 3s");
                    #ifdef EnvDevelop
                      UART_DBG.println("restart System after queue now "+String(Services.restartMode));
                    #endif
                    if (Services.restartMode==hard) {
                      Stm32.ResetSTM32(true);
                      delay (2000);
                    }
                  }
                  delay (1000);
                  ESP.restart();
                });
                VdmTask.deleteTask(VdmTask.taskIdwaitForFinishQueue);
          }
        });
    } else {
      #ifdef EnvDevelop
        UART_DBG.println("normal restart");
      #endif
      VdmTask.taskIdResetSystem = taskManager.scheduleOnce(2000, [] {
                if (Services.restartSTM || Services.restartMode==hard) {
                  syslog.log(LOG_DEBUG,"Services: normal restart STM32 after 2s");
                  #ifdef EnvDevelop
                    UART_DBG.println("restart System now "+String(Services.restartMode));
                  #endif
                  if (Services.restartMode==hard) {
                    Stm32.ResetSTM32(true);
                    delay (2000);
                  }
                }
                delay (1000);
                ESP.restart();
            });
    }
}

void CServices::restartStmApp(uint32_t ms) {
      VdmTask.restartStmApp = taskManager.scheduleOnce(ms, [] {
                VdmTask.startApp();
            });
}
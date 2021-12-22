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


#include "VdmTask.h"
#include "VdmNet.h"
#include "ServerServices.h"
#include <BasicInterruptAbstraction.h>

CVdmTask VdmTask;

CVdmTask::CVdmTask()
{
  taskIdCheckNet=TASKMGR_INVALIDID;
  taskIdMqtt=TASKMGR_INVALIDID;
  taskIdApp=TASKMGR_INVALIDID;
  taskIdStm32Ota=TASKMGR_INVALIDID;
  taskIdServices=TASKMGR_INVALIDID;
  taskIdSetFactoryCfgTimeOut=TASKMGR_INVALIDID;
  taskIdSetFactoryCfgInProgress=TASKMGR_INVALIDID;
  setFactoryCfgState=idle;
}

void CVdmTask::init()
{
    if (taskIdCheckNet==TASKMGR_INVALIDID) {
        taskIdCheckNet = taskManager.scheduleFixedRate(1000, [] {
            VdmNet.checkNet();
        });
    }
}

void CVdmTask::startMqtt()
{
    if (taskIdMqtt==TASKMGR_INVALIDID) {
        taskIdMqtt = taskManager.scheduleFixedRate(10000, [] {
            VdmNet.mqttBroker();
        });
    }
}

void CVdmTask::startApp()
{
    if (taskIdStm32Ota!=TASKMGR_INVALIDID) 
    {
        deleteTask (taskIdStm32Ota);
        delay (1000);       // wait to finish task; 
        taskIdStm32Ota=TASKMGR_INVALIDID;
        UART_DBG.println("delete task stmOta"); 
    }
    if (taskIdApp==TASKMGR_INVALIDID) { 
        UART_DBG.println("start task stmApp");
        app_setup();
        taskIdApp = taskManager.scheduleFixedRate(100, [] {
                app_loop();
        });
        UART_DBG.println("task stmApp " +String(taskIdApp)); 
    } else {
        taskManager.setTaskEnabled (taskIdApp,true);  
        UART_DBG.println("restart task stmApp");  
    }
}

void CVdmTask::startStm32Ota(uint8_t command,String thisFileName)
{
    taskManager.setTaskEnabled (taskIdApp,false);
    UART_DBG.println("stop task stmApp");
    delay (1000);           // wait to finish task;
    //STM32ota_setup();         // Lenti84 --> must be called at startup because of pin configuration
    UART_DBG.println("start task stmOta");
    STM32ota_start(command,thisFileName);
    if (taskIdStm32Ota==TASKMGR_INVALIDID) {
        taskIdStm32Ota = taskManager.scheduleFixedRate(50, [] {
            STM32ota_loop();
        });
    } else {
        UART_DBG.println("task stmOta exists, wait 100 ms for setting");
        delay (100);         
    }
}


void CVdmTask::startServices()
{
    addIntPinResetCfg();
    taskIdServices = taskManager.scheduleFixedRate(1000, [] {
        Services.ServicesLoop();
    });
    
}

void CVdmTask::deleteTask (taskid_t taskId)
{
  taskManager.cancelTask (taskId);
}

bool CVdmTask::taskExists (taskid_t taskId)
{
   return (taskManager.getTask(taskId)!=NULL);
}

void CVdmTask::yieldTask (uint16_t ms)
{
    taskManager.yieldForMicros(ms*1000);
}

void interruptTask(pintype_t thisPin) 
{
   VdmTask.handleSetFactoryCfg (thisPin);
}
    
void CVdmTask::handleSetFactoryCfg (pintype_t thisPin)
{
    int readPin = digitalRead(2);
    UART_DBG.println("Interrupt triggered " + String(thisPin) + String(readPin));
    if (readPin==0) {
        if (setFactoryCfgState==idle) {
            setFactoryCfgState=inProgress;
            taskIdSetFactoryCfgTimeOut = taskManager.scheduleOnce(60*1000, [] {
                VdmTask.setFactoryCfgState=idle;
                UART_DBG.println("setFactoryCfgState idle");
            });
            taskIdSetFactoryCfgInProgress = taskManager.scheduleOnce(10*1000, [] {
                VdmTask.setFactoryCfgState=action;
                UART_DBG.println("setFactoryCfgState action");
            });
        }
    } else {
      if (setFactoryCfgState==action) {
          setFactoryCfgState=resetCfg;
          UART_DBG.println("restore Config");
          restoreConfig();
      }       
    }
}


void CVdmTask::addIntPinResetCfg ()
{
    const int pinSetFactoryCfg = 2;
    BasicArduinoInterruptAbstraction interruptAbstraction;

    pinMode(pinSetFactoryCfg, INPUT_PULLUP);

    taskManager.setInterruptCallback(interruptTask);
    taskManager.addInterrupt(&interruptAbstraction, pinSetFactoryCfg, CHANGE);
}
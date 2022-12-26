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



#pragma once

#include <TaskManagerIO.h>
#include "globals.h"
#include "VdmNet.h" 
#include "stmApp.h"
#include "Services.h"
#include "stm32ota.h"
#include "stm32.h"

enum TsetFactoryCfgState  {idle,inProgress,action,resetCfg};

class CVdmTask
{
public:
  CVdmTask();
  void init();
  void deleteTask(taskid_t taskId);
  bool taskExists (taskid_t taskId);
  void yieldTask (uint16_t ms);
  void startMqtt(uint32_t interval);
  void startApp();
  void startStm32Ota(uint8_t command,String thisFileName);
  void startServices();
  void startPIServices();
  void startClearFS();
  void startGetFS();
  
  taskid_t taskIdCheckNet;
  taskid_t taskIdMqtt;
  taskid_t taskIdApp;
  taskid_t taskIdStm32Ota;
  taskid_t taskIdServices;
  taskid_t taskIdSetFactoryCfgTimeOut; 
  taskid_t taskIdSetFactoryCfgInProgress;
  taskid_t taskIdResetSystem;
  taskid_t restartStmApp;
  taskid_t taskIdRunOnce;
  taskid_t taskIdRunOnceDelayed;
  taskid_t taskIdwaitForFinishQueue;
  taskid_t taskIdPiControl[ACTUATOR_COUNT];
  taskid_t taskIdRunOnceClearFS;
  taskid_t taskIdRunOnceGetFS;
  
  TsetFactoryCfgState setFactoryCfgState;
};

extern CVdmTask VdmTask;

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
#include "stmApp.h"

CServices Services;

CServices::CServices()
{
  serviceValvesStarted=false;
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

void CServices::servicesLoop()
{
  checkServiceValves();  
}

void CServices::runOnce() 
{
   StmApp.app_cmd(APP_PRE_GETVERSION);
   StmApp.app_cmd(APP_PRE_SETMOTCHARS,
          String(VdmConfig.configFlash.motorConfig.maxLowCurrent)+
          String(" ")+String(VdmConfig.configFlash.motorConfig.maxHighCurrent)+String(" "));
}

void CServices::restartSystem() {
      VdmTask.taskIdResetSystem = taskManager.scheduleOnce(1000, [] {
                ESP.restart();
            });
}

void CServices::restartStmApp(uint32_t ms) {
      VdmTask.restartStmApp = taskManager.scheduleOnce(ms, [] {
                VdmTask.startApp();
            });
}
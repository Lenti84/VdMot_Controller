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

CVdmTask VdmTask;

CVdmTask::CVdmTask()
{
}

void CVdmTask::init()
{
    taskIdCheckNet = taskManager.scheduleFixedRate(1000, [] {
        VdmNet.checkNet();
    });
}

void CVdmTask::startMqtt()
{
    taskIdMqtt = taskManager.scheduleFixedRate(10000, [] {
        VdmNet.mqttBroker();
    });
}

void CVdmTask::startApp()
{
    app_setup();
    taskIdApp = taskManager.scheduleFixedRate(100, [] {
        app_loop();
    });
}

void CVdmTask::startServices()
{
    taskIdServices = taskManager.scheduleFixedRate(60*1000, [] {
        Services.ServicesLoop();
    });
}

void CVdmTask::deleteTask (taskid_t taskId)
{
  taskManager.cancelTask (taskId);
}
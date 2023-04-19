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


#include "globals.h"

#include "VdmConfig.h"
#include "VdmSystem.h"
#include "VdmNet.h"
#include "VdmTask.h"
#include "stm32.h"
#include "esp_task_wdt.h"
#include "esp_err.h"
#include "Messenger.h"
#include "VdmTask.h"

uint32_t * StackPtrAtStart;
uint32_t * StackPtrEnd;
UBaseType_t watermarkStart;
uint32_t stackSize;


void setup(void) {
  disableCore0WDT();
  disableCore1WDT();
  disableLoopWDT(); 

  UART_DBG.begin(115200);

  uint32_t* SpStart = NULL;
  StackPtrAtStart = (uint32_t *)&SpStart;
  watermarkStart =  uxTaskGetStackHighWaterMark(NULL);
  StackPtrEnd = StackPtrAtStart - watermarkStart;
  stackSize = StackPtrAtStart - StackPtrEnd;

  #ifdef EnvDevelop
    UART_DBG.println("VdMot_Controller");
    UART_DBG.printf("\r\n\r\nAddress of Stackpointer near start is:  %p \r\n",  (uint32_t *)StackPtrAtStart);
    UART_DBG.printf("End of Stack is near: %p \r\n",  (uint32_t *)StackPtrEnd);
    UART_DBG.printf("Free Stack near start is:  %d \r\n",  (uint32_t)StackPtrAtStart - (uint32_t)StackPtrEnd);
    UART_DBG.printf("Free Prefs entries  %d \r\n",  VdmConfig.prefs.freeEntries());
    UART_DBG.printf("Config space  %d bytes (max 20k) \r\n",  sizeof(VdmConfig.configFlash));
  #endif

  Stm32.STM32ota_setup();
  uint8_t rr=esp_reset_reason();
  if (rr!=1) {  // not at power on reset
    Stm32.ResetSTM32(false);
  }
  #ifdef EnvDevelop
    UART_DBG.println("Start Config");
  #endif
  // init config, read from flash, init network
  VdmSystem.stackSize = stackSize;
  VdmConfig.init();
  VdmConfig.checkToResetCfg();
  VdmNet.init();
  VdmTask.init();
  VdmSystem.getFSDirectory(); 
}

void loop(void) {
  taskManager.runLoop(); 
}



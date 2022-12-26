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

#define piControlManual 0
#define piControlOn     1
#define piControlOff    2

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
  };
  void exec() override {
    controlValve();
	}
  float piCtrl(float e);
  uint8_t calcValve();
  void controlValve();
  uint32_t ta;
  uint32_t ti;
  volatile uint16_t xp;
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
private:
  float iProp;
  time_t ts;
  bool start;
  float esum;
};

extern CPiControl PiControl[ACTUATOR_COUNT];

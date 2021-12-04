/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano

  Comments:


***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************

*END************************************************************************/

#pragma once

#include <stdint.h>
#include <ArduinoJson.h>
#include "VdmConfig.h"

#define noneProtocol 0
#define mqttProtocol 1

class CVdmNet
{
public:
  CVdmNet();
  void init();
  void setup();
  void setupEth();
  void setupWifi();
  void initServer();
  void setupNtp();
  void netLoop();
  void valvesCalib();
  void checkNet();
 
  bool serverIsStarted;
  bool dataBrokerIsStarted;
};

extern CVdmNet VdmNet;




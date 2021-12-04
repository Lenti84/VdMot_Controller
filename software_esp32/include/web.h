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

#ifndef _WEB_H_
#define _WEB_H_

#include <Arduino.h>
#include "Logger.h"
#include "ETH.h"
#include "VdmConfig.h"
#include "globals.h"

//#include <FS.h>

//extern File fsUploadFile;

extern Logger logger;


void handleFileUpload();
void handleFlash();
void handleFileDelete();
void handleListFiles();
String makePage(String title, String contents);
void postValvesPos();
String getValvesStatus();
String getTempsStatus(VDM_TEMPS_CONFIG tempsConfig);
String getNetInfo(ETHClass ETH,VDM_NETWORK_CONFIG netConfig); 
String getNetConfig (VDM_NETWORK_CONFIG netConfig);
String getProtConfig (VDM_PROTOCOL_CONFIG protConfig);
String getValvesConfig (VDM_VALVES_CONFIG valvesConfig);
String getTempsConfig (VDM_TEMPS_CONFIG tempsConfig);
String getSysInfo();
String ip2String (IPAddress ipv4addr);

#endif
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


#include "Messenger.h"
#include <Arduino.h>
#include "Pushover.h"
#include "globals.h"

CMessenger Messenger;

CMessenger::CMessenger()
{
}

void CMessenger::sendMessage (const char* title,const char* thisMessage)
{
  if (VdmConfig.configFlash.messengerConfig.activeFlags.pushOver) {
    sendPO((const char*) &VdmConfig.configFlash.messengerConfig.pushover.appToken,(const char*) &VdmConfig.configFlash.messengerConfig.pushover.userToken,
      title, thisMessage);
  }
}

int CMessenger::testPO(JsonObject doc)
{
    char appToken [31]={0};
    char userToken [31]={0};
    char title [31]={0};
    char message [31] = {"Test from VDMotFB"} ;

    if (!doc["appToken"].isNull()) strncpy(appToken,doc["appToken"].as<const char*>(),sizeof(appToken));
    if (!doc["userToken"].isNull()) strncpy(userToken,doc["userToken"].as<const char*>(),sizeof(userToken));
    if (!doc["title"].isNull()) strncpy(title,doc["title"].as<const char*>(),sizeof(title));

    return (sendPO((const char*) &appToken,(const char*) &userToken,(const char*) &title,(const char*) &message));
}

int CMessenger::sendPO(const char* appToken, const char* userToken ,const char* title, const char* message)
{
    CPushoverMessage myMessage;
    pushoverClient.setToken(appToken);
    pushoverClient.setUser(userToken);
    myMessage.title = title;
    myMessage.message = message;
    int response;
    response = pushoverClient.send(myMessage);
    #ifdef EnvDevelop
        UART_DBG.println("Pushover "+String(response));
    #endif
    return response;
}

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
#include "ESP_Mail_Client.h"
#include "globals.h"
#include <string.h>

CMessenger Messenger;

CMessenger::CMessenger()
{
}

void CMessenger::sendMessage (const char* thisTitle,const char* thisMessage)
{
  if (VdmConfig.configFlash.messengerConfig.activeFlags.pushOver) {
    title=thisTitle;
    message=thisMessage;
    sendPO((const char*) &VdmConfig.configFlash.messengerConfig.pushover.appToken,(const char*) &VdmConfig.configFlash.messengerConfig.pushover.userToken,
      thisTitle, thisMessage);
  }
  if (VdmConfig.configFlash.messengerConfig.activeFlags.email) {
    title=thisTitle;
    message=thisMessage;
    sendEmail((const char*) &VdmConfig.configFlash.messengerConfig.email.user,
      (const char*) &VdmConfig.configFlash.messengerConfig.email.pwd,
      (const char*) &VdmConfig.configFlash.messengerConfig.email.host,VdmConfig.configFlash.messengerConfig.email.port,
      (const char*) &VdmConfig.configFlash.messengerConfig.email.recipient, thisTitle, thisMessage);
  }
}

int CMessenger::testPO(JsonObject doc)
{
    char appToken [31]={0};
    char userToken [31]={0};
    char title [31]={0};
    char message [31] = {"Test from "};
    strncat (message,VdmConfig.configFlash.systemConfig.stationName,sizeof(message) - strlen(message) - 1) ; 

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

void CMessenger::testEmail(JsonObject doc) 
{
  char user [65]={0};
  char pwd [65]={0};
  char title [31]={0};
  char host [65]={0};
  char recipient [65]={0};
  uint16_t port=465;
  char message [65] = {"Test from "};
  strncat (message,VdmConfig.configFlash.systemConfig.stationName,sizeof(message) - strlen(message) - 1) ; 
  if (!doc["user"].isNull()) strncpy(user,doc["user"].as<const char*>(),sizeof(user));
  if (!doc["pwd"].isNull()) strncpy(pwd,doc["pwd"].as<const char*>(),sizeof(pwd));
  if (!doc["host"].isNull()) strncpy(host,doc["host"].as<const char*>(),sizeof(host));
  if (!doc["port"].isNull()) port = doc["port"];
  if (!doc["recipient"].isNull()) strncpy(recipient,doc["recipient"].as<const char*>(),sizeof(recipient));
  if (!doc["title"].isNull()) strncpy(title,doc["title"].as<const char*>(),sizeof(title));
  sendEmail (user,pwd,host,port,recipient,title,message);
}

 

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  Messenger.mSMTCallback(status);
}

void CMessenger::mSMTCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}

void CMessenger::sendEmail (const char* user, const char* pwd ,const char* host, uint16_t port,const char* recipient,const char* title,const char* thisMessage)
{

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
 */

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

/** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  #ifdef EnvDevelop
    smtp.debug(1);
    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);
  #endif

  /* Declare the Session_Config for user defined session credentials */
  Session_Config config;

  /* Set the session config */
  config.server.host_name = host;   //SMTP_HOST;
  config.server.port = port;        //SMTP_PORT;
  config.login.email = user;        //AUTHOR_EMAIL;
  config.login.password = pwd;      //AUTHOR_PASSWORD;

  /** Assign your host name or you public IPv4 or IPv6 only
   * as this is the part of EHLO/HELO command to identify the client system
   * to prevent connection rejection.
   * If host name or public IP is not available, ignore this or
   * use generic host "mydomain.net".
   *
   * Assign any text to this option may cause the connection rejection.
   */
  //config.login.user_domain = F("1und1.de");

 /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  //message.sender.name = F("ESP Mail");
  message.sender.email = user; //AUTHOR_EMAIL;
  message.subject = title; //F("Test sending plain text Email");
  message.addRecipient(recipient,recipient);// RECIPIENT_EMAIL);(F("Someone"), RECIPIENT_EMAIL);

  //String textMsg = thisMessage; //"This is simple plain text message";
  message.text.content = thisMessage; // textMsg;

  if (!smtp.connect(&config))
  {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  #ifdef EnvDevelop
    if (!smtp.isLoggedIn())
    {
      Serial.println("\nNot yet logged in.");
    }
    else
    {
      if (smtp.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
      else
        Serial.println("\nConnected with no Auth.");
    }
  #endif
  
  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

}
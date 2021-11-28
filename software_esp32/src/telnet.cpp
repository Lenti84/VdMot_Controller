/* ------------------------------------------------- */
/*

Credits to Stefan Nikolaus
Wifi Telnet Server auf dem ESP8266
https://www.nikolaus-lueneburg.de/2017/09/wifi-telnet-server-auf-dem-esp8266/

*/
/* ------------------------------------------------- */

#include <Arduino.h>
#include "WiFi.h"
#include <WiFiUdp.h>
#include "app.h"

uint8_t i;
bool ConnectionEstablished; // Flag for successfully handled connection
  
#define MAX_TELNET_CLIENTS 2

WiFiServer TelnetServer(23);
WiFiClient TelnetClient[MAX_TELNET_CLIENTS];


void telnet_setup () {

    TelnetServer.begin();
    TelnetServer.setNoDelay(true);

}


void telnet_msg(String text)
{
  for(i = 0; i < MAX_TELNET_CLIENTS; i++)
  {
    if (TelnetClient[i] || TelnetClient[i].connected())
    {
      TelnetClient[i].println(text);
    }
  }
  //delay(10);  // to avoid strange characters left in buffer
  delay(1);
}
      

void telnet_loop()
{
  String clientCommand;
  clientCommand= "";
  
  // Cleanup disconnected session
  for(i = 0; i < MAX_TELNET_CLIENTS; i++)
  {
    if (TelnetClient[i] && !TelnetClient[i].connected())
    {
      Serial.print("Client disconnected ... terminate session "); Serial.println(i+1); 
      TelnetClient[i].stop();
    }
  }
  
  // Check new client connections
  if (TelnetServer.hasClient())
  {
    ConnectionEstablished = false; // Set to false
    
    for(i = 0; i < MAX_TELNET_CLIENTS; i++)
    {
      // Serial.print("Checking telnet session "); Serial.println(i+1);
      
      // find free socket
      if (!TelnetClient[i])
      {
        TelnetClient[i] = TelnetServer.available(); 
        
        Serial.print("New Telnet client connected to session "); Serial.println(i+1);
        
        TelnetClient[i].flush();  // clear input buffer, else you get strange characters
        TelnetClient[i].println("Welcome!");
        
        TelnetClient[i].print("Millis since start: ");
        TelnetClient[i].println(millis());
        
        TelnetClient[i].print("Free Heap RAM: ");
        TelnetClient[i].println(ESP.getFreeHeap());
  
        TelnetClient[i].println("----------------------------------------------------------------");
        
        ConnectionEstablished = true; 
        
        break;
      }
      else
      {
        // Serial.println("Session is in use");
      }
    }

    if (ConnectionEstablished == false)
    {
      Serial.println("No free sessions ... drop connection");
      TelnetServer.available().stop();
      // TelnetMsg("An other user cannot connect ... MAX_TELNET_CLIENTS limit is reached!");
    }
  }

  for(i = 0; i < MAX_TELNET_CLIENTS; i++)
  {
    if (TelnetClient[i] && TelnetClient[i].connected())
    {
      if (TelnetClient[i].available() > 0)
      {
        int h = TelnetClient[i].available(); // length of the command ????
     
        for (int c = 0; c < h; c++)
        {
          clientCommand+= (char)TelnetClient[i].read();
        }

        if (clientCommand == "stgt") {              
          TelnetClient[i].println("set target send");
          Serial.println("set target send");
          app_cmd(APP_CMD_SETTARGETPOS);
        }
        else if (clientCommand == "gact") {              
          TelnetClient[i].println("get actual values send");
          Serial.println("get actual values send");
          app_cmd(APP_CMD_GETACTUALPOS);
        }
      }
      else
      {
        //Serial.println("nix");
      }
    }
  }
}
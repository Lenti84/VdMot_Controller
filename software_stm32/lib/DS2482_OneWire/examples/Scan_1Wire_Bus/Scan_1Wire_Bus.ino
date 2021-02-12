#include <Wire.h>
#include <OneWire.h>

OneWire oneWire;

void printAddress(uint8_t deviceAddress[8])
{
  Serial.print("{ ");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    Serial.print("0x");
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i<7) Serial.print(", ");
    
  }
  Serial.print(" }");
}

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  Serial.println("Checking for I2C devices...:");
  if (oneWire.checkPresence())
  {
    Serial.println("DS2482-100 present");
    
    oneWire.deviceReset();
    
    Serial.println("\tChecking for 1-Wire devices...");
    if (oneWire.wireReset())
    {
      Serial.println("\tDevices present on 1-Wire bus");
      
      uint8_t currAddress[8];
      
      Serial.println("\t\tSearching 1-Wire bus...");
      
      while (oneWire.wireSearch(currAddress))
      {
        Serial.print("\t\t\tFound device: ");
        printAddress(currAddress);
        Serial.println();
      }
      
      oneWire.wireResetSearch();
      
    }
    else
      Serial.println("\tNo devices on 1-Wire bus");
  }
  else
    Serial.println("No DS2482-100 present");
    
  delay(5000);
}

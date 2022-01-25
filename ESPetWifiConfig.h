#include "EEPROM.h"

void initEEPROM(){
  if (!EEPROM.begin(256)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
}

void readWifiParameters(String &ssid, String &password){
  int address = 0;
  ssid = EEPROM.readString(address);
  address += ssid.length() + 1;
  password = EEPROM.readString(address);
}

void saveWifiParameters(String ssid, String password){
  int address = 0;
  address += EEPROM.writeString(address, ssid) + 1;
  address += EEPROM.writeString(address, password);
  EEPROM.commit();
}

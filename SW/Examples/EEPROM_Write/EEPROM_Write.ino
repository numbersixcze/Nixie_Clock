#include <EEPROM.h>

struct SystemData {
  char ssid[20];
  char passwordW[20];
  char login[20];
  char password[20];
};

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  int eeAddress = 0;
  
  SystemData parameters = {
    "muj-tp-link",
    "tajne-heslo",
    "admin",
    "admin2",
};

  EEPROM.put(eeAddress, parameters);
  Serial.print("Written custom data type! \n\nView the example sketch eeprom_get to see how you can retrieve the values!");
}

void loop() {   /* Empty loop */ }

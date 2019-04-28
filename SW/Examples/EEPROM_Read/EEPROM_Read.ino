#include <EEPROM.h>

struct SystemData {
  char ssid[20];
  char passwordW[20];
  char login[15];
  char password[15];
};

void setup(){

  int eeAddress = 0; //EEPROM address to start reading from

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  
  SystemData customVar; //Variable to store custom object read from EEPROM.
  EEPROM.get( eeAddress, customVar );

  Serial.println( "Read custom object from EEPROM: " );
  Serial.println( customVar.ssid);
  Serial.println( customVar.passwordW);
  Serial.println( customVar.login);
  Serial.println( customVar.password);
}

void loop(){ /* Empty loop */ }
 

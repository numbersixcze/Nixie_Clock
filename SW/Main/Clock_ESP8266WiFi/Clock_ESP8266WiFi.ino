#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ShiftRegister74HC595.h>
#include <Ticker.h>


#define NTPSYNC 90           //Interval for sync data from NTP server
#define NAMESERVER "alarm"    //name for DNS, example when you write "alarm" you should write in web browser alarm.local
#define NTPTIMEOUT 30       //Timeout for answer from NTP server

ESP8266WebServer server(80);  // Create a webserver object that listens for HTTP request on port 80

void handleRoot();              
void handleMain();
void handleNotFound();
void handleLogin();
void handleSetTime();
void handleSetMessage();
void handleAlarm();
void handleAlarmMessage();
void handleAlarmOff();
void handleChLogin();
void handleLogout();
void handleChLoginMessage();
void handleAccess();
void handleAccessMessage();
void handleNTP();
void handleNTPMessage();
void handleDixi();

int TestParseToInt(String iStr);

//temporary variable for save answer from the server
String iHours = "1";
String iMinutes = "1";
String iSeconds = "1";
String iDays = "1";
String iMonths = "1";
String iYears = "1";
String aHours = "1";
String aMinutes = "1";

//Web access
String login = "admin";       //Default login
String password = "admin";    //Default password
String tmpLogin = "";
String tmpPassword = "";

//temporary variable for convert string to int
int intHours = 2;
int intMinutes = 2;
int intSeconds = 2;
int intDays = 2;
int intMonths = 2;
int intYears = 2;
int aintHours = 2;
int aintMinutes = 2;
int timeZone = 2;     // Central European Time
int i = 0;

time_t prevDisplay = 0; // when the digital clock was displayed
uint8_t TimeDate = 0;

bool isNtpSet = false;
bool isAlarmSet = false;
bool dixiON = true;

//Wi-Fi Access
String ssid = "TP-LINK";  //  your network SSID (name)
String pass = "Regulace150";       // your network password

// NTP Servers:
String ntpServerName = "ntp.cesnet.cz";
//static const char ntpServerName[] = "cz.pool.ntp.org";  //Nahradni DNS server

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

//Initialize NTP synchronization 
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

// Pin Definition
const uint8_t AnalogPin     = A0;   //analogový vstup z multiplexu
const uint8_t VN_EN_Pin     = 14;   //VN enable  
const uint8_t VN_CTRL_Pin   = 12;   //VN level control
const uint8_t ExtServoPin   = 16;   //Extern Servo Control Pin
const uint8_t SCK_EIO_Pin   = 13;   //SCK to 595 to extended Outputs 
const uint8_t RCK_EIO_Pin   = 15;   //RCK to 595 to extended Outputs
const uint8_t SER_EIO_Pin   = 5;    //serial_in to 595 to extended Outputs
const uint8_t SER_NX_Pin    = 4;    //serial_in to 595 to nixie tubes
const uint8_t SCK_NX_Pin    = 0;    //SCK to 595 to nixie tubes
const uint8_t RCK_NX_Pin    = 2;    //RCK to 595 to nixie tubes

//Loop Times Setup
const uint16_t FastLoopMs = 200;     //Fast loop called thru timer interrupt every x ms
const uint16_t SlowLoopMul = 3;    //After this number is achieved slow loop is called SlowLoopMs = FastLooMs*SlowLoopMul

//Signals defined for weitching analog multilex
enum Signals {VN_U = 0, VN_I, OC_OV, NoiseAD, PWR_Status, Btn1, Btn2, UCC_U};   //Names
double RSignals[8] = {0,};                                                      //Number  storage [V] - value that reads ADC

//Instance creation
Ticker Timer;                                                           //Create instance of Timer
ShiftRegister74HC595 EIO595(1, SER_EIO_Pin, SCK_EIO_Pin, RCK_EIO_Pin);  //Create instance of shift registr for extended output
ShiftRegister74HC595 NX595(2, SER_NX_Pin, SCK_NX_Pin, RCK_NX_Pin);      //Create instance of shift registr for displaying numbers on nixies

//HEADERS
double ReadSignal(Signals MulSig);      //Reads chosen signal, you better use GetSignal()
double GetSignal(Signals Sig);          //signal getter
void SetNX(uint8_t NumLeftPair, char Dots, uint8_t NumRightPair);   //Sign Dots will blink, directly sets nixies
void BuzzerOn();
void BuzzerOff();
void StatusLedOn();
void StatusLedOff();
void ReleOn();
void ReleOff();
void ClockDots(char Dost);  //Sets dots, both, uppper, bottom or none
void SlowLoop();
void VnOn();
void VnOff();
void SerialUpdate();    //not used, only for debug. Sending some data over serial (4 pin header or usb of Wemos)

// fce 
bool StateN = 0;
void SetNX(uint8_t NumLeftPair, char Dots, uint8_t NumRightPair);
void SerialUpdate();
void VnOn();
void VnOff();
void ClockDots(char Dots);
double ReadSignal(Signals MulSig);
double GetSignal(Signals Sig);
void BuzzerOn();
void BuzzerOff();
void ReleOn();
void ReleOff();
void StatusLedOn();
void StatusLedOff();

uint8_t ReadRotate = 0;
uint16_t LoopCounter = 0;
void FastLoop();
void SlowLoop();


void setup()
{
    //Setting all I/O configs
  pinMode(AnalogPin, INPUT);
  pinMode(VN_EN_Pin, OUTPUT);
  pinMode(VN_CTRL_Pin, OUTPUT);
  pinMode(ExtServoPin, OUTPUT);
  pinMode(SCK_EIO_Pin, OUTPUT);
  pinMode(RCK_EIO_Pin, OUTPUT);
  pinMode(SER_EIO_Pin, OUTPUT);
  pinMode(SCK_NX_Pin, OUTPUT);
  pinMode(RCK_NX_Pin, OUTPUT);
  pinMode(SER_NX_Pin, OUTPUT);

  //Begin serial comm
  Serial.begin(115200);
   
  //while (!Serial) ; // Needed for Leonardo only
  delay(250);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  //Manual set IP address
  /*
  IPAddress ip(192,168,1,205);   
  IPAddress gateway(192,168,1,1);   
  IPAddress subnet(255,255,255,0);   
  WiFi.config(ip, gateway, subnet);
  */

  //Connecting to Wi-Fi AP
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  
  server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/login", HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/setmessage",HTTP_POST,handleSetMessage);
  server.on("/settime", HTTP_GET, handleSetTime);
  server.on("/alarm", HTTP_GET, handleAlarm); 
  server.on("/alarmmessage",HTTP_POST,handleAlarmMessage);
  server.on("/alarmoff",HTTP_GET,handleAlarmOff);
  server.on("/chlogin",HTTP_GET,handleChLogin);
  server.on("/chloginmessage",HTTP_POST,handleChLoginMessage);
  server.on("/access",HTTP_GET,handleAccess);
  server.on("/accessmessage",HTTP_POST,handleAccessMessage);
  server.on("/ntp",HTTP_GET,handleNTP);
  server.on("/ntpmessage",HTTP_POST,handleNTPMessage);
  server.on("/main",HTTP_GET,handleMain);
  server.on("/logout",HTTP_GET,handleLogout);
  server.on("/dixi",HTTP_GET,handleDixi);

  server.onNotFound(handleNotFound);         // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  //Setting Multicast DNS
  if (!MDNS.begin(NAMESERVER)) {
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(NTPSYNC);

  //Zero everything out
  EIO595.setAllLow();
  NX595.setAllLow();

  //Sets loop
  Timer.attach_ms((uint32_t)FastLoopMs, FastLoop);
}


void loop()
{
  MDNS.update();
  server.handleClient(); 
  
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }

}



int TestParseToInt(String iStr){
    if( (iStr != "0") || (iStr != "00")){
        if(iStr.toInt() != 0)
            return iStr.toInt();
        else
            return 999;
    }
    return 0;
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName.c_str(), ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < NTPTIMEOUT) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      isNtpSet = true;

      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }

  Serial.println("No NTP Response :-(");
  isNtpSet = false;
  return 0;
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


void handleRoot() {                          // When URI / is requested, send a web page with login form
  server.send(200, "text/html", "<meta charset='UTF-8'> <form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Uživatel\"></br><input type=\"password\" name=\"password\" placeholder=\"Heslo\"></br><input type=\"submit\" value=\"Přihlásit\"></form><p>Zkus 'admin' a heslo 'admin' ...</p>");
 }

void handleLogin() {                         // If a POST request is made to URI /login
  if( ! server.hasArg("username") || ! server.hasArg("password") 
      || server.arg("username") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neplatný požadavek<br> <a href='/'><button>Vrátit zpět</button>");
    return;
  }
    tmpLogin      = server.arg("username");
    tmpPassword   = server.arg("password");

    if(tmpLogin == login && tmpPassword == password){
      server.send(200, "text/html", "<script type='text/javascript'> window.location = '/main'; </script>");
    }
    else{
      server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
    }
}

void handleLogout(){
    tmpLogin      = "";
    tmpPassword   = "";
    server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Odhlášení provedeno</h1> <br> <a href='/'><button>Zpět domů</button></a>");
}

void handleSetTime(){
  iHours   =    hour();
  iMinutes =  minute();
  iSeconds =  second();
  iDays    =     day();
  iMonths  =   month();
  iYears   =    year(); 

  if(tmpLogin == login && tmpPassword == password){
    server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Nastavení času</h1><p> Čas: <br>(h,m,s) " + iHours + ":" + iMinutes + ":" + iSeconds + "<br> (d,m,r) " + iDays + "." + iMonths + "." + iYears + "</p> <a href='/main'><button>Zpět domů</button></a> <form action='/setmessage' method='POST'> h: <input type='number' min='0' max='23' name='fHours'/> m: <input type='number' min='0' max='59' name='fMinutes'/>  s: <input type='number' min='0' max='59' name='fSeconds'/> <br> d: <input type='number' min='1' max='31' name='fDays'/> m: <input type='number' min='1' max='12' name='fMonths'/> r: <input type='number' min='1970' name='fYears'/> <input type='submit' value='Odeslat'/> </form>");
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}

void handleNotFound(){
  server.send(404, "text/html", "<meta charset='UTF-8'> 404: Stránka nenalezena"); // Send HTTP status 404 (Not Found) when there's no handler for the URL in the request
}

void handleMain(){
  if(tmpLogin == login && tmpPassword == password){
    server.send(200, "text/html", "<meta charset='UTF-8' http-equiv='refresh' content='1'> <style>.dot{width:16px;height:16px;background-color:gray;border-radius:50%;display:inline-block}.dot.red{background-color:red}.dot.green{background-color:green}</style> <h1>Vítejte " + tmpLogin + "!</h1><br> Aktuální čas (h,m,s,d,m,r): " + hour() + ":" + minute() + ":" + second() + " " + day() + "." + month() + "." + year() +  "<br> <form action='/dixi' method='GET'> <input type='submit' value='Zapnout nebo vypnout digitrony'/></form> <a href='/settime'><button>Nastavení času</button> </a> <br> <a href='/alarm'><button>Nastavení budíčku</button> <br> <a href='/chlogin'><button>Změna hesla a účtu</button></a><br><a href='/access'><button>Změna nastavení Wi-Fi</button></a><br><a href='/ntp'><button>Změna NTP serveru</button></a> <br> <a href='/logout'><button>Odhlásit</button></a><div style='position: absolute; right: 64px; top: 20px;'>NTP: <span class='" + (isNtpSet ? "green" : "red") + " dot'></span></div> <div style='position: absolute; right: 64px; top: 40px;'>Budíček: <span class='" + (isAlarmSet ? "green" : "red") + " dot'></span></div><div style='position: absolute; right: 64px; top: 60px;'>Digitrony: <span class='" + (dixiON ? "green" : "red") + " dot'></span></div>");
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}

void handleSetMessage(){
  if(tmpLogin == login && tmpPassword == password){
    if(server.hasArg("fHours") && server.hasArg("fMinutes") && server.hasArg("fSeconds") && server.hasArg("fDays") && server.hasArg("fMonths") && server.hasArg("fYears")){
      iHours = server.arg("fHours");
      iMinutes = server.arg("fMinutes");
      iSeconds = server.arg("fSeconds");
      iDays = server.arg("fDays");
      iMonths = server.arg("fMonths");
      iYears = server.arg("fYears");
      
      intHours = TestParseToInt(iHours);
      intMinutes = TestParseToInt(iMinutes);
      intSeconds = TestParseToInt(iSeconds);
      intDays = TestParseToInt(iDays);
      intMonths = TestParseToInt(iMonths);
      intYears = TestParseToInt(iYears);
      
      if((intHours != 999) || (intMinutes != 999) || (intSeconds != 999) || (intDays != 999) || (intMonths != 999) || (intYears != 999))    {
        setTime(intHours,intMinutes,intSeconds,intDays,intMonths,intYears);
        server.send(200, "text/html", "<script type='text/javascript'> window.location = '/settime'; </script>");
        return;
        }
      server.send(400, "text/html", "<meta charset='UTF-8'> 400: Invalid Request <br> <a href='/settime'><button>Vrátit zpět</button>");
      }
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}


void handleAlarm(){
  if(tmpLogin == login && tmpPassword == password){
    server.send(200, "text/html", "<style>.dot{width:16px;height:16px;background-color:gray;border-radius:50%;display:inline-block}.dot.red{background-color:red}.dot.green{background-color:green}</style> <meta charset='UTF-8'> <h1>Budíček</h1> <p> Budíček nastaven na: <br>" "h:"+ aHours + "  m:" + aMinutes + "</p> <a href='/main'><button>Zpět domů</button></a> <form action='/alarmoff' method='GET'> <input type='submit' value='Vypnout budík'/></form> <form action='/alarmmessage' method='POST'> h: <input type='number' min='0' max='23' name='afHours'/> m: <input type='number' min='0' max='59' name='afMinutes'/> <input type='submit' value='Nastavit'/> </form> <div style='position: absolute; right: 64px; top: 0px;'>Budíček: <span class='" + (isAlarmSet ? "green" : "red") + " dot'></span></div>");
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}

void handleAlarmMessage(){
  if(tmpLogin == login && tmpPassword == password){
    if(server.hasArg("afHours") && server.hasArg("afMinutes")){
      if((server.arg("afHours") != NULL) && (server.arg("afMinutes") != NULL)){
        aHours = server.arg("afHours");
        aMinutes = server.arg("afMinutes");
        aintMinutes = TestParseToInt(aMinutes);
        aintHours   = TestParseToInt(aHours);
        isAlarmSet = true;
        server.send(200, "text/html", "<script type='text/javascript'> window.location = '/alarm'; </script>");
        return;
      }
      server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neplatný požadavek<br> <a href='/alarm'><button>Vrátit zpět</button>");
    }
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}

void handleAlarmOff(){
  if(tmpLogin == login && tmpPassword == password){
    isAlarmSet = false;
    server.send(200, "text/html", "<script type='text/javascript'> window.location = '/alarm'; </script>");
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}


void handleChLogin(){
  if(tmpLogin == login && tmpPassword == password){
    server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Změna hesla a název účtu</h1> <br> <a href='/main'><button>Zpět domů</button></a> <br> <form action='/chloginmessage' method='POST'> účet: <input type='text' name='aLogin'/> heslo: <input type='text' name='aPassword'/> <input type='submit' value='Nastavit'/> </form>");
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}


void handleChLoginMessage(){
  if(tmpLogin == login && tmpPassword == password){
    if(server.hasArg("aLogin") && server.hasArg("aPassword")){
      login = server.arg("aLogin");
      password = server.arg("aPassword");
    }
    server.send(200, "text/html", "<script type='text/javascript'> window.location = '/'; </script>");
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}


void handleAccess(){
  if(tmpLogin == login && tmpPassword == password){
    server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Změna názvu a hesla Wi-Fi</h1> <a href='/main'><button>Zpět domů</button></a> <br> <form action='/accessmessage' method='POST'> SSID (název): <input type='text' name='aSSID'/> heslo: <input type='text' name='aPasw'/> <input type='submit' value='Nastavit'/> </form>");
    return;
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}

void handleAccessMessage(){
  if(tmpLogin == login && tmpPassword == password){
    if(server.hasArg("aSSID") && server.hasArg("aPasw")){
      String tmpLogin = server.arg("aSSID");
      String tmpPassword = server.arg("aPasw");
      server.send(200, "text/html", "<script type='text/javascript'> window.location = '/main'; </script>");
      Serial.print("Connecting to ");
      Serial.println(tmpLogin);
      WiFi.begin(tmpLogin, tmpPassword);

      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.print("IP number assigned by DHCP is ");
      Serial.println(WiFi.localIP());
    }
    
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}


void handleNTP(){
    if(tmpLogin == login && tmpPassword == password){
        server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Změna NTP serveru</h1> <br> Současný NTP server: " + ntpServerName + "<br> Časová zóna: " + timeZone + "<br> Pozor na přesnost adresy!  <br> <form action='/ntpmessage' method='POST'> NTP adresa: <input type='text' name='aNTP'/> <br> Časová zóna (ČR = 2) <input type='number' name='aTzone' min='-10' max='10'/> <input type='submit' value='Nastavit'/></form> <br> <a href='/main'><button>Zpět domů</button></a>");
    }
    server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
}

void handleNTPMessage(){
  if(tmpLogin == login && tmpPassword == password){
    if(server.hasArg("aNTP") && server.hasArg("aTzone")){
      if(server.arg("aNTP") != NULL){
        ntpServerName = server.arg("aNTP");
        if(TestParseToInt(server.arg("aTzone"))!= 999)
        {
              timeZone = TestParseToInt(server.arg("aTzone"));
              server.send(200, "text/html", "<script type='text/javascript'> window.location = '/ntp'; </script>");
              return;
        }
      }
      server.send(400, "text/html", "<meta charset='UTF-8'> 400: Invalid Request <br> <a href='/ntp'><button>Vrátit zpět</button>");
    }
  }
  server.send(401, "text/html", "<meta charset='UTF-8'> 401: Neautorizován <br> <a href='/'><button>Vrátit zpět</button>");
};

void handleDixi(){
 if(tmpLogin == login && tmpPassword == password){
    dixiON ^= true;
    server.send(200, "text/html", "<script type='text/javascript'> window.location = '/main'; </script>");
  }
}

void SetNX(uint8_t NumLeftPair, char Dots, uint8_t NumRightPair)
{
  uint8_t NumNXSet[2]; //[2]
  NumNXSet[0] = ((NumLeftPair % 10) << 4) | (NumLeftPair / 10);
  NumNXSet[1] = ((NumRightPair % 10) << 4) | (NumRightPair / 10);
  
  //PCB bug correction - exchanged 0 and 1
  if((NumLeftPair % 10) == 1)
      NumNXSet[0] &= ~16;
  if((NumLeftPair % 10) == 0)
      NumNXSet[0] |= 16;
  if((NumLeftPair / 10) == 1)
      NumNXSet[0] &= ~1;
  if((NumLeftPair / 10) == 0)
      NumNXSet[0] |= 1;

  if((NumRightPair % 10) == 1)
      NumNXSet[1] &= ~16;
  if((NumRightPair % 10) == 0)
      NumNXSet[1] |= 16;
  if((NumRightPair / 10) == 1)
      NumNXSet[1] &= ~1;
  if((NumRightPair / 10) == 0)
      NumNXSet[1] |= 1;

  NX595.setAll(NumNXSet); //Sends bytes
  
  //Blinking dots
  StateN = !StateN;
  if(((dixiON == true) && TimeDate > 0) || (dixiON == false))
    StateN = true;

  if(StateN)
      {
      ClockDots(Dots);
      StatusLedOn();
      }
  else
      {
      StatusLedOff();
      ClockDots(' ');        //Sets dots
      }
}

void SerialUpdate()
{
  Serial.write(27);       // ESC command
  Serial.print("[2J");    // clear screen command
  Serial.write(27);
  Serial.print("[H");     // cursor to home command

  Serial.print("Buzzer:\t");
  Serial.print(EIO595.get(0));
  Serial.print("\tRele:\t");
  Serial.print(EIO595.get(1));
  Serial.print("\tStatus:\t");
  Serial.print(EIO595.get(2));
  Serial.print("\tLED top:");
  Serial.print(EIO595.get(3));
  Serial.println("");

  Serial.print("LED bottom:\t");
  Serial.print(EIO595.get(4));
  Serial.print("\tMul_A0:\t");
  Serial.print(EIO595.get(5));
  Serial.print("\tMul_A1:\t");
  Serial.print(EIO595.get(6));
  Serial.print("\tMul_A2:\t");
  Serial.print(EIO595.get(7));
  Serial.println("");

  Serial.print("VN_U:\t");
  Serial.print(GetSignal(VN_U));
  Serial.print("V\tVN_I:\t");
  Serial.print(GetSignal(VN_I));
  Serial.print("V\tOV_OC:\t");
  Serial.print(GetSignal(OC_OV));
  Serial.print("V\tNoise_AD:\t");
  Serial.print(GetSignal(NoiseAD));
  Serial.println("V");

  Serial.print("PWR_Status:\t");
  Serial.print(GetSignal(PWR_Status));
  Serial.print("V\tBtn_1:\t");
  Serial.print(GetSignal(Btn1));
  Serial.print("V\tBtn_2:\t");
  Serial.print(GetSignal(Btn2));
  Serial.print("V\tUCC_U:\t");
  Serial.print(GetSignal(UCC_U));
  Serial.println("V");
}

void VnOn()
{
  digitalWrite(VN_EN_Pin, 1);
}

void VnOff()
{
  digitalWrite(VN_EN_Pin, 0);
}

void ClockDots(char Dots)
{
  switch (Dots)
  {
  case '\'':
      EIO595.setNoUpdate(3,1);
      EIO595.setNoUpdate(4,0);
      EIO595.updateRegisters();
      break;

  case '.':
      EIO595.setNoUpdate(3,0);
      EIO595.setNoUpdate(4,1);
      EIO595.updateRegisters();
      break;

  case ':':
      EIO595.setNoUpdate(3,1);
      EIO595.setNoUpdate(4,1);
      EIO595.updateRegisters();
      break;
  
  default:
      EIO595.setNoUpdate(3,0);
      EIO595.setNoUpdate(4,0);
      EIO595.updateRegisters();
      break;
  }
}

double ReadSignal(Signals MulSig)
{
  //Read input settings
  EIO595.setNoUpdate(5, (uint8_t)MulSig & 1);
  EIO595.setNoUpdate(6, ((uint8_t)MulSig >> 1) & 1);
  EIO595.setNoUpdate(7, ((uint8_t)MulSig >> 2) & 1);
  EIO595.updateRegisters();
  return ((3.2/1023.0*(double)analogRead(AnalogPin))-0.0); //Convert to voltage
}

double GetSignal(Signals Sig)
{
  return RSignals[(uint8_t)Sig];
}

void BuzzerOn()
{
  EIO595.set(0,1);
}

void BuzzerOff()
{
  EIO595.set(0,0);

}

void ReleOn()
{
  EIO595.set(1,1);
}

void ReleOff()

{
  EIO595.set(1,0);
}

void StatusLedOn()
{
  EIO595.set(2,1);
}

void StatusLedOff()
{
  EIO595.set(2,0);
}

void FastLoop()
{
  //Read inputs everytime
  RSignals[ReadRotate] = ReadSignal((Signals)ReadRotate); //Reads all the inputs
  if(dixiON)
    VnOn();
  else
    VnOff();
  if(GetSignal(Btn2) > 2.0){
    isAlarmSet = false;
    BuzzerOff();
  }
    
  ++ReadRotate;
  ReadRotate  %= 8;
  ++LoopCounter;
  if (!(LoopCounter % SlowLoopMul))
      SlowLoop();
}

void SlowLoop()
{
  //Slow loop
  if(GetSignal(Btn1) > 2.0)
    TimeDate = 5; //5 cycles change to variable
  if(!TimeDate)
    SetNX(hour(), ':', minute());
  else
    SetNX(day(), '.', month());

  if((hour() == aintHours) && (minute() >= aintMinutes) && (isAlarmSet == true)){
    BuzzerOn();
    if(minute() == aintMinutes + 1){
      isAlarmSet = false;
      BuzzerOff();
    }
      
  }
  
  --TimeDate;
  if (TimeDate >= 250)
    TimeDate = 0;
}
/*
 * TimeNTP_ESP8266WiFi.ino
 * Example showing time sync to NTP time source
 *
 * This sketch uses the ESP8266WiFi library
 */

#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>


ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();
void handleLogin();
void handleSetTime();
void handleSetMessage();
void handleAlarm();
void handleAlarmMessage();

String iHours = "1";
String iMinutes = "1";
String iSeconds = "1";
String iDays = "1";
String iMonths = "1";
String iYears = "1";
String aHours = "1";
String aMinutes = "1";

int intHours = 2;
int intMinutes = 2;
int intSeconds = 2;
int intDays = 2;
int intMonths = 2;
int intYears = 2;
int aintHours = 2;
int aintMinutes = 2;


bool isNtpSet = false;
bool isAlarmSet = false;


const char ssid[] = "TP-LINK";  //  your network SSID (name)
const char pass[] = "Regulace150";       // your network password

// NTP Servers:
static const char ntpServerName[] = "ntp.cesnet.cz";
//static const char ntpServerName[] = "cz.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

const int timeZone = 2;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)


WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void setup()
{
  Serial.begin(115200);
  //while (!Serial) ; // Needed for Leonardo only
  delay(250);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(600);


  server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/login", HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/setmessage",HTTP_POST,handleSetMessage);
  server.on("/settime", HTTP_GET, handleSetTime); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/alarm", HTTP_GET, handleAlarm); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/alarmmessage",HTTP_POST,handleAlarmMessage);
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
  server.handleClient(); 
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }

  if((hour() == aintHours) && (minute() >= aintMinutes) && (isAlarmSet == true)){
    Serial.println("Budicek");
    Serial.println(hour());
    Serial.println(minute());
    Serial.println(aintHours);
    Serial.println(aintMinutes);
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!KONEC!!!!!!!!!!!!!!!!!!!!!!!!");

    //if(tl === 1) isAlarmSet = false;
    if(minute() == aintMinutes + 1)
      isAlarmSet = false;
  }
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
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
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


void handleRoot() {                          // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Uzivatel\"></br><input type=\"password\" name=\"password\" placeholder=\"Heslo\"></br><input type=\"submit\" value=\"Login\"></form><p>Zkus 'admin' a heslo 'admin' ...</p>");
}

void handleLogin() {                         // If a POST request is made to URI /login
  if( ! server.hasArg("username") || ! server.hasArg("password") 
      || server.arg("username") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }

  if(server.arg("username") == "admin" && server.arg("password") == "admin") { // If both the username and the password are correct
    server.send(200, "text/html", "<meta charset='UTF-8'> <style>.dot{width:16px;height:16px;background-color:gray;border-radius:50%;display:inline-block}.dot.red{background-color:red}.dot.green{background-color:green}</style> <h1>Welcome, " + server.arg("username") + "!</h1><p> successful</p> <br> <a href='/settime'><button>Nastavení času</button> </a> <br> <a href='/alarm'><button>Nastavení budíčku</button> </a><div style='position: absolute; right: 64px; top: 0px;'>NTP: <span class='" + (isNtpSet ? "green" : "red") + " dot'></span></div>");
  } else {                                                                              // Username and password don't match
    server.send(401, "text/plain", "401: Unauthorized");
  }
}





void handleSetTime(){
 server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Čas, !</h1><p> Time: " + iHours + " " + iMinutes + " " + iSeconds + " " + iDays + " " + iMonths + " " + iYears + "</p> <a href='/'><button>Go to home</button></a> <form action='/setmessage' method='POST'> h: <input type='text' name='fHours'/> m: <input type='text' name='fMinutes'/>  s: <input type='text' name='fSeconds'/> <br> d: <input type='text' name='fDays'/> m: <input type='text' name='fMonths'/> y: <input type='text' name='fYears'/> <input type='submit' value='Submit'/> </form>");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}


void handleSetMessage(){
  if(server.hasArg("fHours") && server.hasArg("fMinutes") && server.hasArg("fSeconds") && server.hasArg("fDays") && server.hasArg("fMonths") && server.hasArg("fYears")){
    iHours = server.arg("fHours");
    iMinutes = server.arg("fMinutes");
    iSeconds = server.arg("fSeconds");
    iDays = server.arg("fDays");
    iMonths = server.arg("fMonths");
    iYears = server.arg("fYears");

    intHours = iHours.toInt();
    intMinutes = iMinutes.toInt();
    intSeconds = iSeconds.toInt();
    intDays = iDays.toInt();
    intMonths = iMonths.toInt();
    intYears = iYears.toInt();

    setTime(intHours,intMinutes,intSeconds,intDays,intMonths,intYears);
  }
  server.send(200, "text/html", "<script type='text/javascript'> window.location = '/settime'; </script>");
}


void handleAlarm(){
 server.send(200, "text/html", "<style>.dot{width:16px;height:16px;background-color:gray;border-radius:50%;display:inline-block}.dot.red{background-color:red}.dot.green{background-color:green}</style> <meta charset='UTF-8'> <h1>Budíček!</h1> <p> Alarm: <br>" "h:"+ aHours + "  m:" + aMinutes + "</p> <a href='/'><button>Go to home</button></a> <form action='/alarmmessage' method='POST'> h: <input type='text' name='afHours'/> m: <input type='text' name='afMinutes'/> <input type='submit' value='Submit'/> </form> <div style='position: absolute; right: 64px; top: 0px;'>Budíček: <span class='" + (isAlarmSet ? "green" : "red") + " dot'></span></div>");
}

void handleAlarmMessage(){
  if(server.hasArg("afHours") && server.hasArg("afMinutes")){
    aHours = server.arg("afHours");
    aMinutes = server.arg("afMinutes");
 
    aintHours = aHours.toInt();
    aintMinutes = aHours.toInt();
    isAlarmSet = true;
  }
  
  server.send(200, "text/html", "<script type='text/javascript'> window.location = '/alarm'; </script>");
}
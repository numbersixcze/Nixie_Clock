
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266WebServer.h>   // Include the WebServer library


ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();
void handleLogin();
void handleBudicek();
void handleMessage();
String test = "♥";

void setup(void){
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  WiFi.begin("TP-LINK", "Regulace150");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

  server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/login", HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/message",HTTP_POST,handleMessage);
  server.on("/budicek", HTTP_GET, handleBudicek); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();                    // Listen for HTTP requests from clients
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
    server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Welcome, " + server.arg("username") + "!</h1><p> successful</p>");
  } else {                                                                              // Username and password don't match
    server.send(401, "text/plain", "401: Unauthorized");
  }
}


void handleBudicek(){

  server.send(200, "text/html", "<meta charset='UTF-8'> <h1>Budicek, !</h1><p> successful: " + test + "</p> <a href='/'><button>Go to home</button></a> <form action='/message' method='POST'> <input type='text' name='message2'/> <input type='submit' value='♥'/> </form>");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void handleMessage(){
  if(server.hasArg("message2")){
    test = server.arg("message2");
    Serial.println(test);
  }
  server.send(200, "text/html", "<script type='text/javascript'> window.location = '/budicek'; </script>");

}


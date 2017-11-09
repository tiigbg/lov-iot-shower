#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "passwords.h"



ESP8266WebServer server(80);



const int led = BUILTIN_LED;

const char webpage[] = 
"<!DOCTYPE html>"
"<html>"
"<body>"
"<form action=\"/update_wifi.php\">"
"   <p>Fyll i vilket nätverk din sensor ska koppla sig till.</p>"
"  wifi:<br>"
"  <input type=\"text\" name=\"wifi\" >"
"  <br/>"
"  Lösenord:<br/>"
"  <input type=\"text\" name=\"password\">"
"  <br/><br/>"
"  <input type=\"submit\" value=\"Connect\">"
"</form>"
"</body>"
"</html>";



void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/html", webpage);
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);


  startHostingWifi();
  //connectToWifi();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);

  server.on("/update_wifi.php", [](){
    
    // server.arg(0); // wifi
    // server.arg(1); // password
    server.send(200, "text/plain", "Connecting to wifi..");
    
    char newSsid[101];
    char newPassword[101];
    server.arg(0).toCharArray(newSsid, 101);
    server.arg(1).toCharArray(newPassword, 101);
    Serial.println(server.arg(0));
    Serial.print("Received this as new ssid:");
    Serial.println(newSsid);

    WiFi.disconnect();
    delay(200);
    connectToWifi(newSsid, newPassword);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void connectToMainWifi() {
  //connectToWifi(ssid, password);
}

void connectToWifi(char* newSsid, char* newPassword) {
  WiFi.begin(newSsid, newPassword);
  Serial.print("Going to connect to ");
  Serial.println(newSsid);
  
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(newSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  
}

void startHostingWifi() {
  /*IPAddress ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 1, 1);

  WiFi.config(ip, gateway, subnet, dns);*/
  
  WiFi.softAP("shower-sensor");
    
}

void loop(void){
  server.handleClient();
}

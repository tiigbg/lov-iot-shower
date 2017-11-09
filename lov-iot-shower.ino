#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "passwords.h"



ESP8266WebServer server(80);



const int led = BUILTIN_LED; // low will turn it on

// == vars for waterflow sensor ==

byte sensorInterrupt = D2;  // 1 = digital pin 2 on leonardo
byte sensorPin       = D2;
// The hall-effect flow sensor outputs approximately 11 pulses per second per
// litre/minute of flow.
float calibrationFactor = 11;
volatile byte pulseCount;  
float flowRate;
unsigned int frac;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;

// == vars for temperature sensor ==

float temp;
#define ONE_WIRE_BUS D3
OneWire ds(ONE_WIRE_BUS); //  will handle the onewire protocol
DallasTemperature sensors(&ds); // will handle the temperature monitoring devices


// == vars for webb stuff ==

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




// == setup ==

void setup(void){
  Serial.begin(115200);
  
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
  temp              = 0;

  // catch an interrupt when we get a puls from the water flow sensor
  attachInterrupt(sensorInterrupt, pulseCounter, RISING);
  // temperature sensor
  sensors.begin();

  startHostingWifi();
  //connectToWifi();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);

  server.on("/update_wifi.php", handleWifiUpdate);
  server.on("/status", handleStatusPage);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

// == Loop ==

void loop(void){
  server.handleClient();

  if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);


    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);
    Serial.print("Temp: ");
    Serial.print(temp);

    
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    
    // Print the flow rate for this second in litres / minute
    Serial.print(" Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(".");             // Print the decimal point
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");
    // Print the number of litres flowed in this second
    Serial.print("  Current Liquid Flowing: ");             // Output separator
    Serial.print(flowMilliLitres);
    Serial.print("mL/Sec");

    // Print the cumulative total of litres flowed since starting
    Serial.print("  Output Liquid Quantity: ");             // Output separator
    Serial.print(totalMilliLitres);
    Serial.println("mL"); 

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, RISING);
  }
}

// == wifi functions == 
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

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/html", webpage);
  digitalWrite(led, 0);
}

void handleWifiUpdate() {
  // server.arg(0); // wifi
  // server.arg(1); // password
  server.send(200, "text/plain", "Connecting to wifi..");
  
  char newSsid[101];
  char newPassword[101];
  server.arg(0).toCharArray(newSsid, 101);
  server.arg(1).toCharArray(newPassword, 101);
  
  WiFi.disconnect();
  delay(200);
  connectToWifi(newSsid, newPassword);
}

void handleStatusPage() {
  String statusPage = "<html><head><meta http-equiv=\"refresh\" content=\"1\"></head><body>";

  statusPage += "Temp: ";
  statusPage += temp;
  statusPage += " Flow rate: ";
  statusPage += int(flowRate);  // Print the integer part of the variable
  statusPage += ".";             // Print the decimal point
  // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
  frac = (flowRate - int(flowRate)) * 10;
  statusPage += frac;      // Print the fractional part of the variable
  statusPage += "L/min";
  // Print the number of litres flowed in this second
  statusPage += "  Current Liquid Flowing: ";             // Output separator
  statusPage += flowMilliLitres;
  statusPage += "mL/Sec";

  // Print the cumulative total of litres flowed since starting
  statusPage += "  Output Liquid Quantity: ";             // Output separator
  statusPage += totalMilliLitres;
  statusPage += "mL"; 
  
  statusPage += "</body></html>";
  
  server.send(200, "text/html", statusPage);
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

// == water functions ==

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}




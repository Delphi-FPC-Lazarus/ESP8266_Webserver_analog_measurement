/*
  Arduino IDE Project for ESP8266 Board

  This is an simple project for ESP8266 (NodeMCU 1.1 ESP-12E) Module. 
  Webserver providing a few Endpoints for analog meassurement from adc (TC-9520256 / TA12-200)
  for sinusoidal power consumption 
  
  by Peter Lorenz
  support@peter-ebe.de
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "ESPpowermeter_measurement.h"
#include "WLANZUGANGSDATEN.h"

const char* ssid = STASSID;
const char* password = STAPSK;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 240);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);   //optional
IPAddress secondaryDNS(192, 168, 1, 1); //optional

ESP8266WebServer server(80);

// --------------------------------------------
// Handler Webserver 
// --------------------------------------------

void handleRoot() {
  Serial.println("handleRoot()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = "esp8266 powermeter online\r\n";

  message += "Connected to ";
  message += String(ssid);
  message += "\r\n";
  message += "IP address: ";
  message += String(WiFi.localIP().toString());
  message += "\r\n";

  server.send(200, "text/plain", message);

  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleDebug() {
  Serial.println("handleDebug()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = String(getValuesDebug());

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleCurrentMeasurement() {
  Serial.println("handleCurrentMeasurement()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = String(getMeasurement());

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleCurrentPower() {
  Serial.println("handleCurrentPower");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  int value = getMeasurement();
  Serial.println("Value:");
  Serial.println(value);
  float power = float(CALCURRENTPOWER)/float(CALCURRENTVALUE) * value;
  Serial.println("Power");
  Serial.println(power);

  String message = String(power);
  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleAvgDo() {
  Serial.println("handleAvgDo");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  // nur das Flag setzen
  avgActive = true;

  String message = "OK";
  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH  
}

void handleAvgMeasurement() {
  Serial.println("handleAvgMeasurement()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = String(avgValue);

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleAvgPower() {
  Serial.println("handleAvgPower");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  int value = avgValue;
  Serial.println("Value:");
  Serial.println(value);
  float power = float(CALCURRENTPOWER)/float(CALCURRENTVALUE) * value;
  Serial.println("Power");
  Serial.println(power);

  String message = String(power);
  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

// --------------------------------------------
// Wifi 
// --------------------------------------------

void startWifi(void) {
  Serial.println("StartWifi");

  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Connect
  WiFi.begin(ssid, password);
  Serial.println("Connecting Wifi");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void stopWifi(void) {
  Serial.println("stopWifi");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// --------------------------------------------
// Main Loop
// --------------------------------------------

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH

  Serial.begin(115200);
  Serial.println("Serial Monitor");

  // Watchdog
  // Es gibt einen hardware und einen software watchdog
  ESP.wdtDisable();     // Softwarewatchdog aus  
  ESP.wdtEnable(5000);  // Softwarewatchdog auf n sek stellen, achtung wenn ich ihn abgeschaltet lasse l??st aller 6-7 Sekunden der Hardware watchdog aus

  // Wifi
  startWifi();

  // MDNS
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Server
  server.on("/", handleRoot);

  server.on("/debug", handleDebug);

  server.on("/currentMeasurement", handleCurrentMeasurement);
  server.on("/currentPower", handleCurrentPower);

  // Mittelwertmessung ausf??hren
  server.on("/avgDo", handleAvgDo);
  // Mittelwertmessung abfragen
  server.on("/avgFetchMeasurement", handleAvgMeasurement);
  server.on("/avgFetchPower", handleAvgPower);
 
  /*
  server.on("/test", []() {
    server.send(200, "text/plain", );
  });
  */

  server.begin();
  Serial.println("HTTP server started");
}

// --------------------------------------------

void loop(void) {
  // Main Loop
  // der server.handleClient() muss permanent aufgerufen werden
  // Blockieren l??nger als 20ms wird nicht empfohlen

  MDNS.update();
  server.handleClient();

  if (avgActive == true ) {
    avgActive = false;
    Serial.println("enter main loop hook");
 
    //delay(1000); // yield() im Hintergrund, abarbeitung 
    //stopWifi();

    // f??hrt zur Wifi Unterbrechung
    // wenn reconect gesetzt verbindet er automatisch 
    // ansonsten m??sste hier das wifi manuell gestoppt und neu gestartet werden
    doAvgMeasurement();

    //startWifi();

    Serial.println("leave main loop hook");
  }

}

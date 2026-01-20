/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP32 chip.

  Note: This requires ESP32 support package:
    https://github.com/espressif/arduino-esp32

  Please be sure to select the right ESP32 module
  in the Tools -> Board menu!

  Change WiFi ssid (line 62), pass (line 63), and Blynk auth token (line 58) to run :)
  If you're using custom server, also change IPAddress (line 135). 
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill-in your Template ID (only if using Blynk.Cloud) */
//#define BLYNK_TEMPLATE_ID   "YourTemplateID"

#include <WiFi.h>
#include <WiFiClient.h>

// Include WebServer FIRST to prevent conflicts
#include <WebServer.h>

#include <BlynkSimpleEsp32.h>

//Arduino OTA
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// WiFiManager - include after WebServer
#include <WiFiManager.h> // WiFi Configuration Captive Portal

//webSerialMonitor - include after WiFiManager
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

// Secrets
#include "ESP32_secrets.h"

// Add printf functionality to WebSerial
#include "WebSerial_printf.h"

// Dual Printf to Serial Monitor and WebSerial Monitor
#include "dual_printf.h"

#include <Preferences.h> // Native ESP32 library for saving data to flash memory.

Preferences preferences;
WiFiManager wm;

// Blynk credentials variables
char blynk_auth[34] = Secret_AUTH;
char blynk_server[40] = Secret_IP; 
char blynk_port[6]  = Secret_PORT;

// Blynk connection tracking
int blynk_connection_attempts = 0;
const int BLYNK_MAX_ATTEMPTS = 5;
bool has_provisioned_blynk = false;

AsyncWebServer server(80);
int LED = 2;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = Secret_AUTH;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = Secret_SSID; 
char pass[] = Secret_PASS;

// Motor 1 (Left)
#define motorL_Negative 22
#define motorL_Positive 23 
#define motorL_EN 21    

// Motor 2 (Right)
#define motorR_Negative 27
#define motorR_Positive 26 
#define motorR_EN 25

// Analog speeds from 0 (lowest) - 1023 (highest).
// 3 speeds used -- 0 (noSpeed), 750 (minSpeed), 1023 (maxSpeed).
// Use whatever speeds you want...too fast made it a pain in the ass to control.
int minSpeed = 750;
int maxSpeed = 1023;
int noSpeed = 0;

// Neutral zone settings for x and y.
// Joystick must move outside these boundary numbers (in the blynk client app) to activate the motors.
// Makes it a easier to control the tank.
int minRange = 312;
int maxRange = 712;
int minNeuRange = 412;
int maxNeuRange = 612;

// Directional Guard Declarations
enum YRegion { Y_NEUTRAL, Y_FORWARD, Y_BACKWARD };
// Tracks the last committed FWD/REV state (X-axis)
YRegion lastYRegion = Y_NEUTRAL;

// Setting PWM properties for motors.
int freq = 5000; // Was 30000 
int resolution = 10; // Was 10

// Setting up Ultrasonic Sensor.
#include <SR04.h>
#define TRIG_PIN 18
#define ECHO_PIN 19
SR04 sr04 = SR04(ECHO_PIN,TRIG_PIN);
int distance;

// Timer to help run Ultrasonic button state check.
BlynkTimer timer;
void uSonicButtonCheck();
int uSonicState;
bool isHaltedByUSonic = false;
int ultrasonicLimit = 40; // Default value in cm

// Lights.
#define lights 16 // Signal to NPN2 Transistor.
WidgetLED ledIndicator(V11);

// Control ultrasonic NPN1 Transistor.
#define currentToUSonic 5

// Pin for battery voltage.
#define batteryVoltagePin 35
void readBatteryVoltage();

// Battery Parameters.
const float maxBatteryVoltage = 16.68;  // Max voltage for a 18650 3.7v Li-ion cell. 4.2V/cell. 4.17V/cell measured.
const float minBatteryVoltage = 12;  // Min voltage for a 18650 3.7 Li-ion cell. 3V/cell.
// Voltage Divider Parameters.
const float R1 = 41860.0;  // Resistor R1 in voltage divider (42kΩ), multimeter measured value provided.
const float R2 = 9989.0;  // Resistor R2 in voltage divider (10kΩ), multimeter measured value provided.
// Factor to correct for ADC inaccuracies.
const float calibrationFactor = 1.025; // = Voltage reading from multimeter / Voltage after voltage divider.

// Function not needed.
// Arduino like analogWrite.
// Value has to be between 0 and valueMax.
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 4095 from 2 ^ 12 - 1
  uint32_t duty = (1023 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

// Print to both Serial Monitor and WebSerial Monitor.
template <typename T>
void dualPrint(T data) {
  Serial.print(data);
  WebSerial.print(data);
}

template <typename T>
void dualPrintln(T data) {
  Serial.println(data);
  WebSerial.println(data);
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  
  // Give serial time to initialize and wait for monitor to connect
  delay(2000);
  Serial.println("\n\n=== TANK BOOTING ===");
  Serial.println("Initializing systems...");

  // --- 1. Load Custom Params (Blynk) from Flash ---
  Serial.println("Loading preferences...");
  preferences.begin("tank_config", false);
  String savedAuth = preferences.getString("auth", ""); 
  String savedServer = preferences.getString("server", "");
  String savedPort = preferences.getString("port", "");
  
  // Use saved values if available, otherwise use Secret defaults
  if (savedAuth != "") savedAuth.toCharArray(blynk_auth, 34);
  if (savedServer != "") savedServer.toCharArray(blynk_server, 40);
  if (savedPort != "") savedPort.toCharArray(blynk_port, 6);
  
  Serial.printf("Loaded Blynk Server: %s:%s\n", blynk_server, blynk_port);
  Serial.printf("Loaded Blynk Auth: %.4s...\n", blynk_auth);
  
  preferences.end();
  
  // --- 2. WiFi CONNECTION ATTEMPTS ---
  Serial.println("\n--- WiFi Connection Phase ---");
  Serial.println("Attempting to connect to saved Wi-Fi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(); 

  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 5) {
    delay(12000);
    Serial.print(".");
    wifi_attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    // WiFi FAILED: Launch combined provisioner for WiFi + Blynk credentials
    Serial.println("\nWiFi connection failed. Launching provisioner for WiFi and Blynk configuration...");
    launchCombinedProvisioner();
    
    if (!WiFi.isConnected()) {
      Serial.println("WiFi provisioning failed. Restarting...");
      delay(3000);
      ESP.restart();
    }
  }
  
  Serial.println("\nWiFi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // --- 3. NOW SETUP OTA AND WEBSERIAL (after WiFi is established) ---
  Serial.println("\n--- Initializing Services ---");
  Serial.println("Setting up Arduino OTA...");
  OTA_Setup();
  
  Serial.println("Setting up WebSerial...");
  WebSerial_Setup();
  Serial.println("WebSerial ready");

  // --- 4. ATTEMPT BLYNK CONNECTION ---
  Serial.println("\n--- Blynk Connection Phase ---");
  
  // Convert port char array to integer for the function
  int port_int = atoi(blynk_port);
  
  Serial.print("Attempting Blynk Server: ");
  Serial.print(blynk_server);
  Serial.print(":");
  Serial.println(port_int);

  // IP and Port of Blynk Server.
  Blynk.config(blynk_auth, blynk_server, port_int);
  
  // Try to connect to Blynk server
  int blynk_startup_attempts = 0;
  while (!Blynk.connected() && blynk_startup_attempts < 5) {
    Serial.print(".");
    Blynk.connect(4000); // 4 second timeout per attempt
    blynk_startup_attempts++;
  }
  
  if (!Blynk.connected()) {
    Serial.println("\nBlynk server connection failed. Launching Blynk Provisioner...");
    launchBlynkProvisioner();
    
    // Try to reconnect with new credentials
    port_int = atoi(blynk_port);
    Blynk.config(blynk_auth, blynk_server, port_int);
    
    blynk_startup_attempts = 0;
    while (!Blynk.connected() && blynk_startup_attempts < 5) {
      Serial.print(".");
      Blynk.connect(4000);
      blynk_startup_attempts++;
    }
    
    if (!Blynk.connected()) {
      Serial.println("\nBlynk provisioning failed. Restarting...");
      delay(3000);
      ESP.restart();
    }
  }
  
  Serial.println("\nBlynk connected successfully!");

  pinMode(2, OUTPUT);
  
  pinMode(motorL_Negative, OUTPUT);
  pinMode(motorL_Positive, OUTPUT);
  pinMode(motorL_EN, OUTPUT);
  pinMode(motorR_Negative, OUTPUT);
  pinMode(motorR_Positive, OUTPUT);
  pinMode(motorR_EN, OUTPUT);

  pinMode(lights, OUTPUT);

  pinMode(currentToUSonic, OUTPUT);

  pinMode(batteryVoltagePin, INPUT);
  
  
  // Attach PWM functionalitites via ledc to the GPIO to be controlled.
  ledcAttach(motorL_EN, freq, resolution);
  ledcAttach(motorR_EN, freq, resolution);
  ledcAttach(lights, 1000, resolution);

  // Blink Lights.
  ledcWrite(lights, 0);
  delay(250);
  ledcWrite(lights, 511);
  delay(250);
  ledcWrite(lights, 0);
  delay(250);
  ledcWrite(lights, 766);
  delay(250);
  ledcWrite(lights, 0);

  // Set a function to be called every 50ms.
  timer.setInterval(50L, uSonicButtonCheck); 

  // Set a function to be called every 1s.
  timer.setInterval(1000L, readBatteryVoltage);

  // Set default value for ultrasonic limit in app.
  Blynk.virtualWrite(V8, ultrasonicLimit); 
}

void loop()
{
  ArduinoOTA.handle(); // Listen for Arduino code upload OTA.

  // Check if WiFi is connected before attempting to connect to Blynk
  if (WiFi.status() == WL_CONNECTED) {
    // If not connected to Blynk, try to connect
    if (!Blynk.connected()) {
      blynk_connection_attempts++;
      
      // If we've failed too many times and not already provisioning, launch provisioner
      if (blynk_connection_attempts >= BLYNK_MAX_ATTEMPTS && !has_provisioned_blynk) {
        Serial.println("Blynk connection failed multiple times. Launching Blynk Provisioner...");
        has_provisioned_blynk = true; // Prevent repeated provisioning attempts
        launchBlynkProvisioner();
        
        // Reconfigure and try again
        int port_int = atoi(blynk_port);
        Blynk.config(blynk_auth, blynk_server, port_int);
        blynk_connection_attempts = 0; // Reset counter
      }
      
      // Blink onboard led on attempts to connect to blynk server.
      digitalWrite(2, HIGH); 
      delay(500);
      digitalWrite(2, LOW);
      delay(500);
      Serial.println("Attempting to connect to Blynk server...");
      Blynk.connect(5000); // 5 sec timeout
    }
    // If connected, run the Blynk routine
    else {
      blynk_connection_attempts = 0; // Reset on successful connection
      Blynk.run();
      timer.run(); // Initiates BlynkTimer.
    }
  }
}

BLYNK_CONNECTED() {
    Blynk.syncAll(); // Sync blynk client app with blynk server to recall last values.
}

// Combined provisioner for WiFi + Blynk on initial WiFi connection failure
void launchCombinedProvisioner() {
  Serial.println("\n=== WiFi + Blynk Configuration Portal ===\n");
  
  // Combine server and port for display
  char server_port_combined[46];
  snprintf(server_port_combined, sizeof(server_port_combined), "%s:%s", blynk_server, blynk_port);
  
  // Create custom parameters for Blynk config - separate server:port and auth
  WiFiManagerParameter custom_blynk_server("blynk_server", "Blynk Server (IP:PORT)", server_port_combined, 40);
  WiFiManagerParameter custom_blynk_auth("blynk_auth", "Blynk Auth Token", blynk_auth, 34);
  
  // Add custom parameters to WiFiManager
  wm.addParameter(&custom_blynk_server);
  wm.addParameter(&custom_blynk_auth);
  
  // Set callback for when config is saved
  wm.setSaveConfigCallback([]() {
    Serial.println("Config saved - parsing Blynk settings...");
  });
  
  Serial.println("Starting WiFi Configuration Portal...");
  Serial.println("Connect to AP: Tank-Setup");
  Serial.println("Open browser to: http://192.168.4.1\n");
  Serial.println("Enter:");
  Serial.println("  - WiFi SSID");
  Serial.println("  - WiFi Password");
  Serial.println("  - Blynk Server: IP:PORT format");
  Serial.println("    Example: 192.168.1.200:8080");
  Serial.println("  - Blynk Auth Token");
  Serial.println("    Example: YourAuthTokenHere\n");
  
  // Set timeout and start portal
  wm.setConfigPortalTimeout(300); // 5 minutes timeout
  
  // Start the portal with custom AP name and password
  bool res = wm.startConfigPortal("Tank-Setup", "password123");
  
  if (res) {
    Serial.println("\nPortal connection successful!");
    
    // Get the Blynk server config (IP:PORT format)
    String blynkServerConfig = custom_blynk_server.getValue();
    String blynkAuthToken = custom_blynk_auth.getValue();
    
    if (blynkServerConfig.length() > 0) {
      Serial.println("Parsing Blynk configuration...");
      
      // Parse server:port
      int colonPos = blynkServerConfig.indexOf(':');
      
      if (colonPos > 0) {
        String server = blynkServerConfig.substring(0, colonPos);
        String port = blynkServerConfig.substring(colonPos + 1);
        String auth = blynkAuthToken;
        
        Serial.println("Parsed Blynk Configuration:");
        Serial.print("  Server: ");
        Serial.println(server);
        Serial.print("  Port: ");
        Serial.println(port);
        Serial.print("  Auth: ");
        Serial.println(auth);
        
        // Update global variables
        server.toCharArray(blynk_server, 40);
        port.toCharArray(blynk_port, 6);
        auth.toCharArray(blynk_auth, 34);
        
        // Save to preferences
        preferences.begin("tank_config", false);
        preferences.putString("server", server);
        preferences.putString("port", port);
        preferences.putString("auth", auth);
        preferences.end();
      } else {
        Serial.println("Warning: Blynk config format invalid. Using defaults from secrets.");
      }
    } else {
      Serial.println("No Blynk config provided. Using defaults from secrets.");
    }
  } else {
    Serial.println("Portal timeout or failed!");
  }
}

// Blynk-only provisioner for subsequent Blynk connection failures
void launchBlynkProvisioner() {
  Serial.println("\n=== Blynk Server Configuration Portal ===\n");
  
  // Temporarily stop the main AsyncWebServer to free port 80
  Serial.println("Stopping WebSerial server to free port 80...");
  server.end();
  delay(500);
  
  // Create soft AP for Blynk configuration
  WiFi.softAP("Tank-Blynk-Setup", "password123");
  IPAddress softAPIP = WiFi.softAPIP();
  Serial.printf("Soft AP IP: %s\n", softAPIP.toString().c_str());
  
  // Create a simple AsyncWebServer for Blynk config only (no WiFi fields)
  AsyncWebServer blynkServer(80);
  
  // Serve the Blynk configuration form with current values pre-filled
  blynkServer.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
    // Build current server:port value
    String currentServer = String(blynk_server) + ":" + String(blynk_port);
    String currentAuth = String(blynk_auth);
    
    String html = R"(
      <!DOCTYPE html>
      <html>
        <head>
          <meta name="viewport" content="width=device-width, initial-scale=1">
          <title>Tank Blynk Setup</title>
          <style>
            body { font-family: Arial; text-align: center; padding: 20px; }
            input { padding: 10px; margin: 10px; width: 200px; }
            button { padding: 10px 20px; font-size: 16px; }
          </style>
        </head>
        <body>
          <h1>Tank Blynk Configuration</h1>
          <form action="/save" method="POST">
          <label>Blynk Server (IP:PORT):</label><br>
          <input type="text" name="server" value=")" + currentServer + R"(" required><br>
          <label>Blynk Auth Token:</label><br>
          <input type="text" name="auth" value=")" + currentAuth + R"(" required><br>
          <button type="submit">Save & Restart</button>
          </form>
        </body>
      </html>
    )";
    request->send(200, "text/html", html);
  });
  
  // Handle form submission
  blynkServer.on("/save", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (request->hasParam("server", true) && request->hasParam("auth", true)) {
      String serverParam = request->getParam("server", true)->value();
      String authParam = request->getParam("auth", true)->value();
      
      Serial.println("Blynk config received!");
      Serial.printf("Server: %s\n", serverParam.c_str());
      Serial.printf("Auth: %s\n", authParam.c_str());
      
      // Parse server:port
      int colonPos = serverParam.indexOf(':');
      if (colonPos > 0) {
        String server = serverParam.substring(0, colonPos);
        String port = serverParam.substring(colonPos + 1);
        
        // Update globals
        server.toCharArray(blynk_server, 40);
        port.toCharArray(blynk_port, 6);
        authParam.toCharArray(blynk_auth, 34);
        
        // Save to preferences
        preferences.begin("tank_config", false);
        preferences.putString("server", server);
        preferences.putString("port", port);
        preferences.putString("auth", authParam);
        preferences.end();
        
        request->send(200, "text/html", "<h1>Configuration Saved!</h1><p>Restarting...</p>");
        delay(2000);
        ESP.restart();
      } else {
        request->send(400, "text/html", "<h1>Invalid format!</h1><p>Use IP:PORT</p>");
      }
    }
  });
  
  blynkServer.begin();
  Serial.println("Blynk provisioning server started at http://192.168.4.1");
  Serial.println("Configuration timeout: 5 minutes\n");
  
  // Wait for configuration (5 minutes timeout)
  unsigned long startTime = millis();
  while (millis() - startTime < 300000) {  // 5 minutes
    delay(100);
  }
  
  blynkServer.end();
  WiFi.softAPdisconnect(true);  // Turn off soft AP
  Serial.println("Blynk provisioner timeout - exiting...");
  
  // Restart the main WebSerial server
  Serial.println("Restarting WebSerial server...");
  server.begin();
}

// Joystick control
BLYNK_WRITE(V5)
{
  int x = param[0].asInt();
  int y = param[1].asInt();

  // Primary Check: Halt Override
  if (isHaltedByUSonic) {
    // Override user input and ensure motors are OFF
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,LOW);
    digitalWrite(motorR_Positive,LOW);  
    digitalWrite(motorR_Negative,LOW);
    // Exit the function, ignoring the joystick values (x, y)
    return; 
  }

  // Determine the Current Desired X-Axis Region (Forward/Reverse)
  YRegion currentXRegion; 
  
  // FWD Zones: Full FWD (x >= 712) OR Hard FWD (612 < x < 712)
  if (x >= maxRange || (x > maxNeuRange && x < maxRange)) {
    currentXRegion = Y_FORWARD; 
  }
  // REV Zones: Full REV (x <= 312) OR Hard REV (312 < x < 412)
  else if (x <= minRange || (x > minRange && x < minNeuRange)) {
    currentXRegion = Y_BACKWARD;
  }
  // ABSOLUTE CENTER DEAD ZONE: 412 < x < 612
  else if (x > minNeuRange && x < maxNeuRange) {
    // *** CRITICAL LOCK LOGIC ***
    // While passing through the absolute center, maintain the last committed state.
    // This forces the transition to be checked against Y_FORWARD or Y_BACKWARD, 
    // even if the joystick briefly registers "neutral" input.
    if (lastYRegion == Y_FORWARD || lastYRegion == Y_BACKWARD) {
      currentXRegion = lastYRegion;
    } else {
      currentXRegion = Y_NEUTRAL;
    }
  }
  // Default (should only occur if there is a true error)
  else {
    currentXRegion = Y_NEUTRAL;
  }
  
  // CHECK FOR ILLEGAL TRANSITION
  // If the last committed state was FWD and the current request is BWD, BLOCK IT.
  if ((lastYRegion == Y_FORWARD && currentXRegion == Y_BACKWARD) ||
    (lastYRegion == Y_BACKWARD && currentXRegion == Y_FORWARD)) 
  {
    // Block the move. The tank holds the previous state.
    return; 
  }

  // Call moveControl function.
  moveControl(x,y); 
  
  // For debug purposes.
  /*Serial.print("X = ");
  Serial.print(x);
  Serial.print(" : Y = ");
  Serial.println(y);
  int EN1_Value = ledcRead(motorL_EN);
  int EN2_Value = ledcRead(motorR_EN);
  Serial.printf("EN1_Value = %d\n", EN1_Value);
  Serial.printf("EN2_Value = %d\n", EN2_Value);
  Serial.println();*/
}

void moveControl(int x, int y)
{
  // Movement logic
  // Move forward
  if(x >= maxRange && y >= minRange && y <= maxRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, maxSpeed);
    lastYRegion = Y_FORWARD; // <--- COMMIT FWD STATE
  }
  // Move forward left
  else if(x >= maxRange && y >= maxNeuRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, minSpeed);
    ledcWrite(motorR_EN, maxSpeed);
    lastYRegion = Y_FORWARD; // <--- COMMIT FWD STATE
  }
  // Move hard forward left
  else if(x > maxNeuRange && x < maxRange && y > maxRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, noSpeed);
    ledcWrite(motorR_EN, maxSpeed);
    lastYRegion = Y_FORWARD; // <--- COMMIT FWD STATE
  }
  // Move forward right
  else if(x >= maxRange && y <= minNeuRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, minSpeed);
    lastYRegion = Y_FORWARD; // <--- COMMIT FWD STATE
  } 
  // Move hard forward right
  else if(x > maxNeuRange && x < maxRange && y < minRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, noSpeed);
    lastYRegion = Y_FORWARD; // <--- COMMIT FWD STATE
  } 
  // Neutral zone
  else if(y < maxRange && y > minRange && x < maxRange && x > minRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,LOW);
    digitalWrite(motorR_Positive,LOW);  
    digitalWrite(motorR_Negative,LOW);
    // Check for the ABSOLUTE DEAD CENTER (412-612) to reset the guard
    if (y > minNeuRange && y < maxNeuRange && x > minNeuRange && x < maxNeuRange)
    {
      lastYRegion = Y_NEUTRAL; 
    }
  }
  // Move back
  else if(y >= minRange && y <= maxRange && x <= minRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, maxSpeed);
    lastYRegion = Y_BACKWARD; // <--- COMMIT BWD STATE
  }
  // Move back and right
 else if(y <= minNeuRange && x <= minRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, minSpeed);
    lastYRegion = Y_BACKWARD; // <--- COMMIT BWD STATE
  }
  // Move back and hard right
  else if(x > minRange && x < minNeuRange && y < minRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, noSpeed);
    lastYRegion = Y_BACKWARD; // <--- COMMIT BWD STATE
  }
  // Move back and left
  else if(x <= minRange && y >= maxNeuRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, minSpeed);
    ledcWrite(motorR_EN, maxSpeed);
    lastYRegion = Y_BACKWARD; // <--- COMMIT BWD STATE
  }
  // Move back and hard left
  else if(x > minRange && x < minNeuRange && y > maxRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, noSpeed);
    ledcWrite(motorR_EN, maxSpeed);
    lastYRegion = Y_BACKWARD; // <--- COMMIT BWD STATE
  }
}

// Speed slider
BLYNK_WRITE(V6) 
{
  // Debug
  /*int EN1_Value = ledcRead(motorL_EN);
  int EN2_Value = ledcRead(motorR_EN);
  Serial.printf("EN1_Value = %d\n", EN1_Value);
  Serial.printf("EN2_Value = %d\n", EN2_Value);*/
  
  int pinData = param.asInt();
  int minLimiter = 0;
  int maxLimiter = 1023;

  if (pinData <= minLimiter){ // Min val limiter
    maxSpeed = minLimiter;
    Blynk.virtualWrite(V6, minLimiter);
  }
  else if (pinData > maxLimiter){ // Max val limiter
    maxSpeed = maxLimiter;
    Blynk.virtualWrite(V6, maxLimiter);
  }
  else{
    // Map 1-1023 to 750-1023 for speed control.
    // This eliminates the the deadzone between 0-750.
    maxSpeed = map(pinData, 1, 1023, 750, 1023); 
  }
  
  minSpeed = (75.0/100.0)*maxSpeed; /*Converted to percentage so it
  can work with every resolution 8,10,12,16 etc. ( value - 255,1023,2047,4095) where value is 2^resolution*/

  // Debug
  dualPrint("maxSpeed = ");
  dualPrintln(maxSpeed); 
  dualPrint("minSpeed = ");
  dualPrintln(minSpeed);
  dualPrintf("maxSpeed = %d, minSpeed = %d\n", maxSpeed, minSpeed);
}

// Ultrasonic Sensor button
BLYNK_WRITE(V3) 
{
  uSonicState = param.asInt(); 
}

void uSonicButtonCheck() // Function to check ultrasonic button state. On/Off.
{
  if (uSonicState == HIGH) // If button is on.
  {
    digitalWrite(currentToUSonic, HIGH); // Turns on NPN Transistor to supply power to the sensor.
    distance = sr04.Distance();
    Blynk.virtualWrite(V7, distance); // Display distance in app.

    // Debug
    /*dualPrint("Distance = ");
    dualPrint(distance);
    dualPrintln("cm");*/
    
    if (distance <= ultrasonicLimit) // Prevent tank from crashing into objects.
    {
      // Set the flag to halt Tank
      isHaltedByUSonic = true; 
      
      // Stop the motors immediately
      digitalWrite(motorL_Positive,LOW); 
      digitalWrite(motorL_Negative,LOW);
      digitalWrite(motorR_Positive,LOW);  
      digitalWrite(motorR_Negative,LOW);
      
      // Force joystick to neutral position for visual feedback
      Blynk.virtualWrite(V5, 512, 512); 
    } 
    else {
      // Clear the flag if distance is clear (and sensor is ON)
      isHaltedByUSonic = false;
    }     
  }
  else {
    digitalWrite(currentToUSonic, LOW); // Cut power to NPN Transistor.
    Blynk.virtualWrite(V7, "--"); 
    isHaltedByUSonic = false; // Always clear the flag when the sensor is turned OFF
  } 
}

// Numeric Input to set limit
BLYNK_WRITE(V8) {
  ultrasonicLimit = param.asInt();
  int minLimiter = 20;
  int maxLimiter = 200;   
  if (ultrasonicLimit < minLimiter) {
    ultrasonicLimit = minLimiter; // Minimum limit
    Blynk.virtualWrite(V8, minLimiter);
  }
  else if (ultrasonicLimit > maxLimiter) {
    ultrasonicLimit = maxLimiter; // Maximum limit
    Blynk.virtualWrite(V8, maxLimiter);
  }
}

// Led lights slider
BLYNK_WRITE(V4) 
{
  int pinData = param.asInt(); 
  int minLimiter = 0;
  int maxLimiter = 1023;

  if (pinData < minLimiter){ // Min val limiter
    ledcWrite(lights, minLimiter);
    Blynk.virtualWrite(V4, minLimiter);
    ledIndicator.setValue(0);
  }
  else if (pinData > maxLimiter){ // Max val limiter
    ledcWrite(lights, maxLimiter);
    Blynk.virtualWrite(V4, maxLimiter);
    ledIndicator.setValue(255);
  }
  else{
    ledcWrite(lights, pinData);
    ledIndicator.setValue((float(pinData) / maxLimiter) * 255); // Convert x bit io 8 bit for WidgetLED.
  }
 
  // Debug
  /*int ledValue = ledcRead(lights);
  dualPrint("ledValue = ");
  dualPrintln(ledValue);*/
  
}

// Battery Voltage
void readBatteryVoltage() {
  float sum = 0;
  const int numSamples = 64;

  for (int i = 0; i < numSamples; i++) {
    sum += analogReadMilliVolts(batteryVoltagePin);  // Read in millivolts
    delay(5);
  }

  float averageMilliVolts = sum / numSamples;
  float voltage = (averageMilliVolts / 1000.0);  // Convert mV to V
  //dualPrint("V = ");
  //dualPrintln(voltage);
  voltage = voltage * (R1 + R2) / R2;  // Adjust for voltage divider
  //dualPrint("V-divider = ");
  //dualPrintln(voltage);
  voltage = voltage * calibrationFactor; // Correct differences between multimeter reading and voltage divider.
  //dualPrint("V-califactor = ");
  //dualPrintln(voltage);
  //dualPrintln();

  // Write to Value Display
  Blynk.virtualWrite(V14, voltage);
  //Battery Percentage
  float percentage = (voltage - minBatteryVoltage) / (maxBatteryVoltage - minBatteryVoltage) * 100;
  // Write to Gauge
  Blynk.virtualWrite(V15, constrain(percentage, 0, 100));
}

// Arduino OTA
void OTA_Setup(){
  Serial.println("Booting");
  // WiFi is already configured in setup(), just wait for connection
  // OTA will work once WiFi is connected or after provisioning
  
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Tank-NodeMCU-32S");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// WebSerial Setup function.
void WebSerial_Setup(){
// WebSerial is accessible at "<IP Address>/webserial" in browser
WebSerial.begin(&server);

/* Attach Message Callback */
WebSerial.onMessage([&](uint8_t *data, size_t len) {
  Serial.printf("Received %u bytes from WebSerial: ", len);
  Serial.write(data, len);
  Serial.println();
  WebSerial.println("Received Data...");
  String d = "";
  for(size_t i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
});

// Start server
server.begin();
}
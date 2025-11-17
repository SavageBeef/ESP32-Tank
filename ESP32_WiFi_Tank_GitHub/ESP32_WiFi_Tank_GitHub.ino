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
#include <BlynkSimpleEsp32.h>

//Arduino OTA
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//webSerialMonitor
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

// Secrets
#include "ESP32_secrets.h"

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
  
  // Call OTA_Setup function which provides Arduino OTA.
  OTA_Setup();

  // Call WebSerial Setup function.
  WebSerial_Setup();

  // IP and Port of Blynk Server.
  Blynk.config(auth, IPAddress(Secret_IP), 8080);

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
      // Blink onboard led on attempts to connect to blynk server.
      digitalWrite(2,HIGH); 
      delay(1000);
      digitalWrite(2,LOW);
      delay(1000);
      Serial.println("Attempting to connect to Blynk server...");
      Blynk.connect(5000); // 5 sec timeout, loop continues so Arduino OTA doesn't timeout while waiting for the hardwaare to respond for code upload. 
    }
    // If connected, run the Blynk routine
    else {
      Blynk.run();
      timer.run(); // Initiates BlynkTimer.
    }
  }
}

BLYNK_CONNECTED() {
    Blynk.syncAll(); // Sync blynk client app with blynk server to recall last values.
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
  dualPrintln("");
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

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
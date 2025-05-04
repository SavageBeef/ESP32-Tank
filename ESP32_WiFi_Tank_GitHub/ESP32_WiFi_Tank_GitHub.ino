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

  Change WiFi ssid, pass, and Blynk auth token to run :)
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

AsyncWebServer server(80);
int LED = 2;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = ""; 
char pass[] = "";

// Motor 1 (Left)
#define motorL_Negative 22 //motorL_Negative 23
#define motorL_Positive 23 //motorL_Positive 22 
#define motorL_EN 21    //motorL_EN

// Motor 2 (Right)
#define motorR_Negative 13 //motorR_Negative 12
#define motorR_Positive 12 //motorR_Positive 13
#define motorR_EN 32    //motorR_EN

// Analog speeds from 0 (lowest) - 1023 (highest).
// 3 speeds used -- 0 (noSpeed), 350 (minSpeed), 850 (maxSpeed).
// Use whatever speeds you want...too fast made it a pain in the ass to control.
int minSpeed = 450;
int maxSpeed = 1023;
int noSpeed = 0;

// Neutral zone settings for x and y.
// Joystick must move outside these boundary numbers (in the blynk client app) to activate the motors.
// Makes it a easier to control the tank.
int minRange = 312;
int maxRange = 712;

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

// Lights.
#define lights 16 // Signal to NPN2 Transistor.
WidgetLED ledIndicator(V11);

// Control ultrasonic NPN1 Transistor.
#define currentToUSonic 5

// Function not needed.
// Arduino like analogWrite.
// Value has to be between 0 and valueMax.
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 4095 from 2 ^ 12 - 1
  uint32_t duty = (1023 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  
  // Call OTA_Setup function.
  OTA_Setup();

  // WebSerial is accessible at "<IP Address>/webserial" in browser.
  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  server.begin();

  // IPAddress of your server.
  Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,5), 8080);

  pinMode(2, OUTPUT);
  
  pinMode(motorL_Negative, OUTPUT);
  pinMode(motorL_Positive, OUTPUT);
  pinMode(motorL_EN, OUTPUT);
  pinMode(motorR_Negative, OUTPUT);
  pinMode(motorR_Positive, OUTPUT);
  pinMode(motorR_EN, OUTPUT);

  pinMode(lights, OUTPUT);

  pinMode(currentToUSonic, OUTPUT);
  
  
  // Attach PWM functionalitites via ledc to the GPIO to be controlled.
  ledcAttach(motorL_EN, freq, resolution);
  ledcAttach(motorR_EN, freq, resolution);
  ledcAttach(lights, 1000, resolution);


  // Blink onboard blue led 2x to signal successful connection to wifi and local blynk server.
  digitalWrite(2,HIGH); 
  delay(1000);
  digitalWrite(2,LOW);
  delay(1000);
  digitalWrite(2,HIGH); 
  delay(1000);
  digitalWrite(2,LOW); 


  // Set a function to be called every 50ms.
  timer.setInterval(50L, uSonicButtonCheck); 
}

void loop()
{
  Blynk.run(); // Runs Blynk.
  ArduinoOTA.handle(); // Listen for code upload OTA.
  timer.run(); // Initiates BlynkTimer.
}

BLYNK_CONNECTED() {
    Blynk.syncAll(); // Sync blynk client app with blynk server to recall last values.
}

// Joystick control.
BLYNK_WRITE(V5)
{
  int x = param[0].asInt();
  int y = param[1].asInt();
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
  }
  // Move forward left
  else if(x >= maxRange && y >= maxRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, minSpeed);
    ledcWrite(motorR_EN, maxSpeed);
  }
  // Move forward right
  else if(x >= maxRange && y <= minRange)
  {
    digitalWrite(motorL_Positive,HIGH);
    digitalWrite(motorL_Negative,LOW); 
    digitalWrite(motorR_Positive,LOW);
    digitalWrite(motorR_Negative,HIGH);
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, minSpeed);
  }  
  // Neutral zone
  else if(y < maxRange && y > minRange && x < maxRange && x > minRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,LOW);
    digitalWrite(motorR_Positive,LOW);  
    digitalWrite(motorR_Negative,LOW); 
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
  }
  // Move back and right
 else if(y <= minRange && x <= minRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, maxSpeed);
    ledcWrite(motorR_EN, minSpeed);
  }
 
  // Move back and left
  else if(x <= minRange && y >= maxRange)
  {
    digitalWrite(motorL_Positive,LOW); 
    digitalWrite(motorL_Negative,HIGH);
    digitalWrite(motorR_Positive,HIGH); 
    digitalWrite(motorR_Negative,LOW); 
    ledcWrite(motorL_EN, minSpeed);
    ledcWrite(motorR_EN, maxSpeed);
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

  if (pinData < minLimiter){ // Min val limiter
    maxSpeed == minLimiter;
    Blynk.virtualWrite(V6, minLimiter);
  }
  else if (pinData > maxLimiter){ // Max val limiter
    maxSpeed == maxLimiter;
    Blynk.virtualWrite(V6, maxLimiter);
  }
  else{
    maxSpeed = pinData;
  }
  
  // minSpeed = maxSpeed-60; //for res 8 (0-255) is 23.53%
  minSpeed = (25/100)*maxSpeed; /*Converted to percentage so it 
   should work with every resolution 8,10,12,16 etc. ( value - 255,1023,2047,4095) where value is 2^resolution*/

  // Debug
  /*Serial.print("maxSpeed = ");
  Serial.println(maxSpeed);
  Serial.println();*/
}

// Ultrasonic Sensor button.
BLYNK_WRITE(V3) 
{
  uSonicState = param.asInt(); 
}

void uSonicButtonCheck() // Function to check ultrasonic button state. On/Off.
{
  if (uSonicState == HIGH) // If button is on.
  {
  
    currentToUSonic == HIGH; // Turns on NPN Transistor to supply power to the sensor.
   
    distance = sr04.Distance();

    // Debug
    /*Serial.print("Distance = ");
    Serial.print(distance);
    Serial.println("cm");*/
  
    //WebSerial.print("Distance = ");
    //WebSerial.print(distance);
    //WebSerial.println("cm");
    
    if (distance <= 40) // Prevent tank from crashing into objects.
    {
      do {
         digitalWrite(motorL_Positive,LOW); 
         digitalWrite(motorL_Negative,LOW);
         digitalWrite(motorR_Positive,LOW);  
         digitalWrite(motorR_Negative,LOW); 
      } while (currentToUSonic == HIGH); // This keeps the tank/motors at halt until the timer runs the function again and ultrasonic button is off.
    }   
  }
  else {currentToUSonic == LOW;} // Cut power to NPPN Transistor.
}

// Led lights slider.
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
    ledIndicator.setValue((((pinData/maxLimiter)*100)/100)*255); // Convert x bit io 8 bit for WidgetLED.
  }
 
  // Debug
  /*int ledValue = ledcRead(lights);
  Serial.print("ledValue = ");
  Serial.println(ledValue);*/
  
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
  // ArduinoOTA.setHostname("myesp32");

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

void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
  if (d == "ON"){
    digitalWrite(LED, HIGH);
  }
  if (d=="OFF"){
    digitalWrite(LED, LOW);
  }
}
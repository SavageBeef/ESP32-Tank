# ESP32-Tank

## Overview
This project utilizes the ESP32 as a Microcontroller that is programmed to enable a DIY Tank to be controlled over a wireless network using Arduino IDE and Blynk Library.

#### Block Diagram

![Block Diagram](Block%20Diagram.png)

#### Schematic Diagram

![Schematic Diagram](Tank%20Schematic_schem.png)


## IDE
Arduino IDE - 2.3.6

### Boards Manager
esp32 by Espressif Systems - 3.2.0

### Libraries
Blynk by Volodymyr Shymanskyy - 1.1.0

ESP Async WebServer by ESP32Async - 3.7.6

Async TCP by ESP32Async - 3.3.8 

### Fix for Arduino OTA update with Arduino IDE 2
https://forum.arduino.cc/t/network-ports-missing/1355667/4

## Blynk Local Server
https://github.com/Peterkn2001/blynk-server

Blynk mobile app - 2.27.34

## Components List
Chassis - DIY T300 NodeMCU Aluminum Alloy Metal Wall-E Tank Track Caterpillar Chassis Smart Robot Kit 

IC2 - KeeYees ESP-WROOM-32 (NodeMCU-32S) (38 PIN Narrow version)

IC1 - L293D Motor Driver IC

M1 & M2 - ComXim 25GA370 High Torque DC Brush Motor 25mm All Metal Gear (12V, 100R)

S1 & S2 - Gikfun MTS102 2 Position 3 Pins Mini Toggle Switch for Arduino

U1 & U2 - STMicroelectronics L7805CV TO-220 Voltage Regulator

C1 & C2 - 10 uf Electrolytic Capacitor

R1 & R2 - 220 Ohms Resistor

Q1 - PN 2222A NPN Transistor

Q2 - S8050 NPN Transistor

hc-sr1 - HC-SR04 Ultrasonic Sensor

4 10k Ohms Resistors - For voltage divider (Not in the Schematic Diagram)


## Blynk App
#### App control and Virtual Pins
![Virtual Pins](Blynk%20App%20Virtual%20Pins.jpg)

#### Control running
![Control running](Blynk%20App%20Running.jpg)

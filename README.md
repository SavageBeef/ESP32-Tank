# ESP32-Tank: Wireless Network Controlled DIY Tank

## 🚀 Overview

This project details a **DIY Tank** controlled over a wireless network using an **ESP32** microcontroller. The system is programmed using **Arduino IDE** and utilizes the **Blynk Library** for mobile control.

---

## 🏗️ System Diagrams

### Block Diagram

![Block Diagram](Block%20Diagram.png)

### Schematic Diagram

![Schematic Diagram](Tank%20Schematic_schem.png)

---

## 💻 Development Environment

### Arduino IDE

* **IDE Version:** Arduino IDE - 2.3.6

### Boards Manager

* `esp32` by Espressif Systems - 3.2.0

### Libraries

| Library Name | Author | Version |
| :--- | :--- | :--- |
| **Blynk** | Volodymyr Shymanskyy | 1.1.0 |
| **ArduinoOTA** | Arduino, Juraj Andrassy | 1.0.8 |
| **WebSerial** | Ayush Sharma | 1.3.0 |
| **ESP Async WebServer** | ESP32Async | 3.7.6 |
| **Async TCP** | ESP32Async | 3.3.8 |
| **SR04** | -   | -   |

### 🛠️ OTA Update Fix

If you encounter issues with Arduino OTA updates in Arduino IDE 2, refer to this fix:
[https://forum.arduino.cc/t/network-ports-missing/1355667/4](https://forum.arduino.cc/t/network-ports-missing/1355667/4)

---

## 📱 Blynk Setup

### Local Server

The project utilizes a local Blynk server:
[https://github.com/Peterkn2001/blynk-server](https://github.com/Peterkn2001/blynk-server)

### Mobile App

* **Blynk mobile app version:** 2.27.34

### App Interface

#### Virtual Pins & Controls

The Blynk app interface shows the configuration of virtual pins used for control.

![Virtual Pins](Blynk%20App%20Virtual%20Pins.jpg)

#### Control in Action

This image displays the app while the tank is being controlled.

![Control running](Blynk%20App%20Running.jpg)

---

## 🔋 Power Source Configuration

The tank is powered by **eight 18650 Li-ion batteries** configured as **2-Parallel, 4-Series (2P4S)**. This arrangement provides the optimal voltage and extended runtime for the motors and electronics.

| Battery Type | Nominal Voltage | Quantity | Configuration |
| :--- | :--- | :--- | :--- |
| **18650 Li-ion** | 3.7 V | 8 | 2P4S |

### Output Voltage and Current

The **4-Series (4S)** component determines the voltage, while the **2-Parallel (2P)** component increases the current capacity (Ah) for longer runtime.

* **Nominal Output Voltage (Series):** 4 cells × 3.7 V = **14.8 V**
* **Maximum Output Voltage (Fully Charged):** 4 cells × 4.2 V ≈ **16.8 V**

This configuration provides:
* **Optimal Voltage** for the **ComXim 25GA370 DC Brush Motors** (12V rated), ensuring strong performance without significant voltage drop under load.
* **Increased Current Capacity** (due to the parallel components) for a significantly **longer runtime**.

---

## 📋 Components List

| Component Reference | Description |
| :--- | :--- |
| **Chassis** | DIY T300 NodeMCU Aluminum Alloy Metal Wall-E Tank Track Caterpillar Chassis Smart Robot Kit |
| **IC2** (Microcontroller) | KeeYees ESP-WROOM-32 (NodeMCU-32S) (38 PIN Narrow version) |
| **IC1** (Motor Driver) | L293D Motor Driver IC |
| **M1 & M2** (Motors) | ComXim 25GA370 High Torque DC Brush Motor 25mm All Metal Gear (12V, 100R) |
| **S1 & S2** (Switches) | Gikfun MTS102 2 Position 3 Pins Mini Toggle Switch for Arduino |
| **U1 & U2** (Regulators) | STMicroelectronics L7805CV TO-220 Voltage Regulator |
| **C1 & C2** (Capacitors) | 10 µF Electrolytic Capacitor |
| **R1 & R2** (Resistors) | 220 Ohms Resistor |
| **Q1** (Transistor) | PN 2222A NPN Transistor |
| **Q2** (Transistor) | S8050 NPN Transistor |
| **hc-sr1** (Sensor) | HC-SR04 Ultrasonic Sensor |
| **R3 - R8** (Resistors) | For a voltage divider where R2 = **R3** (10k Ohms) and R1 = sum of **R4 - R8** (42k Ohms) |

---

## 🚧 Future Development / To-Do List

This project is a work in progress. The following features and optimizations are ideas for future updates:

### ⚙️ Tank Control Logic

* [x] **Ultrasonic Sensor Optimization:** Improve the HC-SR04 sensor logic to ensure the tank remains stopped after an obstacle is detected, even if the joystick remains in the forward position.
    * The tank should only resume movement when the sensor is turned off, acting as a manual reset after an obstacle stop.
    * [`0412ced`](https://github.com/SavageBeef/ESP32-Tank/commit/0412ced1019b2e85aa10e8bf287e56d3dd1e1c23)
* [x] **Optimize Speed Slider Sensitivity / Eliminate Dead Zone:** The previous 10-bit resolution caused a large motor dead zone (0-750). This was fixed by using the `map()` function to scale the input slider range to the motor's active PWM range.
    * [`4c2c6b1`](https://github.com/SavageBeef/ESP32-Tank/commit/4c2c6b1081278c5ccd0cd4174da5427822565486)
* [x] **Implement Dual-Mode Turning (Soft & Hard):** Refine the differential drive logic to support two distinct turning behaviors based on control input:
    * **Soft Turns:** Gentle, wide-radius turns when the tank is moving forward at speed.
    * **Hard Turns (Pivot):** Sharp, in-place (pivot) turns when the tank's forward speed is near zero.
    * [`a0f3a01`](https://github.com/SavageBeef/ESP32-Tank/commit/a0f3a01f58bbff9ed59f0d5cbbd16faba97a114c)
    * [`9b06988`](https://github.com/SavageBeef/ESP32-Tank/commit/9b069881c559e9dc7d497c702f9e7d618b88abb4) - Directional Transition Guard Feature.

### 💻 Development Environment / Utility

* [ ] **Custom Serial Wrapper:** Create a custom function (e.g., `dualPrint()`, `dualPrintln()`) that simultaneously outputs messages to both the standard **Serial Monitor** and the **WebSerial** interface.

### 🌐 Network Management (Captive Portal)

* [ ] **Wi-Fi Provisioning Webpage:** Implement a captive portal or a simple webpage that launches when the tank fails to connect to the configured network credentials.
    * This page should allow the user to enter new Wi-Fi credentials (SSID, Password), Blynk Port, and the Blynk Auth Token.
    * The new credentials must be saved to the ESP32's **EEPROM/Flash (Preferences)** to persist across reboots.

### 📺 Multimedia & Telepresence 🆕

* [ ] **Hardware Upgrade:** Replace the current **ESP32-WROOM-32** with an **ESP32-CAM** or a more powerful **ESP32 variant with PSRAM** (e.g., ESP32-WROVER-E) to handle high-speed image acquisition and streaming.
* [ ] **Camera Module Integration (FPV):**
    * Add a **Camera Module** (like the OV2640 or OV7725) for **First-Person View (FPV)**.
    * Implement a lightweight video stream (e.g., MJPEG) accessible via a web interface or integrated into the Blynk/custom app.
* [ ] **Audio Integration (Two-Way Communication):**
    * Add a **Microphone** and **Speaker/Amplifier** for two-way audio communication (telepresence).
    * Implement audio capture and streaming, potentially using the **I2S protocol** on the ESP32.

### 📱 Blynk App Integration

* [x] **Battery Life:** Integrate the remaining battery life/voltage reading into the Blynk mobile application. 
    * This allows the user to monitor the power source level in real-time.
    * [`4fa2a0d`](https://github.com/SavageBeef/ESP32-Tank/commit/4fa2a0db8150598015da4d4c1dc8b0fdf28ded42)
* [x] **Distance Feedback:** Integrate the distance measurement from the HC-SR04 ultrasonic sensor into the Blynk mobile application. 
    * This will allow users to monitor the current distance to obstacles in real-time.
    * [`1a86019`](https://github.com/SavageBeef/ESP32-Tank/commit/1a86019a34868218a9e55ddc37525fbe77eed8e1)
* [x] **Custom Sensor Limit:** Add a **Numeric Input widget** in the Blynk app so the user can set a **custom distance limit** (threshold) for the ultrasonic sensor.
    * This will allow the user to dynamically adjust how close the tank gets to an obstacle before stopping.
    * [`fa8743b`](https://github.com/SavageBeef/ESP32-Tank/commit/fa8743b2416cef666430fe2d0306312941814d07)

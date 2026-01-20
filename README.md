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
| **ArduinoOTA** | Arduino, Juraj Andrassy | 1.1.0 |
| **WebSerial** | Ayush Sharma | 2.1.1 |
| **ESP Async WebServer** | ESP32Async | 3.9.0 |
| **Async TCP** | ESP32Async | 3.4.9 |
| **SR04** | Elegoo   | -   |
| **WiFiManager** | tzapu | 2.0.0 |

### 🛠️ OTA Update Fix

If you encounter issues with Arduino OTA updates in Arduino IDE 2, refer to this [fix](https://forum.arduino.cc/t/network-ports-missing/1355667/4).

---

## 📱 Blynk Setup

### Local Server
This project utilizes a local Blynk server. You can find the source code and details here:
[blynk-server](https://github.com/Peterkn2001/blynk-server).

You can set up the server using one of the two methods below:

#### Option 1: Script
You can use the helper scripts to start the server.

1.  Download the configuration scripts here: **[blynk-configs](https://github.com/SavageBeef/blynk-configs)**.
2.  Ensure the script is placed in the **same directory** as the `server-0.41.17.jar` file.
3.  Run the script to start the server.

#### Option 2: Docker (Containerized)
This method uses the [`hokori/blynk-server:0.41.17`](https://hub.docker.com/r/hokori/blynk-server) image, which supports both **x64** and **ARM** architectures.

**1. Prerequisites**:
Ensure you have **Docker** and **Docker Compose** installed on your machine.

**2. Configuration**
* Download/Create your `docker-compose.yml` file: **[blynk-configs](https://github.com/SavageBeef/blynk-configs)**.
* Edit the file as needed to match your environment variables.

**3. Management Commands**:
Open your terminal in the directory containing your `docker-compose.yml` file and use the following commands:

* **Start the container:**
    ```bash
    docker-compose up -d
    ```
* **Stop and remove the container:**
    ```bash
    docker-compose down
    ```

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

* [x] **Custom Serial Wrapper:** Create a custom function (e.g., `dualPrint()`, `dualPrintln()`) that simultaneously outputs messages to both the standard **Serial Monitor** and the **WebSerial** interface.
    * [`7b2f663`](https://github.com/SavageBeef/ESP32-Tank/commit/7b2f663f4e958d80c948881af8dee6b58300c08f) - Custom Serial Wrapper created for Print and Println method.
    * [`731f6c9`](https://github.com/SavageBeef/ESP32-Tank/commit/731f6c9ff5fa201b73b6f177cf589acf51a3baca) - Implement printf feature for WebSerial.
    * [`1e4c11e`](https://github.com/SavageBeef/ESP32-Tank/commit/1e4c11e764a6946d85a6ee605c938358414fb7d8) - dualPrintf() function.

### 🌐 Network Management (Captive Portal)

* [x] **Wi-Fi Provisioning Webpage:** Implement a captive portal or a simple webpage that launches when the tank fails to connect to the configured network credentials.
    * This page should allow the user to enter new Wi-Fi credentials (SSID, Password), Blynk Port, and the Blynk Auth Token.
    * The new credentials must be saved to the ESP32's **EEPROM/Flash (Preferences)** to persist across reboots.
    * [`141e742`](https://github.com/SavageBeef/ESP32-Tank/commit/141e7425c4e670193565e8a104879f825c3bfaf1) - SSID:`Tank-Setup` PASS:`password123` IP:`192.168.4.1`

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

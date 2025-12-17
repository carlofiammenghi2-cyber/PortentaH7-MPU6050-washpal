WashPal: IoT Laundry Monitoring System
WashPal is an intelligent, non-intrusive IoT solution designed to monitor the status of shared laundry machines. By correlating vibration data from an accelerometer with human presence detection via PIR, the system provides real-time availability updates via a web interface, eliminating the guesswork of shared laundry room use.

🚀 Project Overview
In shared living environments, checking laundry availability is often a manual and frustrating task. WashPal solves this by using a Central Unit and a Peripheral Unit that communicate via Bluetooth Low Energy (BLE). The Central Unit processes sensor data and hosts a Wi-Fi Web Server to display the machine's status to users.

Key Features

Dual-Trigger Detection: Prevents false positives by requiring both human presence and machine vibration to initiate a cycle.

Three-State Logic: Automatically tracks machine status: IDLE, WASHING, and READY (Cycle Finished).

Eco-Mode: A power-saving state that allows the system to enter deep sleep during off-peak hours to conserve energy.

Web Dashboard: A real-time UI accessible from any browser connected to the local network.

🛠 Hardware Architecture
The prototype was developed using the Arduino Portenta H7, chosen for its dual-core processing power during the testing phase.

Current Prototype Components

Central Unit: Arduino Portenta H7 equipped with a GY521 (MPU6050) Gyroscope/Accelerometer.

Peripheral Unit: Arduino Portenta H7 equipped with an AM312 PIR Motion Sensor.

Optimized Production Path

While the prototype uses the Portenta H7, the project identifies a more cost-effective hardware transition for scalability:

Central Unit Alternative: Arduino Nano ESP32. This maintains Wi-Fi and high processing speeds while reducing the unit cost by approximately 90%.

Peripheral Unit Alternative: Seeed Studio XIAO nRF52840. This offers a tiny footprint and ultra-low deep sleep current (5μA), which is ideal for battery-powered sensor nodes.

💻 Software & Logic
The system utilizes a Finite State Machine (FSM) to manage state transitions:

IDLE: The Central unit polls the MPU6050. If vibration magnitude exceeds the threshold (>50 counts) and the PIR sensor (via BLE) detects a person, the state transitions to WASHING.

WASHING: The system disconnects the BLE link to save power during the long washing cycle. It monitors for the cessation of vibration.

READY: Triggered after 5 minutes of no vibration. The BLE link reconnects to detect when a user arrives to collect the laundry, eventually resetting the system to IDLE.

File Structure

Gyroscope_Sensor.ino: Main logic for the Central Unit (Web Server, BLE Client, and MPU6050 processing).

AM312_Pir_Sensor.ino: Logic for the Peripheral Unit (BLE Service provider for motion detection).

🔧 Setup & Installation
Hardware Wiring:

Connect the MPU6050 to the Central Unit via I2C (SDA/SCL pins).

Connect the AM312 PIR to a digital input pin on the Peripheral Unit.

Library Dependencies: Install ArduinoBLE, WiFi, Adafruit_MPU6050, and Adafruit_Sensor via the Arduino Library Manager.

Deployment: * Flash the Peripheral code first.

Flash the Central code and open the Serial Monitor to identify the IP address.

Open your browser and enter the IP address to view the WashPal Dashboard.

👥 Contributors
Sara Mashhadi Alizadeh

Manuel Bosisio

Carlo Achille Fiammenghi

Alessandro Guerrisi

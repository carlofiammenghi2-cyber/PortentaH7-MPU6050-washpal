WashPal: An IoT Solution for Laundry Room Monitoring
WashPal is an IoT-based system designed to monitor shared laundry rooms in real-time. By combining vibration analysis with human presence detection, the system provides remote insights into machine availability and room occupancy via a web application.

-----------

Overview

Shared laundry rooms often lead to frustration when residents find machines occupied or finished laundry left uncollected. WashPal solves this with a low-cost, non-intrusive prototype that tracks:

Machine Status: Idle, Running, or Ready for pickup.

Room Occupancy: Detects if someone is currently in the laundry room.

------------

System Architecture

The system utilizes a distributed sensing model with two primary nodes:

Central Unit: Mounted on the washing machine to monitor vibrations and host the Wi-Fi web server.

Peripheral Unit: Placed in the room corner to monitor human presence via a PIR sensor.

Communication: The units interconnect via Bluetooth Low Energy (BLE), while the Central Unit relays data to users over Wi-Fi.

------------

Hardware Implementation

Current Prototype

Central Unit: Arduino Portenta H7 + GY521 (MPU6050) accelerometer.

Peripheral Unit: Arduino Portenta H7 + AM312 PIR motion sensor.

Production Path (Recommended Alternatives)

To reduce the bill of materials (BOM) by approximately 90%, the following hardware is recommended for final deployment:

Central Unit: Arduino Nano ESP32 (~$20 USD).

Peripheral Unit: Seeed Studio XIAO nRF52840 (~$10 USD).

------------

Software Logic

The system operates as a Finite State Machine (FSM) with three primary states:

IDLE: Monitors for a sustained vibration threshold (>50 counts) to confirm a cycle start.

WASHING: Disconnects BLE to the peripheral unit to save power while the machine runs. It assumes completion after 5 minutes of no vibration.

READY: Re-establishes BLE connection to detect when a user arrives to collect the laundry. The system resets to IDLE only after motion is detected and then stops for 1 minute.

------------

User Interface

Users can access a local web dashboard that displays:

Current room and machine status (e.g., "Room OCCUPIED, Machine IDLE").

Eco-Mode: A toggle to disable non-essential sensors and wireless scanning during off-peak hours to save energy.

------------

Repository Structure

Gyroscope_Sensor.ino: Code for the Central Unit (Vibration processing, Wi-Fi Server, BLE Client).

AM312_Pir_Sensor.ino: Code for the Peripheral Unit (Motion detection, BLE Service).

-----------

Contributors

Sara Mashhadi Alizadeh 

Manuel Bosisio 

Carlo Achille Fiammenghi 

Alessandro Guerrisi

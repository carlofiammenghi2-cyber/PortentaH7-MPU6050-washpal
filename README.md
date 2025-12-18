# 🧺 WashPal: An IoT Solution for Laundry Room Monitoring

**WashPal** is an IoT-based system designed to monitor **shared laundry rooms** in real time.  
By combining **vibration analysis** with **human presence detection**, it provides remote insight into machine availability and room occupancy via a sleek web application.

---

## 📘 Overview

Shared laundry rooms often lead to frustration when residents find machines occupied or laundry left uncollected.  
**WashPal** solves this by offering a low-cost, non-intrusive prototype that tracks:

- 🌀 **Machine Status:** *Idle*, *Running*, or *Ready for Pickup*  
- 🚶 **Room Occupancy:** Detects if someone is currently inside  

---

## 🧩 System Architecture

The system utilizes a **distributed sensing model** with two primary nodes:

### 🖥️ Central Unit
- Mounted on the washing machine to monitor vibrations  
- Hosts the local Wi-Fi web server  

### 📡 Peripheral Unit
- Positioned in the room corner to track motion via a PIR sensor  

### 🔗 Communication
- Nodes interconnect using **Bluetooth Low Energy (BLE)**  
- The **Central Unit** relays data to users via **Wi-Fi**

---

## ⚙️ Hardware Implementation

### Current Prototype
| Component | Role | Model |
|------------|------|-------|
| Central Unit | Vibration sensing & Wi-Fi server | Arduino **Portenta H7** + **GY521 (MPU6050)** |
| Peripheral Unit | Motion detection | Arduino **Portenta H7** + **AM312 PIR Sensor** |

---

## 🧠 Software Logic

WashPal operates as a **Finite State Machine (FSM)** with three core states:

1. **IDLE**  
   - Monitors for sustained vibration above threshold (>50 counts) to confirm cycle start.  
2. **WASHING**  
   - Disconnects BLE to save power during washing.  
   - Assumes completion after 5 minutes of inactivity.  
3. **READY**  
   - Reconnects BLE to detect user presence.  
   - Resets to *IDLE* after motion is detected and stops for 1 minute.  

---

## 🖥️ User Interface

Users can access a **local web dashboard** to view:

- 👕 **Machine & Room Status**  
  Example: `"Room OCCUPIED, Machine IDLE"`  
- 🌿 **Eco-Mode**  
  Toggle to disable non-essential sensors and wireless scanning during off-peak hours to conserve energy.

---

## 📂 Repository Structure

/WashPal

├── Gyroscope_Sensor/  
│ ├── Gyroscope_Sensor.ino # Central Unit: Vibration processing, Wi-Fi Server, BLE Client  
│ └── arduino_secrets.h # Wi-Fi credentials (to be added manually)  
├── AM312_Pir_Sensor/   
│ └── AM312_Pir_Sensor.ino # Peripheral Unit: Motion detection, BLE Service  
└── README.md


> 📝 **Note:**  
> You must create an `arduino_secrets.h` file within the `Gyroscope_Sensor` folder containing:
> ```
> #define SECRET_SSID "your_SSID"
> #define SECRET_PASS "your_pwd"
> ```

---

## 👩‍💻 Contributors

- **Sara Mashhadi Alizadeh**  
- **Manuel Bosisio**  
- **Carlo Achille Fiammenghi**  
- **Alessandro Guerrisi**  

---

## 💡 Keywords
`IoT` · `Arduino` · `BLE` · `Laundry Monitoring` · `Vibration Detection` · `PIR Sensing` · `FSM`

---

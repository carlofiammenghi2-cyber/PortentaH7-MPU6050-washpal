/*
  PERIPHERAL BOARD: IR Sensor - High Speed Continuous Mode
  - Sends '1' every 100ms while motion is detected.
  - Optimized for the new "50 Count" threshold (fills in ~5 seconds).
*/
#include <ArduinoBLE.h>

// PIN CONFIGURATION
const int IR_PIN = D2; 

// BLE SETTINGS
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* charUUID    = "19B10001-E8F2-537E-4F6C-D104768A1214";

BLEService irService(serviceUUID);
BLEByteCharacteristic irCharacteristic(charUUID, BLERead | BLENotify);

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  
  // LED Setup
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  
  // Turn all OFF (High is OFF on Portenta)
  digitalWrite(LEDR, HIGH); 
  digitalWrite(LEDG, HIGH); 
  digitalWrite(LEDB, HIGH);

  // Start BLE
  if (!BLE.begin()) {
    // If it's not connected, turn on the red light 
    while (1) { digitalWrite(LEDR, LOW); } // Led Red = Error
  }

  BLE.setLocalName("PortentaSensor"); 
  BLE.setAdvertisedService(irService);
  irService.addCharacteristic(irCharacteristic);
  BLE.addService(irService);

  irCharacteristic.writeValue(0);
  BLE.advertise();

  // Solid Green = Ready and Waiting for Connection
  digitalWrite(LEDG, LOW); 
}

void loop() {
  BLEDevice central = BLE.central();
  static int lastWasZero = -1; // Being static it is not initialized again in the next loop


  if (central) {
    // When connected, turn off the "Waiting" Green light
    digitalWrite(LEDG, HIGH); 

    while (central.connected()) {
      int currentState = digitalRead(IR_PIN);

      // --- CONTINUOUS HIGH-SPEED SENDING ---
      
      // If motion is detected (1), send it REPEATEDLY
      if (currentState == 1) {
        irCharacteristic.writeValue((byte)1);
        Serial.println("Motion! Sending 1...");
        lastWasZero = -1;
        
        // Blink Blue LED to show data transmission
        digitalWrite(LEDB, LOW); delay(10); digitalWrite(LEDB, HIGH);

        // Updated Delay: 100ms = delay + led triggering
        // Target: 50 counts. 
        // 50 * 100ms = 5 Seconds of motion to trigger "Washing"
        delay(90); 
      } 
      // If no motion (0), only send if it changed (Save bandwidth)
      else {
         if (lastWasZero != 0) {
            irCharacteristic.writeValue((byte)0);
            Serial.println("Empty. Sending 0...");
            lastWasZero = 0;
         }
         // Check again in 100ms
         delay(100); 
      }
    }
    
    // If disconnected, turn Green "Waiting" light back on
    digitalWrite(LEDG, LOW); 
  }
}
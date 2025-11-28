/*
  PERIPHERAL BOARD + IR SENSOR
*/
#include <ArduinoBLE.h>

// --- BLE CONFIGURATION ---
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* charUUID    = "19B10001-E8F2-537E-4F6C-D104768A1214";

BLEService sensorService(serviceUUID); 
// We add "BLENotify" so the Central gets data automatically without asking
BLEByteCharacteristic sensorChar(charUUID, BLERead | BLENotify);

// --- SENSOR CONFIGURATION ---
int pirPin = 2; // Ensure your sensor is on Pin D2
int pirState = LOW;
int val = 0;

unsigned long motionDetectedTime = 0;
unsigned long noMotionStartTime = 0; 
unsigned long emptyDelay = 10000;    
unsigned long presenceDelay = 5000;   

bool presenceReported = false;
bool motionActive = false;
bool roomEmptyReported = false; // Added to prevent spamming "Empty" BLE packets

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pirPin, INPUT);
  
  // --- BLE SETUP ---
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("PortentaSensor");
  BLE.setAdvertisedService(sensorService);
  sensorService.addCharacteristic(sensorChar);
  BLE.addService(sensorService);
  
  // Initial Value
  sensorChar.writeValue(0); 
  
  BLE.advertise();
  Serial.println("BLE Active. Waiting for Central...");
}

void loop() {
  // Listen for connections
  BLEDevice central = BLE.central();

  // We only run the logic if a Central is connected, 
  // otherwise we just wait.
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      readSensorAndSend(); // Run your sensor logic here
    }

    Serial.println("Disconnected");
    digitalWrite(LED_BUILTIN, HIGH); // Turn off LED
  }
}

void readSensorAndSend() {
  val = digitalRead(pirPin);
  unsigned long currentTime = millis();

  if (val == HIGH) {
    // 1. MOTION DETECTED
    if (!motionActive) {
      motionDetectedTime = currentTime; 
      presenceReported = false; 
      motionActive = true;
      roomEmptyReported = false; // Reset empty flag
      
      Serial.println("Motion detected!");
      sensorChar.writeValue(1); // SEND CODE 1 via BLE
      digitalWrite(LED_BUILTIN, LOW); // LED ON
    } else {
      // 2. PRESENCE CONFIRMED
      if (!presenceReported && (currentTime - motionDetectedTime >= presenceDelay)) {
        Serial.println("Someone is in the room");
        sensorChar.writeValue(2); // SEND CODE 2 via BLE
        presenceReported = true;
      }
    }
  } else {
    // Motion stopped logic
    if (motionActive) {
      Serial.println("Motion stopped!");
      digitalWrite(LED_BUILTIN, HIGH); // LED OFF
      motionActive = false;
      noMotionStartTime = currentTime;
    }

    // 3. ROOM EMPTY
    if (!roomEmptyReported && (currentTime - noMotionStartTime) > emptyDelay) {
      Serial.println("The washing machine room is empty");
      sensorChar.writeValue(0); // SEND CODE 0 via BLE
      
      noMotionStartTime = currentTime; 
      roomEmptyReported = true; // Flag to ensure we only send this once
    }
  }
}
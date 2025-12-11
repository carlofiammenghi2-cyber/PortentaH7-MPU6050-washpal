#include <ArduinoBLE.h>
#include "GY521.h"
#include <WiFi.h>
#include "arduino_secrets.h"
#include <Wire.h>
#include <rtos.h> 

// --- CONFIGURATION ---
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* charUUID    = "19B10001-E8F2-537E-4F6C-D104768A1214";

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WiFiServer server(80);

// --- HARDWARE ---
GY521 sensor(0x68);

// --- GLOBAL VARIABLES ---
BLEDevice centralPeripheral; 
BLECharacteristic irCharacteristic;
bool isBleConnected = false;
int lastIRStatus = -1; 

// --- WASHING STATE MACHINE ---
enum WashState {
  STATE_IDLE,
  STATE_WASHING,
  STATE_READY
};
WashState machineState = STATE_IDLE;

// Timestamps & Counters
unsigned long firstVibrationTime = 0; 
unsigned long lastVibrationTime = 0;  
uint32_t vibration_counter = 0;       

// NEW: STARTUP FILTER (Threshold increased to 150)
uint32_t ir_startup_counter = 0;      
unsigned long startupWindowStart = 0; 
const int MIN_VIB_TO_START = 50;      // Vibration stays at 50
const int MIN_IR_TO_START = 150;      // UPDATED: IR needs 150 counts (approx 15 sec)
const unsigned long STARTUP_WINDOW = 300000; // 5 Minutes

// Logic Constants
const unsigned long TIMEOUT_DONE = 300000; // 5 Mins silence = Done
const unsigned long TIMEOUT_RESET = 300000; // 5 Mins empty = Reset

// Pickup Logic
bool userVisitedForPickup = false;
unsigned long roomEmptyStartTime = 0;

// IR Tracker
unsigned long lastMotionTime = 0; 
bool hasDetectedMotionOnce = false; 

// Gyro Config
unsigned long lastGyroReadTime = 0;
const long gyroInterval = 100; 
const float VIBRATION_THRESHOLD = 0.25; 

// --- ECO MODE ---
bool isEcoMode = false; 

// --- PROTOTYPES ---
void setupWebServer();
void handleWebClient();
void sendHTML(WiFiClient &client);
void checkVibration();
void manageBLE();
void manageLaundryLogic();
void resetSystem();
void resetIdleCounters();
void enterEcoMode();
void exitEcoMode();

void setup() {
  Serial.begin(115200);
  
  // Power Saving
  pinMode(LEDR, OUTPUT); pinMode(LEDG, OUTPUT); pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR, HIGH); digitalWrite(LEDG, HIGH); digitalWrite(LEDB, HIGH);

  // WiFi
  Serial.print("Creating AP: ");
  Serial.println(ssid);
  if (WiFi.beginAP(ssid, pass) != WL_AP_LISTENING) while (true);
  delay(2000);
  server.begin();
  
  // Gyro
  Wire.begin();
  delay(100);
  if (sensor.wakeup()) {
    sensor.setAccelSensitivity(0); 
    sensor.setGyroSensitivity(0);
  }

  // BLE
  if (!BLE.begin()) Serial.println("BLE Failed!");
  else BLE.scanForUuid(serviceUUID);
}

void loop() {
  handleWebClient();

  if (!isEcoMode) {
    if (millis() - lastGyroReadTime >= gyroInterval) {
      lastGyroReadTime = millis();
      checkVibration();
    }
    
    // ONLY Manage BLE if we are NOT washing
    // This saves energy by disconnecting the sensor while the machine runs
    if (machineState != STATE_WASHING) {
       manageBLE();
    } else {
       // If we are Washing, ensure we are DISCONNECTED
       if (isBleConnected) {
         Serial.println("Washing started. Disconnecting IR Sensor to save power...");
         centralPeripheral.disconnect();
         isBleConnected = false;
         lastIRStatus = -1; // Unknown while washing
       }
    }

    manageLaundryLogic();
  } 
  else {
    rtos::ThisThread::sleep_for(100); 
  }
}

// --- CORE LOGIC ---

void manageLaundryLogic() {
  unsigned long now = millis();

  // STATE 1: IDLE -> WASHING
  if (machineState == STATE_IDLE) {
    
    // Check timeout
    if (startupWindowStart > 0 && (now - startupWindowStart > STARTUP_WINDOW)) {
      resetIdleCounters(); 
    }

    // UPDATED THRESHOLD CHECK (150 IR)
    if (vibration_counter > MIN_VIB_TO_START && ir_startup_counter > MIN_IR_TO_START) {
      machineState = STATE_WASHING;
      Serial.println("Cycle STARTED (Confirmed by Vibration + Presence)");
    }
  }

  // STATE 2: WASHING -> READY
  else if (machineState == STATE_WASHING) {
    if (now - lastVibrationTime > TIMEOUT_DONE) {
      machineState = STATE_READY;
      Serial.println("Cycle FINISHED. Re-enabling IR Sensor...");
      // BLE will automatically reconnect in the next loop() because state is no longer WASHING
      BLE.scanForUuid(serviceUUID); // Restart scanning immediately
    }
  }

  // STATE 3: READY -> RESET (Pickup Logic)
  else if (machineState == STATE_READY) {
    // Only proceed if BLE has reconnected
    if (isBleConnected) {
        if (lastIRStatus == 1 || lastIRStatus == 2) {
          userVisitedForPickup = true;
          roomEmptyStartTime = 0; 
        }
        if (lastIRStatus == 0) {
          if (roomEmptyStartTime == 0) roomEmptyStartTime = now;
          
          if (userVisitedForPickup && (now - roomEmptyStartTime > TIMEOUT_RESET)) {
            resetSystem();
          }
        }
    }
  }
}

void resetSystem() {
  Serial.println("System RESET for next user");
  machineState = STATE_IDLE;
  resetIdleCounters();
  userVisitedForPickup = false;
  roomEmptyStartTime = 0;
}

void resetIdleCounters() {
  vibration_counter = 0;
  ir_startup_counter = 0;
  startupWindowStart = 0;
  firstVibrationTime = 0;
  lastVibrationTime = 0;
}

void checkVibration() {
  sensor.read();
  float ax = sensor.getAccelX();
  float ay = sensor.getAccelY();
  float az = sensor.getAccelZ(); 

  static float prevAx = 0;
  static float prevAy = 0;
  static float prevAz = 0; 

  if (prevAx == 0 && prevAy == 0 && prevAz == 0) { 
    prevAx = ax; prevAy = ay; prevAz = az; 
    return; 
  }

  float deltaX = abs(ax - prevAx);
  float deltaY = abs(ay - prevAy);
  float deltaZ = abs(az - prevAz); 

  if (deltaX > VIBRATION_THRESHOLD || deltaY > VIBRATION_THRESHOLD || deltaZ > VIBRATION_THRESHOLD) {
    vibration_counter++;
    lastVibrationTime = millis();
    
    if (machineState == STATE_IDLE) {
      if (startupWindowStart == 0) startupWindowStart = millis();
      if (firstVibrationTime == 0) firstVibrationTime = millis();
    }
  }
  
  prevAx = ax; prevAy = ay; prevAz = az;
}

void manageBLE() {
  if (isBleConnected) {
    if (!centralPeripheral.connected()) {
      isBleConnected = false;
      lastIRStatus = -1;
      BLE.scanForUuid(serviceUUID);
    } else {
      if (irCharacteristic.valueUpdated()) {
        byte val;
        irCharacteristic.readValue(val);
        
        if (val == 1) { 
           ir_startup_counter++;
           if (machineState == STATE_IDLE && startupWindowStart == 0) {
             startupWindowStart = millis();
           }
        }

        lastIRStatus = val; 

        if (val == 1 || val == 2) {
          lastMotionTime = millis();
          hasDetectedMotionOnce = true;
        }
      }
    }
  } else {
    BLEDevice potentialDevice = BLE.available();
    if (potentialDevice && potentialDevice.localName() == "PortentaSensor") {
      BLE.stopScan();
      if (potentialDevice.connect() && potentialDevice.discoverAttributes()) {
         irCharacteristic = potentialDevice.characteristic(charUUID);
         if (irCharacteristic) {
           irCharacteristic.subscribe();
           centralPeripheral = potentialDevice;
           isBleConnected = true;
         }
      }
      if (!isBleConnected) BLE.scanForUuid(serviceUUID);
    }
  }
}

// --- WEB SERVER ---

void handleWebClient() {
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    String request = ""; 
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (request.length() < 50) request += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            if (request.indexOf("GET /sleep") >= 0) enterEcoMode();
            else if (request.indexOf("GET /wake") >= 0) exitEcoMode();
            sendHTML(client);
            break;
          } else { currentLine = ""; }
        } else if (c != '\r') { currentLine += c; }
      }
    }
    client.stop();
  }
}

void sendHTML(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  client.print("<html><head>");
  client.print("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  if (!isEcoMode) client.print("<meta http-equiv=\"refresh\" content=\"2\">");
  
  client.print("<style>");
  client.print("* { box-sizing: border-box; }"); 
  client.print("body { font-family: Helvetica, Arial, sans-serif; background: #f2f2f7; text-align: center; padding: 20px; color: #333; margin: 0; }");
  client.print(".card { background: white; border-radius: 16px; padding: 20px; margin: 15px auto; box-shadow: 0 2px 8px rgba(0,0,0,0.1); max-width: 400px; text-align: left; }");
  client.print(".header { font-size: 0.85rem; text-transform: uppercase; color: #8e8e93; font-weight: bold; margin-bottom: 8px; }");
  client.print(".big-val { font-size: 1.8rem; font-weight: 800; margin-bottom: 5px; }");
  client.print(".sub-val { font-size: 0.9rem; color: #8e8e93; margin-top: 2px; }");
  client.print(".status-green { color: #28a745; } .status-red { color: #dc3545; } .status-orange { color: #fd7e14; } .status-blue { color: #007bff; }");
  client.print(".btn { display: block; width: 100%; padding: 14px; margin-top: 10px; border-radius: 12px; text-decoration: none; font-weight: bold; text-align: center; color: white; -webkit-appearance: none; }");
  client.print(".btn-sleep { background: #007bff; } .btn-wake { background: #28a745; }");
  client.print("</style>");
  
  client.print("<script>");
  client.print("function formatTime(msDiff) {");
  client.print("  if (msDiff <= 0) return '--:--';");
  client.print("  var date = new Date(new Date().getTime() - msDiff);");
  client.print("  return date.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});");
  client.print("}");

  client.print("function updateTimes() {");
  client.print("  var now = "); client.print(millis()); client.print(";");
  
  client.print("  var lastMotion = "); client.print(lastMotionTime); client.print(";");
  client.print("  if (lastMotion > 0) {");
  client.print("    document.getElementById('seenTime').innerHTML = 'Last seen: ' + formatTime(now - lastMotion);");
  client.print("  }");

  client.print("  var startT = "); client.print(firstVibrationTime); client.print(";");
  client.print("  var state = "); client.print(machineState); client.print(";");
  client.print("  if (state > 0 && startT > 0) {");
  client.print("    document.getElementById('startTime').innerHTML = 'Started: ' + formatTime(now - startT);");
  client.print("  }");
  
  client.print("  var endT = "); client.print(lastVibrationTime); client.print(";");
  client.print("  if (state == 2 && endT > 0) {");
  client.print("    document.getElementById('endTime').innerHTML = 'Finished: ' + formatTime(now - endT);");
  client.print("  }");
  client.print("}");
  client.print("</script>");

  client.print("</head><body onload='updateTimes()'>");

  client.print("<h1>WashPal Insights</h1>");

  // --- CARD 1: LAUNDRY ROOM ---
  client.print("<div class=\"card\">");
  client.print("<div class=\"header\">Laundry Room</div>");
  if (machineState == STATE_WASHING) {
    // SPECIAL STATUS WHEN WASHING (SENSOR SLEEPING)
    client.print("<div class=\"big-val\" style='color:#aaa'>Sensor Paused</div>");
    client.print("<div class=\"sub-val\">Saving energy during cycle</div>");
  } else {
    if (!isBleConnected) {
      client.print("<div class=\"big-val status-red\">Disconnected</div>");
      client.print("<div class=\"sub-val\">IR Sensor offline</div>");
    } else {
      if (lastIRStatus == 1 || lastIRStatus == 2) {
         client.print("<div class=\"big-val status-red\">OCCUPIED</div>");
         client.print("<div class=\"sub-val\">Movement detected</div>");
      } else {
         client.print("<div class=\"big-val status-green\">EMPTY</div>");
         client.print("<div class=\"sub-val\" id=\"seenTime\">Loading...</div>");
      }
    }
  }
  client.print("</div>");

  // --- CARD 2: MACHINE STATUS ---
  if (!isEcoMode) {
    client.print("<div class=\"card\">");
    client.print("<div class=\"header\">Machine Status</div>");
    
    if (machineState == STATE_WASHING) {
       client.print("<div class=\"big-val status-orange\">RUNNING</div>");
       client.print("<div class=\"sub-val\" id=\"startTime\">Calculated...</div>");
       client.print("<div class=\"sub-val\">Vibrations: "); client.print(vibration_counter); client.print("</div>");
    } 
    else if (machineState == STATE_READY) {
       client.print("<div class=\"big-val status-blue\">READY TO BE TAKEN</div>");
       client.print("<div class=\"sub-val\" id=\"endTime\">Calculated...</div>");
       if (userVisitedForPickup) client.print("<div class=\"sub-val status-orange\">Pickup in progress...</div>");
       else client.print("<div class=\"sub-val\">Waiting for pickup</div>");
    } 
    else {
       client.print("<div class=\"big-val status-green\">IDLE</div>");
       client.print("<div class=\"sub-val\">Waiting for start</div>");
       
       if (vibration_counter > 0 || ir_startup_counter > 0) {
         client.print("<div style='margin-top:10px; border-top:1px solid #eee; padding-top:5px;'>");
         client.print("<div class=\"sub-val\">Vibrations: "); 
         client.print(vibration_counter); client.print("/50</div>");
         
         client.print("<div class=\"sub-val\">User Moves: "); 
         client.print(ir_startup_counter); client.print("/150</div>");
         client.print("</div>");
       }
    }
    client.print("</div>");
  }

  // --- CARD 3: SYSTEM ---
  client.print("<div class=\"card\">");
  if (isEcoMode) {
    client.print("<div class=\"header\">System Status</div>");
    client.print("<div class=\"big-val\">Sleeping</div>");
    client.print("<a href=\"/wake\" class=\"btn btn-wake\">WAKE UP</a>");
  } else {
    client.print("<a href=\"/sleep\" class=\"btn btn-sleep\">Enter Eco Mode</a>");
  }
  client.print("</div>");

  client.print("</body></html>");
}

void enterEcoMode() {
  if (isEcoMode) return;
  isEcoMode = true;
  Wire.beginTransmission(0x68); Wire.write(0x6B); Wire.write(0x40); Wire.endTransmission();
  if (isBleConnected) { centralPeripheral.disconnect(); isBleConnected = false; }
  BLE.stopScan();
  digitalWrite(LEDG, LOW); delay(200); digitalWrite(LEDG, HIGH);
}

void exitEcoMode() {
  if (!isEcoMode) return;
  isEcoMode = false;
  Wire.beginTransmission(0x68); Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission();
  BLE.scanForUuid(serviceUUID);
  digitalWrite(LEDG, LOW); delay(200); digitalWrite(LEDG, HIGH);
}
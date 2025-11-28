/*
  CENTRAL BOARD: Ricevitore BLE + Sensore Vibrazione Locale (GY521)
*/
#include <ArduinoBLE.h>
#include "GY521.h"
#include <Wire.h>

// --- CONFIGURAZIONE BLE ---
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* charUUID    = "19B10001-E8F2-537E-4F6C-D104768A1214";

// --- CONFIGURAZIONE GY521 (Giroscopio) ---
GY521 sensor(0x68);

uint32_t vibration_counter = 0;
double threshold = 1; // Soglia sensibilità vibrazione

// Variabili per la logica del giroscopio
float ax, ay, az;
float gx, gy, gz;
float t;

// Timer per lettura non bloccante
unsigned long lastGyroReadTime = 0;
const long gyroInterval = 100; // Leggi il giroscopio ogni 100ms

void setup() {
  Serial.begin(115200); // Aumentato baud rate per gestire più dati
  Wire.begin();
  
  // --- SETUP GY521 ---
  delay(100);
  if (sensor.wakeup() == false) {
    Serial.println("\tERRORE: Impossibile connettere GY521 (check wiring o address 0x68)");
    while(1); // Blocca se il sensore locale è rotto
  }
  sensor.setAccelSensitivity(0);  // 2g
  sensor.setGyroSensitivity(0);   // 250 degrees/s
  sensor.setThrottle(false);
  
  // Reset errori calibrazione
  sensor.axe = 0; sensor.aye = 0; sensor.aze = 0;
  sensor.gxe = 0; sensor.gye = 0; sensor.gze = 0;
  Serial.println("GY521 Pronto.");

  // --- SETUP BLE ---
  if (!BLE.begin()) {
    Serial.println("Avvio BLE fallito!");
    while (1);
  }
  Serial.println("Central BLE attiva. Scansione periferica IR...");
  BLE.scanForUuid(serviceUUID);
}

void loop() {
  // Cerca la periferica IR
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    if (peripheral.localName() == "PortentaSensor") {
      BLE.stopScan();
      monitorSystem(peripheral); // Entra nel loop principale
      BLE.scanForUuid(serviceUUID); // Riprendi scansione se disconnesso
    }
  }
  
  // NOTA: Se la periferica IR non è connessa, controlliamo comunque le vibrazioni?
  // Se vuoi controllare le vibrazioni anche quando il BLE è disconnesso, 
  // scommenta la riga qui sotto:
  // checkVibration(); 
}

// Funzione che gestisce SIA il BLE che il GY521 contemporaneamente
void monitorSystem(BLEDevice peripheral) {
  Serial.println("Connessione al sensore IR...");

  if (!peripheral.connect()) {
    Serial.println("Connessione fallita!");
    return;
  }
  if (!peripheral.discoverAttributes()) {
    Serial.println("Attributi non trovati!");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic sensorChar = peripheral.characteristic(charUUID);
  
  if (sensorChar && sensorChar.canSubscribe()) {
    sensorChar.subscribe();
  }

  Serial.println("SISTEMA ATTIVO: Monitoraggio Vibrazioni (Locale) + IR (Remoto)");

  while (peripheral.connected()) {
    
    // 1. GESTIONE BLE (IR REMOTO)
    if (sensorChar.valueUpdated()) {
      byte status;
      sensorChar.readValue(status);
      printIRStatus(status);
    }

    // 2. GESTIONE GY521 (VIBRAZIONE LOCALE)
    // Usiamo millis() invece di delay() per non bloccare il BLE
    if (millis() - lastGyroReadTime >= gyroInterval) {
      lastGyroReadTime = millis();
      checkVibration();
    }
  }
  Serial.println("Sensore IR disconnesso.");
}

void checkVibration() {
  ax = ay = az = 0;
  gx = gy = gz = 0;
  t = 0;

  // Leggi 20 campioni (dal tuo codice originale)
  for (int i = 0; i < 20; i++) {
    sensor.read();
    ax -= sensor.getAccelX();
    ay -= sensor.getAccelY();
    az -= sensor.getAccelZ();
    gx -= sensor.getGyroX();
    gy -= sensor.getGyroY();
    gz -= sensor.getGyroZ();
    t += sensor.getTemperature(); // Nota: questo rallenta un po' il loop
  }

  // Logica di rilevamento vibrazione
  // Nota: ho aggiunto parentesi per chiarire la logica OR/AND
  if ((ax > threshold && ay > threshold) || 
      (ax > threshold && az > threshold) || 
      (ay > threshold && az > threshold)) {
    
    vibration_counter++;
    
    Serial.print(">>> VIBRAZIONE RILEVATA! (Count: ");
    Serial.print(vibration_counter);
    Serial.println(")");
    
    // Qui puoi accendere un LED rosso locale se vuoi
    // digitalWrite(LEDR, LOW); 
  } else {
    // digitalWrite(LEDR, HIGH);
  }

  // Adjust calibration errors (Codice originale di autocalibrazione continua)
  sensor.axe += ax * 0.05;
  sensor.aye += ay * 0.05;
  sensor.aze += az * 0.05;
  sensor.gxe += gx * 0.05;
  sensor.gye += gy * 0.05;
  sensor.gze += gz * 0.05;
}

void printIRStatus(byte status) {
  Serial.print("[BLE IR] ");
  if (status == 0) Serial.println("Stanza Vuota");
  else if (status == 1) Serial.println("Movimento Rilevato!");
  else if (status == 2) Serial.println("Presenza Confermata");
}
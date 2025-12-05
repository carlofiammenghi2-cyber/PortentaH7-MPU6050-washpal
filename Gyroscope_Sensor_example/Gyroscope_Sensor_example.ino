/*
  CENTRAL BOARD: Ricevitore BLE + Sensore Vibrazione Locale (GY521)
*/
#include <ArduinoBLE.h>
#include "GY521.h"
#include <WiFi.h>
#include "arduino_secrets.h"
#include <Wire.h>

// --- CONFIGURAZIONE BLE ---
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* charUUID    = "19B10001-E8F2-537E-4F6C-D104768A1214";

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;             // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(80);

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

// Global state for webpage
volatile byte lastIRStatus = 0;
volatile int lastVibrationCount = 0;
volatile float lastAx = 0, lastAy = 0, lastAz = 0;


void setupWebServer() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  Serial.println("Access Point Web Server");
  // by default the local IP address of will be 192.168.3.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));
  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  //Create the Access point
  status = WiFi.beginAP(ssid,pass);
  if(status != WL_AP_LISTENING){
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your Wi-Fi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void handleWebClient() {
  // Check WiFi status changes (optional, from your original code)
  static uint8_t status = WL_IDLE_STATUS;
  if (status != WiFi.status()) {
    status = WiFi.status();
    if (status == WL_AP_CONNECTED) {
      Serial.println("Device connected to AP");
    } else {
      Serial.println("Device disconnected from AP");
    }
  }

  // Listen for incoming web clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("new web client");
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        
        if (c == '\n') {
          // End of HTTP request - send response
          if (currentLine.length() == 0) {
            // HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // HTML content with your sensor data
            client.print("<html><head>");
            client.print("<meta http-equiv=\"refresh\" content=\"5\">"); // Auto-refresh every 5s
            client.print("<style>");
            client.print("* { font-family: sans-serif; margin: 0; padding: 0; }");
            client.print("body { padding: 2em; font-size: 1.4em; text-align: center; background: #f0f8ff; }");
            client.print("h1 { color: #2c3e50; margin-bottom: 2em; }");
            client.print(".sensor { background: white; margin: 1em auto; padding: 1.5em; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 600px; }");
            client.print(".status0 { color: #27ae60; } .status1 { color: #f39c12; } .status2 { color: #e74c3c; }");
            client.print(".vibration { font-size: 1.2em; font-weight: bold; }");
            client.print("</style></head>");
            client.print("<body>");
            client.print("<h1>🏢 Sensor Monitor Dashboard</h1>");

            // IR Status (BLE from other board)
            client.print("<div class=\"sensor\">");
            client.print("<h2>📡 IR Status (Remoto)</h2>");
            client.print("<p class=\"status");
            client.print(lastIRStatus);
            client.print("\">");
            
            if (lastIRStatus == 0) client.print("✅ Stanza Vuota");
            else if (lastIRStatus == 1) client.print("⚠️ Movimento Rilevato!");
            else if (lastIRStatus == 2) client.print("🚨 Presenza Confermata");
            else client.print("❓ Sconosciuto");
            
            client.print("</p></div>");

            // Vibration Status (Local Gyro)
            client.print("<div class=\"sensor\">");
            client.print("<h2>📱 Vibrazione (Locale GY521)</h2>");
            client.print("<p class=\"vibration\">Contatore: ");
            client.print(lastVibrationCount);
            client.print("</p>");
            
            client.print("<p>Accelerazione media:<br>");
            client.print("Ax: ");
            client.print(lastAx, 2);
            client.print(" | Ay: ");
            client.print(lastAy, 2);
            client.print(" | Az: ");
            client.print(lastAz, 2);
            client.print("</p></div>");

            client.print("<p style=\"margin-top: 2em;\"><small>Auto-refresh ogni 5 secondi | IP: ");
            client.print(WiFi.localIP());
            client.print("</small></p>");
            client.print("</body></html>");

            // End HTTP response
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("web client disconnected");
  }
}

void setup() {
  Serial.begin(115200); // Aumentato baud rate per gestire più dati

  setupWebServer();
  
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

  // Handle WiFi/web clients every cycle
  handleWebClient();

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
    handleWebClient();  // keep responding to web browsers
    
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

    // store values for web page
    lastVibrationCount = vibration_counter;
    lastAx = ax;
    lastAy = ay;
    lastAz = az;
    
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
  lastIRStatus = status;   // expose to web UI
  if (status == 0) Serial.println("Stanza Vuota");
  else if (status == 1) Serial.println("Movimento Rilevato!");
  else if (status == 2) Serial.println("Presenza Confermata");
}
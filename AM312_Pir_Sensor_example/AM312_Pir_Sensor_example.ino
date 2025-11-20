int pirPin = 2;

int pirState = LOW;
int val = 0;

unsigned long motionDetectedTime = 0; // Time when motion was first detected
unsigned long noMotionStartTime = 0;  // Time when motion ended
unsigned long emptyDelay = 10000;    // 10 seconds delay for empty room message (just for testing, maybe it could be useful to use 30s)
unsigned long presenceDelay = 5000;   // 5 seconds for presence confirmation

bool presenceReported = false;
bool motionActive = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pirPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  val = digitalRead(pirPin);
  unsigned long currentTime = millis();

  if (val == HIGH) {
    if (!motionActive) {
      motionDetectedTime = currentTime; // first detection
      presenceReported = false; // reset presence report for new motion
      motionActive = true;
      Serial.println("Motion detected!");
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      // If motion continues, check if presence should be confirmed
      if (!presenceReported && (currentTime - motionDetectedTime >= presenceDelay)) {
        Serial.println("Someone is in the room");
        presenceReported = true;
      }
    }
  } else {
    if (motionActive) {
      Serial.println("Motion stopped!");
      digitalWrite(LED_BUILTIN, HIGH);
      motionActive = false;
      noMotionStartTime = currentTime;
    }

    // Check if no motion for more than 30 seconds (room empty)
    if ((currentTime - noMotionStartTime) > emptyDelay) {
      Serial.println("The washing machine room is empty");
      noMotionStartTime = currentTime; // prevent repeated messages
    }
  }
}

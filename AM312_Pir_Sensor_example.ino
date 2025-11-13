// int ledPin = 13;        // onboard LED
int pirPin = 2;         // PIR sensor output connected to D2

int pirState = LOW;     // current state of PIR output
int val = 0;            // variable for reading PIR status

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     
  pinMode(pirPin, INPUT);      
  Serial.begin(9600);          
}

void loop(){
  val = digitalRead(pirPin);   // read PIR sensor output
  if (val == HIGH) {           
    if (pirState == LOW) {
      Serial.println("Motion detected!");
      digitalWrite(LED_BUILTIN, LOW); // turn LED ON
      pirState = HIGH;
    }
  } else {
    if (pirState == HIGH){
      Serial.println("Motion stopped!");
      digitalWrite(LED_BUILTIN, HIGH);  // turn LED OFF
      pirState = LOW;
    }
  }
}

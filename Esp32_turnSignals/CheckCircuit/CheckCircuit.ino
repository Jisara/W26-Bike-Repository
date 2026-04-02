void setup() {
  pinMode(32, INPUT_PULLUP); // Use internal resistor to hold pin HIGH
  pinMode(33, OUTPUT);
  pinMode(25, INPUT_PULLUP); // Use internal resistor to hold pin HIGH
  pinMode(26, OUTPUT);
  Serial.begin(115200);
}
void loop() {
  //checks left
  int sensorState = digitalRead(32);
  digitalWrite(33, HIGH);
  if (sensorState == LOW) {
    // Circuit is CLOSED (connected to GND)
    Serial.println("Circuit Closed");
  } else {
    // Circuit is OPEN
    Serial.println("Circuit Open");
  }

  //checks right
  int sensorState2 = digitalRead(25);
  digitalWrite(26, HIGH);
  if (sensorState2 == LOW) {
    // Circuit is CLOSED (connected to GND)
    Serial.println("Circuit Closed");
  } else {
    // Circuit is OPEN
    Serial.println("Circuit Open");
  }
}
int turnLeftIN = 1;
int turnLeftOUT = 2;
int turnRightIN = 3;
int turnRightOUT = 4;

unsigned long previousMillisLeft = 0;
unsigned long previousMillisRight = 0;
long intervalLeft = 1000; // blink rate for led1
long intervalRight = 1000; // blink rate for led2
int leftState = LOW; // state of the LED
int rightState = LOW; // state of the LED
bool leftOn = false; //should the LED be on?
bool rightOn = false; // should the LED be on?

void setup() {
  // put your setup code here, to run once:
  pinMode(turnLeftIN, INPUT);
  pinMode(turnRightIN, INPUT);
  pinMode(turnLeftOUT, OUTPUT);
  pinMode(turnRightOUT, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long currentMillis = millis();
  leftState = digitalRead(turnLeftIN);
  rightState = digitalRead(turnRightIN);


//may need to change sensitivity
  leftOn = (leftState == LOW) ? leftOn : (leftOn == false) ? true : false;
  rightOn = (rightState == LOW) ? rightOn : (rightOn == false) ? true : false;

  //need to make sure if leftON = false, led is LOW
  //aka make sure it does not stay on
  if(leftOn == false && leftState == HIGH){
    leftState = LOW;
    digitalWrite(turnLeftOUT, leftState);
  }
  if(rightOn == false && rightState == HIGH){
    rightState = LOW;
    digitalWrite(turnRightOUT, rightState);
  }
  
  // Blink LED 1
  if (leftOn == true && currentMillis - previousMillisLeft >= intervalLeft) { //has the interval passed since the last blink
    previousMillisLeft = currentMillis; // update previousMillis (so that it doesn't repeat forever)
    leftState = (leftState == LOW) ? HIGH : LOW; //changes high to low or low to high
    digitalWrite(turnLeftOUT, leftState); //tell the board to send the signal
  }

   //Blink LED 2
  if (rightOn == true && currentMillis - previousMillisRight >= intervalRight) {
    previousMillisRight = currentMillis;
    rightState = (rightState == LOW) ? HIGH : LOW;
    digitalWrite(turnRightOUT, rightState);
  }

}

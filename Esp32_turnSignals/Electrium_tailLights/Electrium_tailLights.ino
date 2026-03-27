int turnLeftIN = 32;
int turnLeftOUT = 33;
int turnRightIN = 25;
int turnRightOUT = 26;

unsigned long previousMillisLeft = 0;
unsigned long previousMillisRight = 0;
long intervalLeft = 1000; // blink rate for led1
long intervalRight = 1000; // blink rate for led2

//unsigned long lastPressedLeft = 0; // tracks when the button was last pressed

int leftOUTState = LOW; // state of output
int rightOUTState = LOW; // state of output

int leftINState = LOW; // state of the LED
int rightINState = LOW; // state of the LED

bool leftOn = false; //should the LED be on?
bool rightOn = false; // should the LED be on?


void setup() {
 // put your setup code here, to run once:
  pinMode(turnLeftIN, INPUT_PULLUP);
  pinMode(turnRightIN, INPUT_PULLUP);
  pinMode(turnLeftOUT, OUTPUT);
  pinMode(turnRightOUT, OUTPUT);

  Serial.begin(115200);
  Serial.println("READY!!!");
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  leftINState = digitalRead(turnLeftIN);
  rightINState = digitalRead(turnRightIN);

//2 things -- make buffer before button change registors
//-- make sure held presses do not constantly flip the state


  if(leftINState == LOW){
    delay(500);
    leftOn = !leftOn; // change state of leftOne

    if(!leftOn){
      digitalWrite(turnLeftOUT, LOW);
      leftOUTState = LOW;
    }    
  }

  if(rightINState == LOW){
    delay(500);
    rightOn = !rightOn; // change state of leftOne

    if(!rightOn){
      digitalWrite(turnRightOUT, LOW);
      rightOUTState = LOW;
    }    
  }


  if (leftOn == true && currentMillis - previousMillisLeft >= intervalLeft) { //has the interval passed since the last blink
    previousMillisLeft = currentMillis; // update previousMillis (so that it doesn't repeat forever)
    leftOUTState = (leftOUTState == LOW) ? HIGH : LOW; //changes high to low or low to high
    digitalWrite(turnLeftOUT, leftOUTState); //tell the board to send the signal

  }

  if (rightOn == true && currentMillis - previousMillisRight >= intervalRight) { //has the interval passed since the last blink
    previousMillisRight = currentMillis; // update previousMillis (so that it doesn't repeat forever)
    rightOUTState = (rightOUTState == LOW) ? HIGH : LOW; //changes high to low or low to high
    digitalWrite(turnRightOUT, rightOUTState); //tell the board to send the signal

  }
  

}


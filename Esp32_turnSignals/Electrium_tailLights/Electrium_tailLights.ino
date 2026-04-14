int turnLeftIN = 32;
int turnLeftOUT = 33;
int turnRightIN = 25;
int turnRightOUT = 26;

unsigned long previousMillisLeft = 0;
unsigned long previousMillisRight = 0;
long intervalLeft = 1000; // blink rate for led1
long intervalRight = 1000; // blink rate for led2

unsigned long lastPressedLeft = 0; // tracks when the button was last pressed
unsigned long lastPressedRight = 0; // tracks when the button was last pressed

long debounceWait = 10; // amount of time to debounce
bool debouncedLeft = true; //true == not debounced yet
bool debouncedRight = true; //true == not debounced yet


int leftOUTState = LOW; // state of output
int rightOUTState = LOW; // state of output

int leftINState = LOW; // state of the LED
int rightINState = LOW; // state of the LED

int leftPreviousINState = HIGH; // last recorded state of input
int rightPreviousINState = HIGH; // last recorded state of input


bool leftOn = false; //should the LED be on?
bool rightOn = false; // should the LED be on?


void setup() {
  pinMode(turnLeftIN, INPUT_PULLUP);
  pinMode(turnRightIN, INPUT_PULLUP);
  pinMode(turnLeftOUT, OUTPUT);
  pinMode(turnRightOUT, OUTPUT);

  Serial.begin(115200);
}

void loop() {
  unsigned long currentMillis = millis();
  
  //reads new button state
  leftINState = digitalRead(turnLeftIN);
  rightINState = digitalRead(turnRightIN);

//triggers when button is pressed (input state changes)
  if(leftPreviousINState != leftINState){
    lastPressedLeft = currentMillis;
    debouncedLeft = false;
    leftPreviousINState = leftINState;
  }

  if(rightPreviousINState != rightINState){
    lastPressedRight = currentMillis;
    debouncedRight = false;
    rightPreviousINState = rightINState;
  }

  //changes boolean that tells the system that the light should be blinking
  if(leftINState == LOW && !debouncedLeft && currentMillis - lastPressedLeft >= debounceWait){
    leftOn = !leftOn; 
    debouncedLeft = true;

    //turn light off if button is pressed when light is on
    if(!leftOn && leftOUTState == HIGH){
      digitalWrite(turnLeftOUT, LOW);
      leftOUTState = LOW;
    }    
  }

  if(rightINState == LOW && !debouncedRight && currentMillis - lastPressedRight >= debounceWait){
    rightOn = !rightOn; 
    debouncedRight = true;

    if(!rightOn){
      digitalWrite(turnRightOUT, LOW);
      rightOUTState = LOW;

    }    
  }

  //blinks light at specified interval
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
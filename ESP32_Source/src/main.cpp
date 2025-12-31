#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ezButton.h>

#define PIN_1 1
#define PIN_2 2
#define PIN_3 3
#define PIN_4 4
#define PIN_5 5
#define PIN_6 6
#define PIN_7 7
#define PIN_8 8
#define PIN_9 9
#define PIN_10 10 
#define PIN_11 11
#define PIN_12 12
#define PIN_13 13
#define PIN_14 14
#define PIN_15 15

#define SETMODE_STRENGTH4      4
#define SETMODE_HOURS          3
#define SETMODE_MINUTES        2
#define SETMODE_10SEC          1

int currentSetMode = SETMODE_10SEC;
int32_t timerSetTime = 0;

#define MAX_STRENGTH_LEVEL 4
int currentStrengthLevel = 0;

// Define the LED pin
const int ledPin = 2; // GPIO pin for the LED

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


ezButton button_up(PIN_3);
ezButton button_down(PIN_4);
ezButton button_mode(PIN_5);
ezButton button_start(PIN_6);

int timeCurrentRemained = 0;
unsigned long startMillis = 0; 
unsigned long pausedMillis = 0;
#define TIMERSTATUS_RUNNING 2
#define TIMERSTATUS_PAUSED 1
#define TIMERSTATUS_STOPPED 0
int fTimerRunning = TIMERSTATUS_STOPPED;



#include <AccelStepper.h>

// Define some steppers and the pins the will use
// Initialize with the 28BYJ-specific sequence: IN1, IN3, IN2, IN4
AccelStepper stepper(AccelStepper::HALF4WIRE, PIN_13, PIN_11, PIN_12, PIN_10);

void upadteDisplay();

void setup() {
  Serial.begin(115200);

  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);

  Wire.begin();
  Wire.setPins(PIN_8, PIN_9);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(1000);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Hello, world!!!!");
  display.display(); 
  delay(1000);

  upadteDisplay();

  stepper.setMaxSpeed(2000.0);
  stepper.setAcceleration(500.0);
  stepper.moveTo(0);
  delay(2000);
  // stepper.setSpeed(200);
}


void upadteDisplay() {
  char buffer [32];
  display.clearDisplay();

  if (fTimerRunning == TIMERSTATUS_STOPPED) {
    display.setCursor(0, 0);
    switch (currentSetMode) {
      case SETMODE_10SEC:
        display.println("Set 10 sec");
        break;
      case SETMODE_MINUTES:
        display.println("Set Min");
        break;
      case SETMODE_HOURS:
        display.println("Set Hours");
        break;            
      case SETMODE_STRENGTH4:
        display.println("Strength");
        break;
    }

    display.setCursor(0, 18);
    int hours = timerSetTime / 3600;
    int minutes = (timerSetTime % 3600) / 60;
    int seconds = timerSetTime % 60;
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    display.println(buffer);
  } else {
    display.setCursor(0, 0);
    if (fTimerRunning == TIMERSTATUS_RUNNING) {
      display.println("Running");
    } else if (fTimerRunning == TIMERSTATUS_PAUSED) {
      display.println("Paused");
    }

    display.setCursor(0, 18);
    int hours = timeCurrentRemained / 3600;
    int minutes = (timeCurrentRemained % 3600) / 60;
    int seconds = timeCurrentRemained % 60;
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    display.println(buffer);    
  }
  display.setCursor(0, 40);
  sprintf(buffer, "Strength %d", currentStrengthLevel);
  display.println(buffer);
  display.display();  
}

void loop() {
  bool fUpdateDisplay = false;
  unsigned long currentMillis = millis();
  
  button_start.loop();
  button_down.loop();
  button_up.loop();
  button_mode.loop();

  int distanceToGo = stepper.distanceToGo();
  if (distanceToGo == 0) {
    // delay(1000);
    int currentPos = stepper.currentPosition();
    Serial.println("currentPos = " + String(currentPos));
    Serial.println("distanceToGo == 0");
    if (currentPos == 0) {
      stepper.moveTo(2048);
    } else {
      stepper.moveTo(0);
    }
  }
  stepper.run();

  // put your main code here, to run repeatedly:
  // Serial.println("running loop");

  // display.clearDisplay();
  // display.setCursor(0, 10);
  // display.println("ON");
  // display.display(); 
  // Turn the LED on
  // digitalWrite(ledPin, HIGH);
  // delay(1000); // Wait for 1 second

  // display.clearDisplay();
  // display.setCursor(0, 10);
  // display.println("OFF");
  // display.display(); 
  // Turn the LED off
  // digitalWrite(ledPin, LOW);
  // delay(1000); // Wait for 1 second


  if(button_start.isPressed()) {
    Serial.println("The button_start is pressed");
    fUpdateDisplay = true;
  }

  if(button_start.isReleased()) {
    Serial.println("The button_start is released");

    if (fTimerRunning == TIMERSTATUS_STOPPED) {
      fTimerRunning = TIMERSTATUS_RUNNING;
      startMillis = millis();
      pausedMillis = -1;
      timeCurrentRemained = timerSetTime;
      // myServo.write(SERVO_ANGLE_RUNNING);
    } else if (fTimerRunning == TIMERSTATUS_RUNNING) {
      fTimerRunning = TIMERSTATUS_PAUSED;
      pausedMillis= millis();
      // myServo.write(SERVO_ANGLE_STOPPED);
    } else  if (fTimerRunning == TIMERSTATUS_PAUSED) {
      fTimerRunning = TIMERSTATUS_RUNNING;
      // myServo.write(SERVO_ANGLE_RUNNING);
    }
    fUpdateDisplay = true;
  }

  if(button_up.isPressed()) {
    Serial.println("The button_up is pressed");
    fUpdateDisplay = true;
  }

  if(button_up.isReleased()) {
    Serial.println("The button_up is released");
    if (fTimerRunning == false) {
        switch (currentSetMode) {
          case SETMODE_10SEC:
            timerSetTime += 10;
            break;
          case SETMODE_MINUTES:
            timerSetTime += 60;
            break;
          case SETMODE_HOURS:
            timerSetTime += 3600;
            break;            
          case SETMODE_STRENGTH4:
            currentStrengthLevel++;
            if (currentStrengthLevel > MAX_STRENGTH_LEVEL) {
              currentStrengthLevel = MAX_STRENGTH_LEVEL;
            }
            break;
        } 
        if (timerSetTime >= 3600 * 5) {
          timerSetTime = 3600 * 5;
        }             
        fUpdateDisplay = true;
    }
  }
  
  if(button_down.isPressed()) {
    Serial.println("The button_down is pressed");
    fUpdateDisplay = true;
  }

  if(button_down.isReleased()) {
    Serial.println("The button_down is released");
    if (fTimerRunning == false) {
        switch (currentSetMode) {
          case SETMODE_10SEC:
            timerSetTime -= 10;
            break;
          case SETMODE_MINUTES:
            timerSetTime -= 60;
            break;
          case SETMODE_HOURS:
            timerSetTime -= 3600;
            break;
          case SETMODE_STRENGTH4:
            currentStrengthLevel--;
            if (currentStrengthLevel < 0) {
              currentStrengthLevel = 0;
            }
            break;
        }
        if (timerSetTime < 0) {
          timerSetTime = 0;
        }      
        fUpdateDisplay = true;
    }
  }
  if(button_mode.isPressed()) {
    Serial.println("The button_mode is pressed");
    fUpdateDisplay = true;
  }

  if(button_mode.isReleased()) {
    Serial.println("The button_mode is released");
    currentSetMode++;
    if (currentSetMode > SETMODE_STRENGTH4) {
      currentSetMode = SETMODE_10SEC;
    }
  }

  if (fTimerRunning == TIMERSTATUS_RUNNING) {
    if (pausedMillis != -1) {
      startMillis += (millis() - pausedMillis);
      pausedMillis = -1;
    }
    int newTimeRemained = timerSetTime - (int)((millis() - startMillis) / 1000.0);
    if (newTimeRemained == 0) {
      timeCurrentRemained = newTimeRemained;
      fTimerRunning = TIMERSTATUS_STOPPED;
      // myServo.write(SERVO_ANGLE_STOPPED);
      fUpdateDisplay = true;
    } else if (newTimeRemained != timeCurrentRemained) {
      timeCurrentRemained = newTimeRemained;
      fUpdateDisplay = true;
    }
  }

  if (fUpdateDisplay) {
    upadteDisplay();
  }


}


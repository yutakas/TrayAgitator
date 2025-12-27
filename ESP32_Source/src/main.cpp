#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ezButton.h>

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


#define TIMER_INTERRUPT_DEBUG       0
#define ISR_SERVO_DEBUG             0
// Select different ESP32 timer number (0-3) to avoid conflict
#define USE_ESP32_TIMER_NO          3
// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "ESP32_ISR_Servo.h"

// Published values for SG90 servos; adjust if needed
#define MIN_MICROS      800  //544
#define MAX_MICROS      2450
#define SERVO_PIN      10
#define SERVO_ANGLE_STOPPED    0
#define SERVO_ANGLE_RUNNING    90
int servoIndex  = -1;
int servoDirection = 1; // 1: increasing angle, -1: decreasing angle

ezButton button_up(3);
ezButton button_down(4);
ezButton button_mode(5);
ezButton button_start(6);

int timeCurrentRemained = 0;
unsigned long startMillis = 0; 
unsigned long pausedMillis = 0;
#define TIMERSTATUS_RUNNING 2
#define TIMERSTATUS_PAUSED 1
#define TIMERSTATUS_STOPPED 0
int fTimerRunning = TIMERSTATUS_STOPPED;

void upadteDisplay();

void setup() {
  Serial.begin(115200);

  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);

  Wire.begin();
  Wire.setPins(8,9);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

	ESP32_ISR_Servos.useTimer(USE_ESP32_TIMER_NO);
	servoIndex = ESP32_ISR_Servos.setupServo(SERVO_PIN, MIN_MICROS, MAX_MICROS);




  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Hello, world!!!!");
  display.display(); 
  delay(1000);

  upadteDisplay();
}

void moveServo() {
  static unsigned long lastMoveMillis = 0;
  static int lastPosition = -1;
  if (millis() - lastMoveMillis < 50) {
    return;
  }
  if (servoIndex != -1) {
    int position = ESP32_ISR_Servos.getPosition(servoIndex);
    if ((lastPosition != -1) && (lastPosition != position)) {
      return; // wait until last move is finished
    }
    Serial.println("Servo pos = " + String(position));
    if (position == SERVO_ANGLE_RUNNING) {
      servoDirection = -1;
      Serial.println("Servo is running");
      delay(500);
    } else if (position == SERVO_ANGLE_STOPPED) {
      servoDirection = 1;
      Serial.println("Servo is stopped");
      delay(500);
    } 
    lastPosition = position + servoDirection * 10;
    ESP32_ISR_Servos.setPosition(servoIndex, lastPosition);
    lastMoveMillis = millis();
    // delay(1000);
    // delay(1000);


    // for (int position = 0; position <= 180; position++) 
    // { 
    //   // goes from 0 degrees to 180 degrees
    //   // in steps of 1 degree

    //   if (position %30 == 0)
    //   {
    //     Serial.println("Servo1 pos = " + String(position) + ", Servo2 pos = " + String(180 - position) );
    //   }
          
    //   ESP32_ISR_Servos.setPosition(servoIndex1, position);
    //   // waits 30ms for the servo to reach the position
    //   delay(30);
    // }
  }
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

  // moveServo();

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


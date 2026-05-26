#include <Arduino.h>
#include <WiFi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ezButton.h>
#include <Preferences.h>
#include <esp_system.h>

Preferences prefs;

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
bool cancelGestureHandled = false;




// functins for BEEP
#define LEDC_CHANNEL_FOR_BEEP 0
void beep_start();
void beep_stop();
void beep_run();
void beep_short();
void upadteDisplay();
void setSessionActiveFlag(bool active);



#define STEPPER_STEP_PIN PIN_10
#define STEPPER_DIRECT_PIN PIN_11
#define STEPPER_ENABLE_PIN PIN_12

int pulseWidthMicros = 20;  // microseconds
int stepIntervalMicros = 1000; // base interval at max strength (1ms = 1000µs)

//--- test direct control of stepper motor ---
void testStepperDirectControl() {
  Serial.println("Start testStepperDirectControl");
  int numberOfSteps = 1000;

  digitalWrite(STEPPER_DIRECT_PIN, HIGH);
  digitalWrite(STEPPER_ENABLE_PIN, HIGH); 

  for(int n = 0; n < numberOfSteps; n++) {
    digitalWrite(STEPPER_STEP_PIN, HIGH);
    delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
    digitalWrite(STEPPER_STEP_PIN, LOW);
    
    delayMicroseconds(stepIntervalMicros);
    
    // digitalWrite(ledPin, !digitalRead(ledPin));
  }
  // digitalWrite(stepPin, HIGH);
  // delay(2000);
  digitalWrite(STEPPER_ENABLE_PIN, LOW); 
  Serial.println("End testStepperDirectControl");
}


// Human-readable last-reset cause. Diagnoses standalone reboots (no USB needed):
// BROWNOUT = power sag, PANIC = firmware crash, *WDT = a hang/lockup.
const char* resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "Power-on";
    case ESP_RST_EXT:       return "Ext reset";
    case ESP_RST_SW:        return "SW reset";
    case ESP_RST_PANIC:     return "PANIC/crash";
    case ESP_RST_INT_WDT:   return "Int WDT";
    case ESP_RST_TASK_WDT:  return "Task WDT";
    case ESP_RST_WDT:       return "Other WDT";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "Unknown";
  }
}

//--- SETUP and LOOP ---
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);

  // Restore saved settings from NVS
  prefs.begin("agitator", true); // read-only
  timerSetTime = prefs.getInt("timerSetTime", 0);
  currentStrengthLevel = prefs.getInt("strengthLevel", 0);
  int sessionWasActive = prefs.getInt("sessionOn", 0); // was a run in progress at last reset?
  prefs.end();

  WiFi.mode(WIFI_OFF);

  // Initialize the LED pin as an output
  // pinMode(ledPin, OUTPUT);
  pinMode(STEPPER_ENABLE_PIN, OUTPUT);
  digitalWrite(STEPPER_ENABLE_PIN, LOW); 

  // buzzzer pin
  ledcSetup(LEDC_CHANNEL_FOR_BEEP, 2000, 8); // Channel 1, 2kHz, 8-bit resolution
  ledcAttachPin(PIN_7, LEDC_CHANNEL_FOR_BEEP); // Attach the pin to a channel

  // I2C setup
  Wire.begin();
  Wire.setPins(PIN_8, PIN_9);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(1000);

  // Report why the board last reset so standalone reboots can be diagnosed
  // without a USB/serial connection, plus whether a run was active at the time.
  esp_reset_reason_t resetReason = esp_reset_reason();
  Serial.printf("Last reset reason: %d (%s), session active: %d\n",
                resetReason, resetReasonStr(resetReason), sessionWasActive);

  // No run is in progress immediately after boot, so clear the flag now; it only
  // returns to 1 once a real session starts. Keeps the next reading accurate.
  setSessionActiveFlag(false);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Last reset:");
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.println(resetReasonStr(resetReason));
  display.setTextSize(1);
  display.setCursor(0, 38);
  display.print("When: ");
  display.println(sessionWasActive ? "MOTOR RUNNING" : "idle");
  display.setCursor(0, 52);
  display.println("Press any button");
  display.display();

  button_up.setDebounceTime(50);
  button_down.setDebounceTime(50);
  button_mode.setDebounceTime(50);
  button_start.setDebounceTime(50);

  // Hold on the reset-reason screen until the user acknowledges with a button,
  // so a reboot that happens unattended can't scroll past before it's read.
  for (;;) {
    button_up.loop();
    button_down.loop();
    button_mode.loop();
    button_start.loop();
    if (button_up.isPressed()    || button_up.isReleased()    ||
        button_down.isPressed()  || button_down.isReleased()  ||
        button_mode.isPressed()  || button_mode.isReleased()  ||
        button_start.isPressed() || button_start.isReleased()) {
      break;
    }
    delay(10);
  }

  upadteDisplay();

  pinMode(STEPPER_DIRECT_PIN, OUTPUT);
  pinMode(STEPPER_STEP_PIN, OUTPUT);

  // testStepperDirectControl();
}



//--- Persist settings to NVS ---
void savePrefs() {
  prefs.begin("agitator", false); // read-write
  prefs.putInt("timerSetTime", timerSetTime);
  prefs.putInt("strengthLevel", currentStrengthLevel);
  prefs.end();
}

// Persist whether an agitation session is active. On the next boot setup() reads
// this back so a power-loss reset can be correlated with motor activity: if it's
// still 1, the board died mid-run (NVS is wiped to 0 cleanly on stepper_stop()).
// NVS skips writes when the value is unchanged, so wear stays negligible.
void setSessionActiveFlag(bool active) {
  prefs.begin("agitator", false); // read-write
  prefs.putInt("sessionOn", active ? 1 : 0);
  prefs.end();
}

//--- BEEP functions ---
bool fBeepRunning = false;
unsigned long beepStartMillis = 0;
unsigned long beepDurationMillis = 500; //2000;
void beep_start() {
  ledcWriteTone(LEDC_CHANNEL_FOR_BEEP, 440 * 2); // Play 880 Hz tone
  beepStartMillis = millis();
  fBeepRunning = true;
}

void beep_stop() {
  ledcWriteTone(LEDC_CHANNEL_FOR_BEEP, 0);
  fBeepRunning = false;
} 

void beep_run() {
  if (fBeepRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - beepStartMillis >= beepDurationMillis) {
      // Stop the beep
      beep_stop();
    }
  }
}

void beep_short() {
  ledcWriteTone(LEDC_CHANNEL_FOR_BEEP, 440 * 4);
  delay(50);
  ledcWriteTone(LEDC_CHANNEL_FOR_BEEP, 0);
}

//--- Update Display ---
void upadteDisplay() {
  char buffer [32];
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2); // main UI is size 2; set explicitly so earlier screens don't leak state

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

  display.setTextSize(1);
  display.setCursor(0, 40);
  sprintf(buffer, "Strength %d", currentStrengthLevel);
  display.println(buffer);
 
  // Internal chip die temperature on the bottom row (small). Trend gauge only:
  // reads the silicon temp (warmer than ambient), accuracy ~+/-2-3 C.
  display.setTextSize(1);
  display.setCursor(0, 56);
  sprintf(buffer, "Chip %.1fC", temperatureRead());
  display.println(buffer);

  display.display();
}

bool fStepperRunning = false;
unsigned long stepperLastRunMicros = 0;
unsigned long stepperCycleStartMillis = 0;
bool stepperInRestPhase = false;

#define STEPPER_RUN_MS  2000
#define STEPPER_REST_MS 8000

void stepper_start() {
  setSessionActiveFlag(true);  // record "run active" before energizing the coils
  stepperLastRunMicros = micros();
  stepperCycleStartMillis = millis();
  stepperInRestPhase = false;
  fStepperRunning = true;
  digitalWrite(STEPPER_DIRECT_PIN, LOW);
  digitalWrite(STEPPER_ENABLE_PIN, HIGH);
}

void stepper_stop() {
  fStepperRunning = false;
  stepperInRestPhase = false;
  digitalWrite(STEPPER_ENABLE_PIN, LOW);
  setSessionActiveFlag(false);  // clean stop: clear the flag after de-energizing
}

void stepper_run() {
  if (fStepperRunning) {
    unsigned long elapsed = millis() - stepperCycleStartMillis;

    if (stepperInRestPhase) {
      if (elapsed >= STEPPER_REST_MS) {
        // Rest phase over, start rotating
        stepperInRestPhase = false;
        stepperCycleStartMillis = millis();
        stepperLastRunMicros = micros();
        digitalWrite(STEPPER_ENABLE_PIN, HIGH);
      }
    } else {
      if (elapsed >= STEPPER_RUN_MS) {
        // Run phase over, start resting
        stepperInRestPhase = true;
        stepperCycleStartMillis = millis();
        digitalWrite(STEPPER_ENABLE_PIN, LOW);
      } else {
        // Stepping
        unsigned long currentMicros = micros();
        unsigned long interval = stepIntervalMicros + (MAX_STRENGTH_LEVEL - currentStrengthLevel) * 200UL;
        if (currentMicros - stepperLastRunMicros >= interval) {
          stepperLastRunMicros = currentMicros;
          digitalWrite(STEPPER_STEP_PIN, HIGH);
          delayMicroseconds(pulseWidthMicros);
          digitalWrite(STEPPER_STEP_PIN, LOW);
        }
      }
    }
  }
} 

// the loop function runs over and over again forever
void loop() {
  bool fUpdateDisplay = false;
  unsigned long currentMillis = millis();

  // Refresh ~1x/sec so the chip-temperature readout updates even while idle
  // (during a run the countdown already forces a redraw each second).
  static unsigned long lastPeriodicRefresh = 0;
  if (currentMillis - lastPeriodicRefresh >= 1000) {
    lastPeriodicRefresh = currentMillis;
    fUpdateDisplay = true;
  }

  button_start.loop();
  button_down.loop();
  button_up.loop();
  button_mode.loop();
  beep_run();


  stepper_run();

  // Track physical button hold state using edge events
  // PIN_5 (mode) rests LOW: isReleased()=physical press, isPressed()=physical release
  // PIN_6 (start) rests HIGH: isPressed()=physical press, isReleased()=physical release
  static bool startHeld = false;
  static bool modeHeld = false;

  if (button_start.isPressed()) { startHeld = true; Serial.printf("[BTN] start HELD (modeHeld=%d cancelFlag=%d timer=%d)\n", modeHeld, cancelGestureHandled, fTimerRunning); fUpdateDisplay = true; }
  if (button_start.isReleased()) { startHeld = false; Serial.printf("[BTN] start FREE (modeHeld=%d cancelFlag=%d timer=%d)\n", modeHeld, cancelGestureHandled, fTimerRunning); }
  if (button_mode.isReleased()) { modeHeld = true; Serial.printf("[BTN] mode  HELD (startHeld=%d cancelFlag=%d timer=%d)\n", startHeld, cancelGestureHandled, fTimerRunning); }
  if (button_mode.isPressed()) { modeHeld = false; Serial.printf("[BTN] mode  FREE (startHeld=%d cancelFlag=%d timer=%d)\n", startHeld, cancelGestureHandled, fTimerRunning); }
  if (button_up.isReleased()) { Serial.println("[BTN] up    HELD"); fUpdateDisplay = true; }
  if (button_up.isPressed()) { Serial.println("[BTN] up    FREE"); }
  if (button_down.isReleased()) { Serial.println("[BTN] down  HELD"); fUpdateDisplay = true; }
  if (button_down.isPressed()) { Serial.println("[BTN] down  FREE"); }

  // Cancel timer: both mode + start physically held simultaneously
  if (startHeld && modeHeld) {
    if (!cancelGestureHandled && fTimerRunning != TIMERSTATUS_STOPPED) {
      Serial.println("[CANCEL] fired");
      fTimerRunning = TIMERSTATUS_STOPPED;
      stepper_stop();
      beep_short();
      fUpdateDisplay = true;
      cancelGestureHandled = true;
    }
  }
  // Reset cancel flag once both buttons are physically released and no pending events
  if (cancelGestureHandled && !startHeld && !modeHeld
      && !button_start.isReleased() && !button_mode.isPressed()) {
    Serial.println("[CANCEL] flag reset");
    cancelGestureHandled = false;
  }

  if(button_start.isReleased() && !cancelGestureHandled) {
    Serial.printf("[ACTION] start toggle (timer=%d)\n", fTimerRunning);

    if (fTimerRunning == TIMERSTATUS_STOPPED) {
      if (timerSetTime > 0) {
        fTimerRunning = TIMERSTATUS_RUNNING;
        startMillis = millis();
        pausedMillis = -1;
        timeCurrentRemained = timerSetTime;
        beep_short();
        stepper_start();
      }
    } else if (fTimerRunning == TIMERSTATUS_RUNNING) {
      fTimerRunning = TIMERSTATUS_PAUSED;
      pausedMillis= millis();
      stepper_stop();
      beep_short();
    } else  if (fTimerRunning == TIMERSTATUS_PAUSED) {
      fTimerRunning = TIMERSTATUS_RUNNING;
      stepper_start();
      beep_short();
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
            beep_short();
            break;
          case SETMODE_MINUTES:
            timerSetTime += 60;
            beep_short();
            break;
          case SETMODE_HOURS:
            timerSetTime += 3600;
            beep_short();
            break;            
          case SETMODE_STRENGTH4:
            currentStrengthLevel++;
            if (currentStrengthLevel > MAX_STRENGTH_LEVEL) {
              currentStrengthLevel = MAX_STRENGTH_LEVEL;
            }
            beep_short();
            break;
        } 
        if (timerSetTime >= 3600 * 5) {
          timerSetTime = 3600 * 5;
        }
        savePrefs();
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
            beep_short();
            break;
          case SETMODE_MINUTES:
            timerSetTime -= 60;
            beep_short();
            break;
          case SETMODE_HOURS:
            timerSetTime -= 3600;
            beep_short();
            break;
          case SETMODE_STRENGTH4:
            currentStrengthLevel--;
            if (currentStrengthLevel < 0) {
              currentStrengthLevel = 0;
            }
            beep_short();
            break;
        }
        if (timerSetTime < 0) {
          timerSetTime = 0;
        }
        savePrefs();
        fUpdateDisplay = true;
    }
  }
  if(button_mode.isPressed()) {
    Serial.println("The button_mode is pressed");
    fUpdateDisplay = true;
  }

  if(button_mode.isReleased() && !cancelGestureHandled) {
    Serial.println("The button_mode is released");
    currentSetMode++;
    if (currentSetMode > SETMODE_STRENGTH4) {
      currentSetMode = SETMODE_10SEC;
    }
    beep_short();
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
      stepper_stop();
      fUpdateDisplay = true;
      beep_start();
    } else if (newTimeRemained != timeCurrentRemained) {
      timeCurrentRemained = newTimeRemained;
      fUpdateDisplay = true;
    }
  }

  if (fUpdateDisplay) {
    upadteDisplay();
  }


}


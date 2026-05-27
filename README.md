# TrayAgitator

A timer-controlled **tray agitator** driven by an ESP32-S3 and a stepper motor.
It runs the motor in an intermittent **2 s on / 8 s off** cycle for a set
duration, with adjustable agitation strength, a countdown timer, an OLED UI, and
buzzer feedback — handy for tasks like film/print development, PCB etching, or
staining where a tray needs gentle, periodic agitation.

## Features

- **Countdown timer** settable in 10-second, minute, and hour steps (up to 5 h).
- **Adjustable strength** (0–4) — controls the stepper's step rate.
- **Intermittent agitation**: the motor rotates ~2 s, then rests ~8 s, repeating
  for the whole run.
- **OLED UI** (SSD1306 128×64) showing mode, time, strength, and chip temperature.
- **DONE screen**: when the timer finishes it shows `DONE!` and counts up the
  time elapsed since it finished; press START to reset.
- **Cancel gesture**: hold MODE + START together to stop a running timer.
- **Persistent settings**: timer and strength are saved to NVS and restored on boot.
- **Boot diagnostics**: shows the last reset reason (and whether a run was active)
  on the OLED and serial, to help track down power/reboot issues. WiFi is disabled
  to reduce power spikes.

## Hardware

- **MCU:** Waveshare **ESP32-S3 Zero** (powered from USB-C, 5 V; 3V3 taken from its
  3V3 pad for the OLED and driver logic).
- **Stepper driver:** **A4988** (motor supply `VMOT` = 8–35 V, e.g. 12 V, with a
  100 µF cap across VMOT–GND).
- **Motor:** bipolar 4-wire NEMA-style stepper.
- **Display:** SSD1306 OLED, 128×64, I²C @ `0x3C`.
- **Buzzer:** passive buzzer.
- **Input:** 4 push buttons (`INPUT_PULLUP`, 50 ms debounce).

### Pin map

| GPIO | Function        | Connects to            |
|------|-----------------|------------------------|
| 3    | UP button       | button → GND           |
| 4    | DOWN button     | button → GND           |
| 5    | MODE button     | button → GND           |
| 6    | START button    | button → GND           |
| 7    | Buzzer          | passive buzzer + GND   |
| 8    | I²C SDA         | OLED SDA               |
| 9    | I²C SCL         | OLED SCL               |
| 10   | Stepper STEP    | A4988 STEP             |
| 11   | Stepper DIR     | A4988 DIR              |
| 12   | Stepper ENABLE  | A4988 EN (see note)    |

**Wiring notes**

- All grounds must be common: ESP32 GND, A4988 logic GND, motor-supply (−), OLED,
  buzzer, and button returns.
- The A4988 `EN` pin is active-LOW on the chip, but the firmware drives EN **HIGH
  to run** / LOW to idle — verify your physical EN wiring/logic matches.
- Tie A4988 `RESET` to `SLEEP`; set `MS1/MS2/MS3` for the desired microstep mode.
- The MODE button idles LOW while the others idle HIGH (per a firmware comment) —
  confirm that switch's normally-open/closed wiring.

See [Documents/](Documents/) for the full diagrams:
[wiring](Documents/TrayAgitator-wiring.svg),
[schematic](Documents/TrayAgitator-schematic.svg) (KiCad source
`TrayAgitator.kicad_sch`), and the `TrayAgitator Build Doc.docx`.

## Controls

- **MODE** cycles the setting screen: `Set 10 sec` → `Set Min` → `Set Hours` →
  `Strength`.
- **UP / DOWN** adjust the current setting (only while the timer is stopped):
  time by 10 s / 1 min / 1 h, or strength 0–4.
- **START**:
  - stopped → start the run (if a time is set)
  - running → pause
  - paused → resume
  - done → acknowledge and return to the setting screen
- **MODE + START (held together)** → cancel a running/paused timer.

## Build & flash

The firmware lives in [ESP32_Source/](ESP32_Source/) and builds with
[PlatformIO](https://platformio.org/).

```bash
cd ESP32_Source
pio run            # build
pio run -t upload  # flash over USB (serial/esptool — more reliable than JTAG)
pio device monitor # serial monitor @ 115200
```

- **Environment:** `esp32-s3-devkitc-1` (Arduino framework), used as the build
  target for the S3 board.
- **Key dependencies** (auto-installed by PlatformIO): Adafruit SSD1306,
  ezButton, AccelStepper. See [ESP32_Source/platformio.ini](ESP32_Source/platformio.ini).

> On macOS, prefer `pio run -t upload` (serial) over JTAG programming — the
> built-in USB-JTAG can be unreliable for flashing.

## Project structure

```
TrayAgitator/
├── ESP32_Source/        # PlatformIO firmware project
│   ├── src/main.cpp     # all firmware logic
│   └── platformio.ini   # board / build config
├── Documents/           # KiCad schematic, wiring/schematic SVGs, build doc
└── README.md
```

## Notes

- The current build **pauses at boot** on the reset-reason screen until a button
  is pressed — this is a debugging aid and can be removed for normal use.
- Some warmth on the chip-temperature readout is normal (it reads the silicon die
  temperature, above ambient).

# SmartCool — Comprehensive Troubleshooting Guide

This guide addresses every common failure mode encountered when building, programming, and demonstrating the **SmartCool Automatic Cooling System**.

---

## 1. I2C 16×2 LCD Screen Issues

### Symptom 1: Top row shows solid dark/white rectangles, bottom row is blank
- **Cause A: Contrast Potentiometer Needs Adjustment**
  - **Fix:** On the back of the I2C backpack module, locate the small blue potentiometer trimmer. Using a small Phillips or flathead screwdriver, gently turn the potentiometer until the letters become clearly visible against the backlight.
- **Cause B: Incorrect I2C Hex Address**
  - **Fix:** Most PCF8574 modules use address `0x27`, but many newer chips (PCF8574A) use `0x3F`.
  - In `SmartCool_Arduino.ino`, locate line:
    ```cpp
    LiquidCrystal_I2C lcd(0x27, 16, 2);
    ```
    Change `0x27` to `0x3F` and re-upload:
    ```cpp
    LiquidCrystal_I2C lcd(0x3F, 16, 2);
    ```
  - **Tip:** If uncertain, upload the quick I2C Scanner sketch provided below to print the exact hex address to your Serial Monitor.

### Symptom 2: Compile Error: `LiquidCrystal_I2C.h: No such file or directory`
- **Cause:** The library is not installed in the Arduino IDE.
- **Fix:**
  1. In the Arduino IDE, open **Tools** -> **Manage Libraries...** (or press `Ctrl + Shift + I`).
  2. In the search bar, type `LiquidCrystal_I2C`.
  3. Install the version authored by **Frank de Brabander**.
  4. Re-compile and upload.

---

## 2. LM35 Temperature Sensor Issues

### Symptom 1: LM35 gets scalding hot within 3 seconds of plugging in
- **CRITICAL DANGER:** The sensor is plugged in backwards!
- **Fix:** Disconnect USB power immediately! Check the sensor pinout:
  - Looking directly at the **flat printed face** with pins pointing DOWN:
    - Left = **5V**
    - Middle = **A0**
    - Right = **GND**

### Symptom 2: Readings jump or fluctuate rapidly (e.g. 27°C -> 34°C -> 25°C)
- **Cause A: Electrical Inductive Noise from the Fan**
  - When the fan spins, electromagnetic noise from the DC motor brushes couples into adjacent breadboard contacts.
  - **Fix:** Place a **100 nF (0.1 µF) ceramic capacitor** directly across the LM35's 5V and GND pins on the breadboard. Keep the LM35 signal wire physically separated from the fan motor wires.
- **Cause B: Floating Analog Pin**
  - **Fix:** Ensure the middle pin of the LM35 is firmly inserted into Arduino Pin **A0**.
- **Note:** The `SmartCool_Arduino.ino` firmware implements a 10-sample rolling arithmetic moving average filter specifically to eliminate noise jitter.

### Symptom 3: Temperature displays `0.0°C` or `ERR! SENS`
- **Cause:** Analog pin reads 0V or invalid voltage (< 2°C or > 110°C).
- **Fix:** Verify 5V power supply to sensor. Ensure breadboard power rails are bridged if you are using split buses.

---

## 3. Fan Driver & Transistor Circuit Issues

### Symptom 1: Fan does not turn ON when temperature passes the threshold
- **Check 1: Missing Common Ground (Most Common Mistake!)**
  - **Fix:** Ensure a jumper wire connects the **External 12V DC power supply Negative (-)** directly to an **Arduino GND pin**. Without this common reference, base current cannot flow.
- **Check 2: Transistor Pinout Orientation**
  - **Fix:** Verify the TIP122 pins. Facing the front plastic label (metal tab on back):
    - Pin 1 (Left) = Base -> connects to Arduino Pin 8 via 1kΩ resistor.
    - Pin 2 (Middle) = Collector -> connects to Fan Negative (-) wire.
    - Pin 3 (Right) = Emitter -> connects to GND.
- **Check 3: External 12V Supply**
  - Verify your 12V supply is plugged in and switched on. Measure voltage across the 12V rail using a multimeter.
- **Check 4: Arduino Indicator Pin 13**
  - The onboard LED on Arduino Pin 13 mirrors the fan output. If the Pin 13 LED turns ON but the fan doesn't spin, the software is working and the issue is strictly in the transistor wiring or 12V power supply.

### Symptom 2: Fan runs continuously and will not turn OFF
- **Cause A: Base Pin Floating or Shorted**
  - **Fix:** Install a **10 kΩ pull-down resistor** between Pin 1 (Base) of the TIP122 and GND.
- **Cause B: Blown Transistor**
  - If the circuit was operated without the 1N4007 flyback diode, inductive back-EMF may have shorted the collector-to-emitter junction of the TIP122. Replace the transistor and install the flyback diode.

### Symptom 3: Arduino resets or disconnects every time the fan kicks in
- **Cause:** Inductive inrush voltage dip or back-EMF spike resetting the microcontroller.
- **Fix:**
  - Verify that the fan is **NOT** powered from the Arduino 5V pin.
  - Verify that the **1N4007 flyback diode** is installed across the fan with its silver cathode band connected to +12V.
  - Add a **100 µF to 470 µF electrolytic capacitor** across the external 12V power supply rails to buffer current spikes.

---

## 4. Web Serial Dashboard & Communication Issues

### Symptom 1: Error: `Failed to open serial port` or `Access denied`
- **Primary Cause: COM Port Locked by Arduino IDE Serial Monitor**
  - A serial COM port in Windows can only be accessed by one application at a time.
  - **Fix:** Close the Serial Monitor window inside the Arduino IDE (or close any PuTTY / TeraTerm sessions) before clicking **"Connect Arduino"** in the web dashboard.

### Symptom 2: Button says "Web Serial API Not Supported"
- **Cause:** Web Serial is a Chromium standard.
- **Fix:** Web Serial is fully supported on **Google Chrome, Microsoft Edge, and Opera** (desktop). It is not supported in Mozilla Firefox, Apple Safari, or mobile browsers. Switch to Chrome or Edge.

### Symptom 3: Connected, but values remain `--.-` and no graph updates
- **Cause: Baud Rate Mismatch or Incompatible Serial Format**
  - **Fix:** Confirm both Arduino firmware and dashboard are set to **9600 Baud**.
  - Open the **Serial Communication Console** at the bottom left of `SmartCool_Dashboard.html`. You should see incoming strings such as:
    ```
    TEMP:28.5,FAN:0,TH:30.0,MODE:AUTO,STAT:NORMAL
    ```
  - If text is gibberish, reset the Arduino using the small red push-button on the Uno board.

---

## 5. Quick Diagnostic Tools

### Arduino I2C Scanner Sketch
If your 16×2 LCD address is unknown, upload this brief sketch to the Arduino Uno, open Serial Monitor at 9600 baud, and view your module's address:

```cpp
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial);
  Serial.println("\nI2C Scanner Active");
}

void loop() {
  byte error, address;
  int nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found\n");
  delay(3000);
}
```

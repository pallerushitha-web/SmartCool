/*
 ============================================================================
  SmartCool: Temperature-Based Smart Automatic Cooling System
  Firmware Version: 1.0.0
  Hardware Target: Arduino Uno (ATmega328P)
 ============================================================================
  
  SAFETY CRITICAL HARDWARE NOTICE:
  - NEVER connect the 12V DC Fan directly to an Arduino GPIO or 5V pin.
  - The fan MUST be driven via a power transistor (TIP122 NPN Darlington).
  - A flyback protection diode (1N4007 or 1N4001) MUST be placed reverse-biased
    across the fan terminals (Cathode/stripe to +12V, Anode to Collector).
  - External 12V DC power supply ground MUST be tied to Arduino GND (Common GND).
  - Do NOT power the 12V fan with a 5V adapter; use a dedicated 12V 1A DC supply.

  PIN ASSIGNMENTS:
  - LM35 Temperature Sensor Vout : Analog Pin A0
  - Fan Driver Base (TIP122)     : Digital Pin 8
  - I2C LCD SDA                  : Analog Pin A4 (or dedicated SDA pin)
  - I2C LCD SCL                  : Analog Pin A5 (or dedicated SCL pin)
  - Built-in Status LED          : Digital Pin 13

  REQUIRED LIBRARIES:
  - Wire.h (Standard Arduino library)
  - LiquidCrystal_I2C.h by Frank de Brabander
    Install via: Arduino IDE -> Tools -> Manage Libraries -> Search "LiquidCrystal_I2C"
    
  I2C ADDRESS NOTE:
  - Most PCF8574 I2C backpacks use 0x27 or 0x3F.
  - If your screen shows solid blocks or stays blank, verify contrast trimmer
    on the backpack and run an I2C scanner sketch if address differs.
 ============================================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// PIN DEFINITIONS & CONSTANTS
// ==========================================
const uint8_t PIN_LM35     = A0;  // LM35 analog signal input
const uint8_t PIN_FAN      = 8;   // TIP122 base control output
const uint8_t PIN_LED_IND  = 13;  // Onboard LED follows fan status

// Operating Modes
enum OperationMode {
  MODE_AUTO   = 0,
  MODE_MANUAL = 1
};

// ==========================================
// CONFIGURATION & CALIBRATION DEFAULTS
// ==========================================
// Default threshold in degrees Celsius
float thresholdTemp = 30.0;

// Hysteresis prevents rapid on/off switching (chattering) around threshold
// Fan turns ON  at (thresholdTemp)
// Fan turns OFF at (thresholdTemp - hysteresis)
const float HYSTERESIS = 1.0; 

// Sensor validation bounds for LM35 (ambient room limits)
const float MIN_VALID_TEMP = 2.0;    // Readings below 2°C in room ambient indicate disconnected sensor
const float MAX_VALID_TEMP = 110.0;  // Readings above 110°C indicate short-circuit / faulty sensor

// Telemetry & sampling intervals (non-blocking using millis)
const unsigned long TELEMETRY_INTERVAL_MS = 1000; // Send serial data every 1 second
const unsigned long SENSOR_SAMPLE_RATE_MS = 200;  // Sample sensor every 200ms

// ==========================================
// GLOBAL STATE VARIABLES
// ==========================================
OperationMode currentMode = MODE_AUTO;
bool fanState             = false; // true = ON, false = OFF
bool manualFanCommand     = false; // User's requested state in MANUAL mode
bool sensorError          = false;
float currentTempCelsius  = 0.0;

// Rolling average buffer for smoothing LM35 analog noise
const int NUM_SAMPLES = 10;
float sampleBuffer[NUM_SAMPLES];
int sampleIndex = 0;
bool bufferFilled = false;

// Timing counters
unsigned long lastTelemetryTime = 0;
unsigned long lastSampleTime    = 0;
unsigned long lastLcdUpdateTime = 0;

// Serial reception buffer
String inputBuffer = "";
bool stringComplete = false;

// ==========================================
// LCD DISPLAY INSTANCE
// Default address 0x27, 16 columns, 2 rows
// If text does not appear, change 0x27 to 0x3F
// ==========================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom degree symbol icon (5x8 matrix)
byte degreeChar[8] = {
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};

// Custom animated fan icon frame 1
byte fanIcon1[8] = {
  B00100,
  B10101,
  B01110,
  B11111,
  B01110,
  B10101,
  B00100,
  B00000
};

// Custom animated fan icon frame 2
byte fanIcon2[8] = {
  B01010,
  B00100,
  B11111,
  B00100,
  B11111,
  B00100,
  B01010,
  B00000
};

// ==========================================
// HELPER: READ & CONVERT LM35 SENSOR
// ==========================================
/*
  LM35 Precision Centigrade Temperature Sensor:
  - Sensitivity: 10 mV / °C
  - At 25°C, output is 250 mV (0.25V)
  - Arduino Uno default ADC reference: 5.0 Volts
  - 10-bit ADC resolution: 1024 steps (0 to 1023)
  - Voltage per ADC unit = 5.0V / 1024.0 = 0.0048828 V = 4.8828 mV
  - Temp in °C = (ADC_Value * 5.0 / 1024.0) / 0.010 = (ADC_Value * 500.0) / 1024.0
  
  To eliminate noise spikes from jumper leads and power fluctuations:
  We take multiple samples and compute an arithmetic moving average.
*/
float readLM35Temperature() {
  int rawADC = analogRead(PIN_LM35);
  
  // Convert ADC value to Celsius
  float voltage = (rawADC * 5.0) / 1024.0;
  float tempC = voltage * 100.0; // 10mV per degree C -> multiply by 100
  
  // Store into rolling filter
  sampleBuffer[sampleIndex] = tempC;
  sampleIndex++;
  if (sampleIndex >= NUM_SAMPLES) {
    sampleIndex = 0;
    bufferFilled = true;
  }
  
  // Calculate average
  int totalSamples = bufferFilled ? NUM_SAMPLES : sampleIndex;
  if (totalSamples == 0) return tempC;
  
  float sum = 0.0;
  for (int i = 0; i < totalSamples; i++) {
    sum += sampleBuffer[i];
  }
  return sum / totalSamples;
}

// ==========================================
// FAN HARDWARE DRIVER CONTROLLER
// ==========================================
void setFanHardwareState(bool enable) {
  fanState = enable;
  if (enable) {
    digitalWrite(PIN_FAN, HIGH);      // Saturate TIP122 base -> Fan spins
    digitalWrite(PIN_LED_IND, HIGH);  // Illuminate onboard indicator
  } else {
    digitalWrite(PIN_FAN, LOW);       // Cut off TIP122 base -> Fan stops
    digitalWrite(PIN_LED_IND, LOW);   // Extinguish onboard indicator
  }
}

// ==========================================
// CONTROL LOGIC: EVALUATE HYSTERESIS & MODE
// ==========================================
void evaluateCoolingLogic() {
  if (sensorError) {
    // Fail-safe protection: if sensor is disconnected or out of bounds,
    // turn OFF the fan to prevent unmanaged operation, or keep safe state.
    setFanHardwareState(false);
    return;
  }

  if (currentMode == MODE_MANUAL) {
    // In manual mode, strictly obey user's explicit manual command
    setFanHardwareState(manualFanCommand);
  } else {
    // In AUTOMATIC mode: Evaluate temperature against threshold with hysteresis
    if (!fanState) {
      // Fan is currently OFF: Trip ON when temperature meets or exceeds threshold
      if (currentTempCelsius >= thresholdTemp) {
        setFanHardwareState(true);
      }
    } else {
      // Fan is currently ON: Keep running until temp drops strictly below (threshold - hysteresis)
      if (currentTempCelsius < (thresholdTemp - HYSTERESIS)) {
        setFanHardwareState(false);
      }
    }
  }
}

// ==========================================
// UPDATE 16x2 I2C LCD SCREEN
// ==========================================
void updateLcdDisplay() {
  static bool fanAnimFrame = false;
  fanAnimFrame = !fanAnimFrame;

  // Row 0: Temperature & Fan State Icon
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  if (sensorError) {
    lcd.print("ERR! SENS");
  } else {
    if (currentTempCelsius < 10.0) lcd.print(" ");
    lcd.print(currentTempCelsius, 1);
    lcd.write(0); // Custom degree symbol
    lcd.print("C ");
  }

  // Fan running icon
  lcd.setCursor(14, 0);
  if (fanState) {
    lcd.write(fanAnimFrame ? 1 : 2);
    lcd.print(" ");
  } else {
    lcd.print("  ");
  }

  // Row 1: Threshold & Operating Mode
  lcd.setCursor(0, 1);
  lcd.print("Th: ");
  lcd.print(thresholdTemp, 1);
  lcd.write(0); // degree
  lcd.print("C ");

  lcd.setCursor(11, 1);
  if (sensorError) {
    lcd.print("[ERR]");
  } else if (currentMode == MODE_MANUAL) {
    lcd.print(fanState ? "[M:ON]" : "[M:OF]");
  } else {
    lcd.print(fanState ? "[AUTO]" : "[IDLE]");
  }
}

// ==========================================
// EMIT STRUCTURED TELEMETRY OVER USB SERIAL
// Protocol:
// TEMP:28.5,FAN:0,TH:30.0,MODE:AUTO,STAT:NORMAL
// ==========================================
void sendSerialTelemetry() {
  Serial.print("TEMP:");
  if (sensorError) {
    Serial.print("ERR");
  } else {
    Serial.print(currentTempCelsius, 1);
  }

  Serial.print(",FAN:");
  Serial.print(fanState ? "1" : "0");

  Serial.print(",TH:");
  Serial.print(thresholdTemp, 1);

  Serial.print(",MODE:");
  Serial.print(currentMode == MODE_AUTO ? "AUTO" : "MANUAL");

  Serial.print(",STAT:");
  if (sensorError) {
    Serial.print("ERROR");
  } else if (fanState) {
    Serial.print("COOLING");
  } else {
    Serial.print("NORMAL");
  }

  Serial.print("\n"); // Standard newline terminator
}

// ==========================================
// PROCESS COMMANDS FROM WEB DASHBOARD
// Supported incoming commands:
// - SET_TH:32.5     -> Update threshold temperature
// - SET_MODE:AUTO   -> Switch to automatic thermostat mode
// - SET_MODE:MANUAL -> Switch to manual fan control mode
// - SET_FAN:1       -> Turn fan ON (valid in manual mode)
// - SET_FAN:0       -> Turn fan OFF (valid in manual mode)
// - GET_STATUS      -> Immediately respond with current state
// ==========================================
void processIncomingCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // Echo acknowledgment / parse command
  if (cmd.startsWith("SET_TH:")) {
    float newTh = cmd.substring(7).toFloat();
    if (newTh >= 15.0 && newTh <= 60.0) {
      thresholdTemp = newTh;
      Serial.print("ACK:TH:");
      Serial.println(thresholdTemp, 1);
      updateLcdDisplay();
    } else {
      Serial.println("ERR:TH_OUT_OF_RANGE (15-60C)");
    }
  } 
  else if (cmd.startsWith("SET_MODE:")) {
    String modeStr = cmd.substring(9);
    modeStr.toUpperCase();
    if (modeStr == "AUTO") {
      currentMode = MODE_AUTO;
      Serial.println("ACK:MODE:AUTO");
      evaluateCoolingLogic();
      updateLcdDisplay();
    } else if (modeStr == "MANUAL") {
      currentMode = MODE_MANUAL;
      Serial.println("ACK:MODE:MANUAL");
      evaluateCoolingLogic();
      updateLcdDisplay();
    } else {
      Serial.println("ERR:UNKNOWN_MODE");
    }
  } 
  else if (cmd.startsWith("SET_FAN:")) {
    String fanStr = cmd.substring(8);
    int state = fanStr.toInt();
    if (currentMode == MODE_MANUAL) {
      manualFanCommand = (state == 1);
      setFanHardwareState(manualFanCommand);
      Serial.print("ACK:FAN:");
      Serial.println(manualFanCommand ? "1" : "0");
      updateLcdDisplay();
    } else {
      Serial.println("WARN:FAN_COMMAND_IGNORED (In AUTO mode. Switch to MANUAL first)");
    }
  } 
  else if (cmd == "GET_STATUS" || cmd == "STATUS") {
    sendSerialTelemetry();
  } 
  else {
    Serial.print("ERR:UNKNOWN_COMMAND:");
    Serial.println(cmd);
  }
}

// ==========================================
// ARDUINO SETUP
// ==========================================
void setup() {
  // Initialize Hardware Pins
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LED_IND, OUTPUT);
  pinMode(PIN_LM35, INPUT);

  // SAFE STARTUP STATE: Ensure fan is strictly OFF at power on
  digitalWrite(PIN_FAN, LOW);
  digitalWrite(PIN_LED_IND, LOW);

  // Initialize USB Serial Communication
  Serial.begin(9600);
  inputBuffer.reserve(64);

  // Initialize I2C LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  
  // Register custom characters
  lcd.createChar(0, degreeChar);
  lcd.createChar(1, fanIcon1);
  lcd.createChar(2, fanIcon2);

  // Splash Screen for Presentation / Boot verification
  lcd.setCursor(0, 0);
  lcd.print("SmartCool Sys v1");
  lcd.setCursor(0, 1);
  lcd.print("Calibrating LM35");
  
  // Warm up and prime the rolling buffer
  for (int i = 0; i < NUM_SAMPLES; i++) {
    readLM35Temperature();
    delay(40);
  }

  delay(600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  delay(500);
  lcd.clear();

  // Print startup greeting on Serial for Web Serial connection handshake
  Serial.println("SmartCool System Online (Baud: 9600)");
  sendSerialTelemetry();
}

// ==========================================
// MAIN ARDUINO LOOP (NON-BLOCKING)
// ==========================================
void loop() {
  unsigned long now = millis();

  // 1. Periodically sample temperature
  if (now - lastSampleTime >= SENSOR_SAMPLE_RATE_MS) {
    lastSampleTime = now;
    float temp = readLM35Temperature();

    // Check sensor sanity
    if (temp < MIN_VALID_TEMP || temp > MAX_VALID_TEMP) {
      sensorError = true;
    } else {
      sensorError = false;
      currentTempCelsius = temp;
    }

    // Evaluate cooling conditions
    evaluateCoolingLogic();
  }

  // 2. Periodically send telemetry to Web Dashboard
  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = now;
    sendSerialTelemetry();
  }

  // 3. Periodically refresh LCD screen
  if (now - lastLcdUpdateTime >= 400) {
    lastLcdUpdateTime = now;
    updateLcdDisplay();
  }

  // 4. Handle incoming Serial characters from Dashboard
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputBuffer.length() > 0) {
        processIncomingCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      // Append character if buffer limit not exceeded
      if (inputBuffer.length() < 60) {
        inputBuffer += inChar;
      }
    }
  }
}

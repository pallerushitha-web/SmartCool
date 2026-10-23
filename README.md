# SmartCool — Temperature-Based Smart Automatic Cooling System

![SmartCool Red & Black Theme](https://img.shields.io/badge/Theme-Red%20%26%20Black-FF303B?style=for-the-badge)
![Arduino Uno](https://img.shields.io/badge/Microcontroller-Arduino%20Uno-00979C?style=for-the-badge&logo=arduino)
![Web Serial API](https://img.shields.io/badge/Protocol-Web%20Serial%20API-4285F4?style=for-the-badge&logo=googlechrome)
![License: MIT](https://img.shields.io/badge/License-MIT-10B981?style=for-the-badge)

**SmartCool** is a complete, production-grade, and beginner-friendly temperature-controlled automatic cooling system designed for college capstone projects, engineering presentations, and industrial IoT demonstrations. 

The system reads ambient temperature in real time using an **LM35 precision analog sensor**, displays telemetry on an **I2C 16×2 LCD**, automatically actuates a high-power **12V DC cooling fan** using a **TIP122 Darlington transistor driver** with **1N4007 flyback diode protection**, and streams bidirectional telemetry directly to a modern **Red & Black cyber-dashboard** via the browser's native **Web Serial API** (no backend servers or Node.js required!).

---

## 🌟 Key Features

- **Precision Temperature Sensing:** Reads LM35 on Analog `A0` ($10\text{ mV}/^\circ\text{C}$) with a 10-sample rolling arithmetic moving average filter to eliminate noise jitter.
- **Solid-State Power Driver:** TIP122 NPN Darlington driver capable of switching up to 5A loads with a 1kΩ base resistor and 10kΩ pull-down for safe startup.
- **Inductive Back-EMF Protection:** 1N4007 flyback diode clamps motor reverse voltage spikes.
- **Hysteresis Anti-Chatter Logic:** Built-in 1.0°C thermal hysteresis band (Fan trips ON at $\ge \text{Threshold}$; turns OFF only below $\text{Threshold} - 1.0^\circ\text{C}$).
- **I2C 16×2 LCD Screen:** Real-time dual-row display with custom degree character (`°C`) and rotating animated fan glyph.
- **Futuristic Red & Black Dashboard (`SmartCool_Dashboard.html`):**
  - High-contrast matte charcoal (`#090A0D` / `#14161B`) and crimson accents (`#FF303B`).
  - Animated CSS spinning fan visualizer with pulse glow effects.
  - Interactive real-time temperature graph with threshold line (powered by Chart.js).
  - Bidirectional threshold adjustments (`SET_TH:xx.x`) and Manual/Automatic mode toggling (`SET_MODE`).
  - Live ASCII Serial Monitor console with command transmitter.
  - Synthesized futuristic audio alerts using the Web Audio API.
  - Full **Capstone Presentation Demo Sandbox** with thermal wave, cool down, and error injection simulators.

---

## 📁 Project File Structure

| File | Description |
| :--- | :--- |
| **[`SmartCool_Dashboard.html`](file:///d:/hackthon/SmartCool_Dashboard.html)** | Standalone, zero-dependency web dashboard with Web Serial & simulation lab. |
| **[`SmartCool_Arduino.ino`](file:///d:/hackthon/SmartCool_Arduino.ino)** | Complete Arduino Uno firmware with moving average, hysteresis, and LCD driver. |
| **[`WIRING_GUIDE.md`](file:///d:/hackthon/WIRING_GUIDE.md)** | Pinouts, circuit schematics, calculations, and safety instructions. |
| **[`TROUBLESHOOTING.md`](file:///d:/hackthon/TROUBLESHOOTING.md)** | Step-by-step diagnostic fixes for LCD, LM35, TIP122, and Web Serial. |
| **[`PRESENTATION_GUIDE.md`](file:///d:/hackthon/PRESENTATION_GUIDE.md)** | 5-minute capstone script, live demo walkthrough, and 15 Viva Voce Q&As. |
| **[`README.md`](file:///d:/hackthon/README.md)** | Master documentation and quick-start guide. |

---

## ⚡ Quick Start: 3 Steps to Run

### Step 1: Upload Arduino Firmware
1. Open the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the I2C LCD library:
   - Go to **Tools** -> **Manage Libraries...**
   - Search for `LiquidCrystal_I2C` by **Frank de Brabander** and click **Install**.
3. Open [`SmartCool_Arduino.ino`](file:///d:/hackthon/SmartCool_Arduino.ino).
4. Connect your Arduino Uno via USB, select **Tools** -> **Board** -> **Arduino Uno**, and choose your COM Port.
5. Click **Upload** ($\rightarrow$).

> [!NOTE]
> If your LCD shows solid white blocks or stays blank, refer to the [Troubleshooting Guide](file:///d:/hackthon/TROUBLESHOOTING.md) to check the contrast knob or change the I2C address between `0x27` and `0x3F`.

---

### Step 2: Assemble the Hardware
Follow the detailed step-by-step breadboard assembly in [`WIRING_GUIDE.md`](file:///d:/hackthon/WIRING_GUIDE.md).

```
   LM35 Sensor:       Vs -> 5V  |  Vout -> A0  |  GND -> GND
   I2C 16x2 LCD:      VCC -> 5V |  GND -> GND  |  SDA -> A4  |  SCL -> A5
   TIP122 Driver:     Base -> Pin 8 (via 1kΩ)  |  Collector -> Fan (-)  |  Emitter -> GND
   12V DC Fan:        Positive (+) -> 12V Supply (+)  |  Negative (-) -> Collector
   Flyback Diode:     Cathode (Band) -> +12V  |  Anode -> Collector
   Power Supply:      12V Negative (-) -> Arduino GND (COMMON GROUND)
```

> [!CAUTION]
> **Safety Warning:** NEVER connect the 12V fan directly to Arduino pins or to the 5V line. A separate regulated 12V DC power supply is required, and its negative lead must connect to Arduino GND.

---

### Step 3: Launch the Web Dashboard
You can launch the dashboard using either of two methods:

#### Method A: Direct File Open
Double-click [`SmartCool_Dashboard.html`](file:///d:/hackthon/SmartCool_Dashboard.html) in your file manager to open it in **Google Chrome** or **Microsoft Edge**.

#### Method B: Local HTTP Server (Recommended)
Since Python 3 is installed on your system, you can run a local server:
```powershell
# Open terminal in project directory
python -m http.server 8080
```
Then open your browser and navigate to:
**`http://localhost:8080/SmartCool_Dashboard.html`**

1. Ensure the **Arduino IDE Serial Monitor is CLOSED** (so the COM port is free).
2. Click **"Connect Arduino"** in the top navigation bar.
3. Select your Arduino Uno COM port in the browser popup and click **Connect**.
4. Real-time temperature and fan telemetry will start streaming immediately!

---

## 💻 Web Serial Bidirectional Protocol

The Arduino Uno and the web dashboard communicate over USB Serial at **9600 Baud** using structured ASCII strings:

### Microcontroller to Dashboard (Telemetry Streamed Every 1000ms):
```text
TEMP:28.5,FAN:0,TH:30.0,MODE:AUTO,STAT:NORMAL\n
```
- `TEMP:` Current filtered temperature in Celsius (`ERR` if sensor out of range).
- `FAN:` `1` for active cooling, `0` for idle.
- `TH:` Active trigger threshold in °C.
- `MODE:` `AUTO` or `MANUAL`.
- `STAT:` `NORMAL`, `COOLING`, or `ERROR`.

### Dashboard to Microcontroller (Commands):
| Command | Action | Microcontroller Response |
| :--- | :--- | :--- |
| `SET_TH:32.5` | Changes trigger threshold to 32.5°C | `ACK:TH:32.5` |
| `SET_MODE:AUTO` | Activates automatic thermostat logic | `ACK:MODE:AUTO` |
| `SET_MODE:MANUAL` | Activates manual fan override mode | `ACK:MODE:MANUAL` |
| `SET_FAN:1` | Forces fan ON (valid in manual mode) | `ACK:FAN:1` |
| `SET_FAN:0` | Forces fan OFF (valid in manual mode) | `ACK:FAN:0` |
| `GET_STATUS` | Requests immediate telemetry packet | Full telemetry packet |

---

## 🎯 Capstone Presentation Testing Sandbox

If you are demonstrating the project without physical hardware or in an auditorium where live heating is inconvenient:
1. Click **"Start Demo"** in the dashboard header.
2. A prominent **DEMO MODE ACTIVE** banner will illuminate.
3. In the **Capstone Presentation Testing Sandbox** card:
   - Click **"🔥 Simulate Heat Wave (36°C)"** to demonstrate fan cut-in and blade animation.
   - Click **"❄️ Simulate Cool Down (26°C)"** to demonstrate automatic 1.0°C hysteresis cut-out.
   - Click **"⚠️ Inject Sensor Error"** to demonstrate fail-safe shutdown.
   - Move the **Simulated Ambient Heat slider** to trigger precise trip points in front of the evaluators!

---

## 🛡️ Circuit Specifications & Calculations

- **Sensor Sensitivity:** $10\text{ mV}/^\circ\text{C}$ (LM35 Precision Analog)
- **ADC Resolution:** $\frac{5.0\text{V}}{1024} = 4.8828\text{ mV per step}$
- **Calculated Temperature:** $\text{Temp}(^\circ\text{C}) = \frac{\text{ADC} \times 500}{1024}$
- **Transistor Driver:** TIP122 NPN Darlington ($h_{FE} \ge 1000$, $I_C = 5\text{A}$)
- **Base Resistor:** $R_B = \frac{5.0\text{V} - 2.0\text{V}}{0.003\text{A}} \approx 1000\ \Omega\ (1\text{ k}\Omega)$
- **Diode Protection:** 1N4007 ($1000\text{V}, 1\text{A}$) reverse-biased across inductive fan coils

---

## 🤝 College Presentation & Viva Voce

Refer to [`PRESENTATION_GUIDE.md`](file:///d:/hackthon/PRESENTATION_GUIDE.md) for:
- 30-second elevator pitch
- 5-minute timed presentation script
- 15 comprehensive technical viva questions and model answers

---

## 📄 License
This project is open-source under the MIT License — free for academic, personal, and educational use.

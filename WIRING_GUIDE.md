# SmartCool — Complete Hardware Wiring & Circuit Guide

This guide provides step-by-step wiring instructions, electrical safety rules, and circuit explanations for the **SmartCool Temperature-Based Automatic Cooling System** using an Arduino Uno, LM35 temperature sensor, I2C 16×2 LCD, and a 12V DC cooling fan driven by a TIP122 Darlington transistor.

---

## 1. Safety Critical Rules (Read Before Powering On!)

> [!CAUTION]
> **1. NEVER connect the 12V fan directly to an Arduino GPIO or 5V pin:**
> An Arduino digital I/O pin can deliver a maximum of **40 mA** (absolute maximum; recommended 20 mA). A 12V DC cooling fan rated at 0.30A draws **300 mA** (and up to 600–800 mA inrush current on startup). Connecting the fan directly to an Arduino pin will permanently destroy the microcontroller.

> [!CAUTION]
> **2. DO NOT use the Arduino 5V regulator or a 5V adapter for the 12V fan:**
> A 12V fan requires a regulated 12V DC power supply (rated for at least 1A). Operating the fan on 5V may cause it to stall, overheat, or fail to achieve required airflow.

> [!IMPORTANT]
> **3. Common Ground is Mandatory:**
> You must connect the **negative (-) terminal of the external 12V power supply** directly to an **Arduino GND pin**. Without a common ground reference, the transistor base current loop cannot close, resulting in erratic switching, floating noise, or complete driver failure.

> [!IMPORTANT]
> **4. Flyback Diode (1N4007) is Mandatory:**
> The cooling fan motor is an inductive load. When the transistor switches OFF, collapsing magnetic fields induce an instantaneous reverse voltage spike (back-EMF) that can exceed 50–100V. The flyback diode safely clamps this spike, protecting the TIP122 transistor.

---

## 2. Component Pinout Reference

### A. TIP122 NPN Darlington Transistor
Looking at the **front (printed face)** with the metal heat-sink tab facing away and pins pointing down:
```
       +-------------+
       |   TIP122    |   <- Metal Tab at back (connected to Collector)
       |   [FRONT]   |
       +-------------+
          |   |   |
          1   2   3
          B   C   E
```
- **Pin 1 (Base - B):** Connects to Arduino **Pin D8** via a **1 kΩ resistor**.
- **Pin 2 (Collector - C):** Connects to the **Fan Negative (-) wire** and **Diode Anode**.
- **Pin 3 (Emitter - E):** Connects to **Common GND** (Arduino GND & 12V Supply (-)).

### B. LM35DZ Precision Temperature Sensor
Looking at the **flat (printed) face** with pins pointing downward:
```
          +-------+
         /  LM35   \   <- Flat printed face
        +-----------+
          |   |   |
          1   2   3
         Vs  Vout GND
```
- **Pin 1 (Left - Vs):** Connects to Arduino **5V**
- **Pin 2 (Middle - Vout):** Connects to Arduino **Analog Pin A0**
- **Pin 3 (Right - GND):** Connects to Arduino **GND**

> [!WARNING]
> If you connect the LM35 backwards (swapping Vs and GND), the sensor will rapidly become burning hot and may be destroyed. Always verify the flat face orientation before powering up!

### C. 16×2 LCD with I2C Backpack (PCF8574)
- **GND:** Connects to Arduino **GND**
- **VCC:** Connects to Arduino **5V**
- **SDA:** Connects to Arduino **A4** (or dedicated SDA header)
- **SCL:** Connects to Arduino **A5** (or dedicated SCL header)

### D. 1N4007 (or 1N4001) Flyback Diode
- **Cathode (end with the silver/white band):** Connects to **+12V Supply Rail** (Fan Positive wire).
- **Anode (solid black body without stripe):** Connects to **TIP122 Collector** (Fan Negative wire).

---

## 3. Comprehensive Wiring Interconnection Table

| Component | Component Pin | Arduino Uno Pin | External 12V Power Supply | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **LM35 Sensor** | Pin 1 (Vs) | `5V` | — | Regulated 5V from Arduino |
| | Pin 2 (Vout) | `A0` | — | Analog temperature voltage (10 mV/°C) |
| | Pin 3 (GND) | `GND` | — | Ground reference |
| **I2C 16×2 LCD** | VCC | `5V` | — | Powered from 5V rail |
| | GND | `GND` | — | Ground reference |
| | SDA | `A4` (SDA) | — | I2C Serial Data |
| | SCL | `A5` (SCL) | — | I2C Serial Clock |
| **TIP122 Driver** | Pin 1 (Base) | `Pin 8` via 1kΩ | — | Base drive current ~3mA for saturation |
| | Pin 1 (Base) | `GND` via 10kΩ | — | Pull-down resistor keeps fan OFF at boot |
| | Pin 2 (Collector) | — | — | Sinks fan current to ground when active |
| | Pin 3 (Emitter) | `GND` | `Negative (-)` | **COMMON GROUND CONNECTION** |
| **12V DC Fan** | Red Wire (+) | — | `+12V Positive (+)` | High voltage rail; DO NOT touch Arduino pins! |
| | Black Wire (-) | — | — | Connects to TIP122 Collector & Diode Anode |
| **1N4007 Diode** | Cathode (Band) | — | `+12V Positive (+)` | Connected reverse-biased across fan |
| | Anode (Black) | — | — | Connected to TIP122 Collector (Fan (-)) |

---

## 4. Complete Circuit Schematic Diagram

```
                       +12V DC Power Supply (+)
                              |
                              +------------------------+
                              |                        |
                              |                    [+] |
                         +---------+                  ---
                         |  1N4007 | (Cathode/Band)   | | 12V DC Fan
                         | Flyback |                 |M| (0.30A)
                         |  Diode  | (Anode)          | |
                         +---------+                  ---
                              |                    [-] |
                              +------------------------+
                              |
                       (Collector - Pin 2)
                            +-----+
                            |     |
    Arduino Pin 8 ---[ 1kΩ ]|  T  | TIP122
                            |  I  | Darlington
                            |  P  |
                   +--------|     |
                   |        +-----+
                [ 10kΩ ]       | (Emitter - Pin 3)
                   |           |
    Arduino GND ---+-----------+----------------------- External 12V Supply (-)
                                                      [COMMON GROUND]

   -------------------------------------------------------------------------
   LM35 Sensor Connections:
      LM35 Pin 1 (Vs)   -----> Arduino 5V
      LM35 Pin 2 (Vout) -----> Arduino Analog A0
      LM35 Pin 3 (GND)  -----> Arduino GND

   I2C 16x2 LCD Connections:
      LCD VCC           -----> Arduino 5V
      LCD GND           -----> Arduino GND
      LCD SDA           -----> Arduino A4
      LCD SCL           -----> Arduino A5
   -------------------------------------------------------------------------
```

---

## 5. Mathematical Calculations & Component Sizing

### A. Transistor Base Resistor Calculation
- Fan current $I_c = 300\text{ mA} = 0.30\text{ A}$
- TIP122 DC Current Gain $h_{FE} \approx 1000$ (minimum 250 in saturation)
- Base current required for full saturation:
  $$I_b = \frac{I_c}{100} = \frac{300\text{ mA}}{100} = 3\text{ mA}$$
- Arduino Uno digital output voltage: $V_{OH} = 5.0\text{ V}$
- TIP122 base-emitter saturation voltage $V_{BE(sat)} \approx 1.8\text{ V}$ to $2.0\text{ V}$
- Voltage drop across base resistor $R_B$:
  $$V_{RB} = 5.0\text{ V} - 2.0\text{ V} = 3.0\text{ V}$$
- Value of Base Resistor $R_B$:
  $$R_B = \frac{V_{RB}}{I_b} = \frac{3.0\text{ V}}{0.003\text{ A}} = 1000\ \Omega\ (1\text{ k}\Omega)$$
  *(Using a standard $1\text{ k}\Omega$ or $2.2\text{ k}\Omega$ 0.25W metal/carbon film resistor provides stable saturation).*

### B. TIP122 Power Dissipation & Thermal Management
- In saturation, $V_{CE(sat)} \approx 1.0\text{ V}$ to $1.2\text{ V}$
- Power dissipated inside the transistor:
  $$P_D = V_{CE(sat)} \times I_C \approx 1.2\text{ V} \times 0.30\text{ A} = 0.36\text{ Watts}$$
- Since $0.36\text{ W} < 2\text{ W}$ (the free-air limit of a TO-220 package), a small clip-on aluminum heatsink is recommended for continuous operation, though the transistor will remain acceptably warm during standard demonstrations.

---

## 6. Step-by-Step Breadboard Assembly

1. **Step 1 — Establish Power Rails on Breadboard:**
   - Reserve one long breadboard bus for Arduino 5V (Red wire) and Arduino GND (Blue/Black wire).
   - Use a separate isolated bus for the External 12V Supply (+) and (-).
   - **Bridge the Arduino GND to the 12V Supply (-) bus** with a jumper wire.

2. **Step 2 — Wire the LM35 Sensor:**
   - Place LM35 on the breadboard. Confirm flat face is towards you.
   - Left pin -> 5V bus.
   - Middle pin -> Jumper to Arduino Pin **A0**.
   - Right pin -> Arduino GND bus.

3. **Step 3 — Wire the I2C 16×2 LCD:**
   - Connect 4 female-to-male jumper wires from the LCD backpack.
   - VCC -> 5V bus.
   - GND -> GND bus.
   - SDA -> Arduino **A4**.
   - SCL -> Arduino **A5**.

4. **Step 4 — Build the TIP122 Transistor Driver:**
   - Insert TIP122 into 3 adjacent tie-point rows (e.g., rows 10, 11, 12).
   - Connect Pin 1 (Base) to Arduino **Pin 8** through the **1 kΩ resistor**.
   - Insert a **10 kΩ resistor** between Pin 1 (Base) and GND.
   - Connect Pin 3 (Emitter) directly to Common GND.

5. **Step 5 — Connect Fan & Flyback Diode:**
   - Connect the **1N4007 Diode** across two spare rows:
     - Cathode (silver band) connected to +12V rail.
     - Anode (black side) connected to Pin 2 (Collector) of TIP122.
   - Connect Fan Positive (+) wire to the +12V rail (same row as Diode Cathode).
   - Connect Fan Negative (-) wire to Pin 2 of TIP122 (same row as Diode Anode).

---

## 7. Safe Testing Without the 12V Fan (Laboratory Benchmark)

If you do not currently have the 12V power supply or fan ready, you can safely test the entire system:
1. Connect a standard **Red LED** in series with a **330 Ω resistor** across Arduino **Pin 8** and **GND** (or use the onboard Arduino LED on **Pin 13**, which is configured in firmware to mirror the fan state).
2. When temperature reaches or exceeds 30°C, the LED will illuminate, indicating that the switching signal is active!
3. All telemetry, web graphs, and serial data will function identically.

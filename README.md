# SafeSignal – Wireless Emergency Communication System

SafeSignal is a standalone, resilient prototype emergency communication system built on dual ESP32 microcontrollers communicating over **ESP-NOW**. It allows users in disaster zones or network-isolated environments to transmit physical-button status alerts wirelessly to a monitoring receiver station without requiring Wi-Fi routers, cellular networks, or internet infrastructure.

---

## 1. System Architecture & Pin Mapping

### A. Transmitter (Device 1)
* **Microcontroller:** ESP32-WROOM-32 (30-pin)
* **Input Buttons:** Configured as `INPUT_PULLUP` (Button connects GPIO pin to `GND`).

| Component | Pin / GPIO | Wiring Instructions |
| :--- | :--- | :--- |
| **SAFE Button** | `GPIO 25` | One leg to `GPIO 25`, other leg to `GND` |
| **NEED HELP Button** | `GPIO 26` | One leg to `GPIO 26`, other leg to `GND` |
| **MEDICAL EMERGENCY Button** | `GPIO 27` | One leg to `GPIO 27`, other leg to `GND` |
| **Transmission Feedback LED** | `GPIO 2` | Built-in Blue LED (blinks on transmission & ACK) |

```
    Transmitter Breadboard:
    [GPIO 25] ----[ Pushbutton (SAFE) ]-------- GND
    [GPIO 26] ----[ Pushbutton (NEED HELP) ]---- GND
    [GPIO 27] ----[ Pushbutton (MEDICAL) ]------ GND
```

---

### B. Receiver (Device 2)
* **Microcontroller:** ESP32-WROOM-32 (30-pin)
* **Indicators:** 3 LEDs (with 220Ω series current-limiting resistors) and 1 Active Buzzer Module.

| Component | Pin / GPIO | Wiring Instructions |
| :--- | :--- | :--- |
| **Green LED (SAFE)** | `GPIO 25` | `GPIO 25` -> `220Ω Resistor` -> Anode (+); Cathode (-) to `GND` |
| **Yellow LED (NEED HELP)** | `GPIO 26` | `GPIO 26` -> `220Ω Resistor` -> Anode (+); Cathode (-) to `GND` |
| **Red LED (MEDICAL)** | `GPIO 27` | `GPIO 27` -> `220Ω Resistor` -> Anode (+); Cathode (-) to `GND` |
| **Active Buzzer Signal** | `GPIO 33` | `GPIO 33` -> `I/O / SIG` pin of Active Buzzer Module |
| **Active Buzzer VCC** | `3V3` or `VIN` | Connect to ESP32 `3V3` (or `5V`/`VIN` if 5V module) |
| **Active Buzzer GND** | `GND` | Connect to ESP32 `GND` |

```
    Receiver Breadboard:
    [GPIO 25] ----[ 220Ω ]----(Anode [>|] Cathode)---- GND  (Green LED)
    [GPIO 26] ----[ 220Ω ]----(Anode [>|] Cathode)---- GND  (Yellow LED)
    [GPIO 27] ----[ 220Ω ]----(Anode [>|] Cathode)---- GND  (Red LED)
    [GPIO 33] ----------------[ SIG / I/O ]----------- Buzzer Module
    [ 3V3   ] ----------------[ VCC ]----------------- Buzzer Module
    [  GND  ] ----------------[ GND ]----------------- Buzzer Module
```

---

## 2. Status Output Matrix

| Received Status | Green LED (GPIO 25) | Yellow LED (GPIO 26) | Red LED (GPIO 27) | Buzzer (GPIO 33) |
| :--- | :---: | :---: | :---: | :---: |
| **SAFE** | **ON** | OFF | OFF | OFF (Silent) |
| **NEED HELP** | OFF | **ON** | OFF | OFF (Silent) |
| **MEDICAL EMERGENCY** | OFF | OFF | **ON** | **ACTIVE (Pulsing Alarm)** |

*Only the LED corresponding to the latest received message remains lit.*

---

## 3. Step-by-Step Setup Guide

### Step 1: Install ESP32 Board Support in Arduino IDE
1. Open **Arduino IDE**.
2. Go to `File` > `Preferences`.
3. In **Additional boards manager URLs**, add:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
4. Go to `Tools` > `Board` > `Boards Manager...`, search for `esp32` by **Espressif Systems**, and install it.

### Step 2: Obtain Receiver MAC Address (For Unicast & Phase 3 ACKs)
1. Open [`Get_MAC_Address/Get_MAC_Address.ino`](file:///c:/Users/Vinaya/OneDrive/Desktop/SafeSignal/Get_MAC_Address/Get_MAC_Address.ino).
2. Connect your **Receiver ESP32** via USB.
3. Select board `ESP32 Dev Module` (or `DOIT ESP32 DEVKIT V1`) and your COM port under `Tools`.
4. Upload the sketch and open **Serial Monitor** at **115200 baud**.
5. Copy the line formatted like:
   ```cpp
   uint8_t receiverAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
   ```
6. Open [`SafeSignal_Transmitter/SafeSignal_Transmitter.ino`](file:///c:/Users/Vinaya/OneDrive/Desktop/SafeSignal/SafeSignal_Transmitter/SafeSignal_Transmitter.ino) and paste your receiver's MAC address at line 40.
   *(Note: The code defaults to `{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}` broadcast, which works out of the box without changing the MAC, but unicast is recommended for link-layer ACK confirmation).*

### Step 3: Flash the Firmware
1. **Receiver:** Upload [`SafeSignal_Receiver/SafeSignal_Receiver.ino`](file:///c:/Users/Vinaya/OneDrive/Desktop/SafeSignal/SafeSignal_Receiver/SafeSignal_Receiver.ino) to Device 2.
   *On startup, the receiver executes a hardware self-test: flashing each LED sequentially and sounding a brief test chirp on the buzzer.*
2. **Transmitter:** Upload [`SafeSignal_Transmitter/SafeSignal_Transmitter.ino`](file:///c:/Users/Vinaya/OneDrive/Desktop/SafeSignal/SafeSignal_Transmitter/SafeSignal_Transmitter.ino) to Device 1.

---

## 4. Phase 1 – Phase 3 Verification

1. Power both devices via USB or battery packs.
2. Open Serial Monitors for both devices at 115200 baud.
3. **Press the SAFE button (GPIO 25):**
   * TX logs transmission attempt #1.
   * RX logs incoming packet with ID and payload.
   * Green LED turns ON; Yellow & Red turn OFF; Buzzer remains silent.
   * TX receives link ACK and double-blinks onboard LED (GPIO 2).
4. **Press the NEED HELP button (GPIO 26):**
   * Yellow LED turns ON; Green & Red turn OFF; Buzzer remains silent.
5. **Press the MEDICAL EMERGENCY button (GPIO 27):**
   * Red LED turns ON; Green & Yellow turn OFF; Buzzer emits an urgent pulsing emergency alarm.
6. **Retry & Fail-Safe Test (Phase 3):**
   * Unplug the Receiver board.
   * Press any button on the Transmitter.
   * Observe TX Serial Monitor: attempts 1, 2, and 3 fail to receive an ACK, and TX alerts `WARNING: Delivery FAILED after 3 retries`.

---

## 5. Roadmap: Future Expansion Phases

```
+-----------------------------------------------------------------------------------+
|                            SafeSignal Full Architecture                           |
+-----------------------------------------------------------------------------------+
 [Field Nodes: ESP-NOW / LoRa] ---> [Gateway Node: ESP32 + UART/WiFi]
                                              | (HTTP / REST API)
                                              v
                                    [FastAPI Backend + SQLite]
                                              | (WebSockets / SSE)
                                              v
                                [React Incident Dashboard]
                                 - Rule-Based Priority vs FCFS
```

* **Phase 4: LoRa Long-Range Extension:**
  Add SX1278 / SX1262 LoRa modules via SPI to scale transmission distance from ~150 meters (ESP-NOW) up to 5–10 km in challenging terrain.
* **Phase 5: FastAPI & SQLite Gateway Backend:**
  Receiver node bridges received packets over Serial/USB or Wi-Fi to a lightweight FastAPI server that logs emergencies into an SQLite audit table (`id`, `device_id`, `status`, `timestamp`, `acknowledged`).
* **Phase 6: React Dispatcher Dashboard:**
  Real-time web monitoring interface visualizing active incidents, color-coded urgency indicators, and an operator acknowledgment button.
* **Phase 7: Priority Algorithm Benchmarking (Rule-Based vs. FCFS):**
  * *First-Come-First-Served (FCFS):* Processes messages strictly by timestamp.
  * *Rule-Based Priority Queue:* Ranks `MEDICAL EMERGENCY` (Weight 3) > `NEED HELP` (Weight 2) > `SAFE` (Weight 1), measuring dispatch latency differences under message storm conditions.

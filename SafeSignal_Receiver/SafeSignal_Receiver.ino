/*
 * ======================================================================================
 * Project: SafeSignal – Wireless Emergency Communication System
 * Device:  Receiver (Device 2)
 * Hardware: ESP32-WROOM-32 (30-pin)
 * ======================================================================================
 * Pin Mapping:
 * - Green LED:       GPIO 25 (220Ω in series -> Anode; Cathode -> GND)
 * - Yellow LED:      GPIO 26 (220Ω in series -> Anode; Cathode -> GND)
 * - Red LED:         GPIO 27 (220Ω in series -> Anode; Cathode -> GND)
 * - Buzzer Signal:   GPIO 33 (Active Buzzer Module 3.3V compatible)
 * ======================================================================================
 * Expected Behavior:
 * - SAFE:              Green LED ON,  Yellow OFF, Red OFF, Buzzer OFF
 * - NEED HELP:         Yellow LED ON, Green OFF,  Red OFF, Buzzer OFF
 * - MEDICAL EMERGENCY: Red LED ON,    Green OFF,  Yellow OFF, Buzzer ACTIVE
 * 
 * Only the LED corresponding to the latest received message remains ON.
 * ======================================================================================
 */

#include <esp_now.h>
#include <WiFi.h>

// --------------------------------------------------------------------------------------
// 1. PIN DEFINITIONS
// --------------------------------------------------------------------------------------
#define PIN_LED_GREEN    25
#define PIN_LED_YELLOW   26
#define PIN_LED_RED      27
#define PIN_BUZZER       33

// Set to true for standard Active-HIGH buzzer modules (Module sounds when GPIO is HIGH)
// Set to false if you are using an Active-LOW buzzer module
#define BUZZER_ACTIVE_HIGH true

// Buzzer alert mode for Medical Emergency:
// Set true for intermittent pulsing alarm (200ms ON / 150ms OFF siren style)
// Set false for solid continuous tone
#define BUZZER_PULSE_MODE true

// --------------------------------------------------------------------------------------
// 2. MESSAGE DATA STRUCTURES
// --------------------------------------------------------------------------------------
enum EmergencyStatus : uint8_t {
  STATUS_NONE = 0,
  STATUS_SAFE = 1,
  STATUS_NEED_HELP = 2,
  STATUS_MEDICAL_EMERGENCY = 3
};

typedef struct struct_message {
  uint32_t messageId;
  EmergencyStatus status;
  char statusText[24];
  uint32_t timestamp;
} struct_message;

struct_message incomingMessage;

// --------------------------------------------------------------------------------------
// 3. SYSTEM STATE VARIABLES
// --------------------------------------------------------------------------------------
volatile EmergencyStatus currentStatus = STATUS_NONE;
volatile bool newStatusReceived = false;
volatile uint32_t lastMessageId = 0;

// Buzzer modulation timing
unsigned long lastBuzzerToggleTime = 0;
bool buzzerState = false;

// --------------------------------------------------------------------------------------
// 4. HARDWARE ACTUATION FUNCTIONS
// --------------------------------------------------------------------------------------
void setBuzzerHardware(bool active) {
  if (BUZZER_ACTIVE_HIGH) {
    digitalWrite(PIN_BUZZER, active ? HIGH : LOW);
  } else {
    digitalWrite(PIN_BUZZER, active ? LOW : HIGH);
  }
}

void applyStatusOutputs(EmergencyStatus status) {
  switch (status) {
    case STATUS_SAFE:
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_YELLOW, LOW);
      digitalWrite(PIN_LED_RED, LOW);
      setBuzzerHardware(false);
      buzzerState = false;
      Serial.println("[ACTION] Outputs: [GREEN: ON]  [YELLOW: OFF] [RED: OFF] [BUZZER: OFF]");
      break;

    case STATUS_NEED_HELP:
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_YELLOW, HIGH);
      digitalWrite(PIN_LED_RED, LOW);
      setBuzzerHardware(false);
      buzzerState = false;
      Serial.println("[ACTION] Outputs: [GREEN: OFF] [YELLOW: ON]  [RED: OFF] [BUZZER: OFF]");
      break;

    case STATUS_MEDICAL_EMERGENCY:
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_YELLOW, LOW);
      digitalWrite(PIN_LED_RED, HIGH);
      // Buzzer is driven in loop() for pulsing siren effect
      if (!BUZZER_PULSE_MODE) {
        setBuzzerHardware(true);
      }
      Serial.println("[ACTION] Outputs: [GREEN: OFF] [YELLOW: OFF] [RED: ON]  [BUZZER: ACTIVE]");
      break;

    case STATUS_NONE:
    default:
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_YELLOW, LOW);
      digitalWrite(PIN_LED_RED, LOW);
      setBuzzerHardware(false);
      buzzerState = false;
      break;
  }
}

// --------------------------------------------------------------------------------------
// 5. ESP-NOW RECEIVE CALLBACK (COMPATIBLE WITH ESP32 CORE 2.X AND 3.X)
// --------------------------------------------------------------------------------------
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  const uint8_t *mac_addr = recv_info->src_addr;
#else
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
#endif

  if (len != sizeof(struct_message)) {
    Serial.printf("[RX-WARN] Packet size mismatch. Expected %u bytes, got %d bytes\n", 
                  sizeof(struct_message), len);
    return;
  }

  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  currentStatus = incomingMessage.status;
  lastMessageId = incomingMessage.messageId;
  newStatusReceived = true;

  // Print reception diagnostics
  Serial.println("\n==================================================");
  Serial.printf("[RX] *** INCOMING EMERGENCY PACKET RECEIVED ***\n");
  Serial.printf("[RX] Sender MAC      : %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.printf("[RX] Message ID      : #%lu\n", (unsigned long)incomingMessage.messageId);
  Serial.printf("[RX] Status Payload  : %s (Code: %d)\n", incomingMessage.statusText, (int)incomingMessage.status);
  Serial.printf("[RX] TX Timestamp    : %lu ms\n", (unsigned long)incomingMessage.timestamp);
  Serial.println("==================================================");
}

// --------------------------------------------------------------------------------------
// 6. SETUP
// --------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println(" SafeSignal Receiver - Initializing             ");
  Serial.println("==================================================");

  // Configure output pins
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Initial self-test: flash each LED sequentially, chirp buzzer once
  Serial.println("[INIT] Running self-test on LEDs and Buzzer...");
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(150);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, HIGH);
  delay(150);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, HIGH);
  delay(150);
  digitalWrite(PIN_LED_RED, LOW);
  
  // Chirp buzzer
  setBuzzerHardware(true);
  delay(100);
  setBuzzerHardware(false);
  Serial.println("[INIT] Self-test complete. All outputs cleared.");

  // Ensure all outputs start OFF
  applyStatusOutputs(STATUS_NONE);

  // Set WiFi Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("[INIT] Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] Failed to initialize ESP-NOW! Rebooting in 3s...");
    delay(3000);
    ESP.restart();
  }

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("[INIT] ESP-NOW listening on all wireless channels.");
  Serial.println("[INIT] Waiting for incoming SafeSignal packets...");
  Serial.println("==================================================\n");
}

// --------------------------------------------------------------------------------------
// 7. MAIN LOOP
// --------------------------------------------------------------------------------------
void loop() {
  // 1. Process newly received status
  if (newStatusReceived) {
    newStatusReceived = false;
    applyStatusOutputs(currentStatus);
  }

  // 2. Continuous handling for Medical Emergency alarm (Buzzer pulsing)
  if (currentStatus == STATUS_MEDICAL_EMERGENCY) {
    if (BUZZER_PULSE_MODE) {
      unsigned long currentMillis = millis();
      // Fast alarm siren pattern: 200ms ON, 150ms OFF
      unsigned long interval = buzzerState ? 200 : 150;

      if (currentMillis - lastBuzzerToggleTime >= interval) {
        lastBuzzerToggleTime = currentMillis;
        buzzerState = !buzzerState;
        setBuzzerHardware(buzzerState);
      }
    }
  }

  // Short idle yield
  delay(5);
}

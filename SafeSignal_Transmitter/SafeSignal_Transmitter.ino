/*
 * ======================================================================================
 * Project: SafeSignal – Wireless Emergency Communication System
 * Device:  Transmitter (Device 1)
 * Hardware: ESP32-WROOM-32 (30-pin)
 * ======================================================================================
 * Pin Mapping:
 * - SAFE Button:              GPIO 25  (INPUT_PULLUP -> Button -> GND)
 * - NEED HELP Button:         GPIO 26  (INPUT_PULLUP -> Button -> GND)
 * - MEDICAL EMERGENCY Button: GPIO 27  (INPUT_PULLUP -> Button -> GND)
 * - Status LED (Onboard):     GPIO 2   (Blinks upon transmission / ACK)
 * ======================================================================================
 * Features:
 * - ESP-NOW direct wireless transmission (No router needed)
 * - Software debounce on all push buttons
 * - Phase 3 ACK feedback & automatic retry logic (up to 3 retries)
 * - Broadcast fallback or direct Unicast targeting Receiver MAC
 * ======================================================================================
 */

#include <esp_now.h>
#include <WiFi.h>

// --------------------------------------------------------------------------------------
// 1. PIN DEFINITIONS
// --------------------------------------------------------------------------------------
#define PIN_BTN_SAFE        25
#define PIN_BTN_HELP        26
#define PIN_BTN_MEDICAL     27
#define PIN_STATUS_LED      2   // Built-in blue LED on most ESP32 boards

// --------------------------------------------------------------------------------------
// 2. RECEIVER MAC ADDRESS CONFIGURATION
// --------------------------------------------------------------------------------------
// NOTE:
// To enable peer-to-peer ACK confirmation, run 'Get_MAC_Address.ino' on your Receiver ESP32,
// then paste the 6-byte MAC address below:
// Example: uint8_t receiverAddress[] = {0xC4, 0xDD, 0x57, 0x9E, 0x88, 0x14};
//
// By default, broadcast address {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} will transmit to any
// listening receiver without needing MAC pairing, but does not support MAC-level ACKs.
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// --------------------------------------------------------------------------------------
// 3. MESSAGE DATA STRUCTURES
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

struct_message txMessage;
esp_now_peer_info_t peerInfo;

// --------------------------------------------------------------------------------------
// 4. RETRY & TRANSMISSION STATE VARIABLES
// --------------------------------------------------------------------------------------
volatile bool ackReceived = false;
volatile bool transmissionDone = false;
volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;

uint32_t messageCounter = 0;
const int MAX_RETRIES = 3;
const unsigned long RETRY_INTERVAL_MS = 150;

// Button debouncing state
const unsigned long DEBOUNCE_DELAY = 50; // milliseconds

struct ButtonState {
  uint8_t pin;
  EmergencyStatus status;
  const char* label;
  int lastReading;
  int stableState;
  unsigned long lastDebounceTime;
};

ButtonState buttons[] = {
  {PIN_BTN_SAFE,    STATUS_SAFE,              "SAFE",              HIGH, HIGH, 0},
  {PIN_BTN_HELP,    STATUS_NEED_HELP,         "NEED HELP",         HIGH, HIGH, 0},
  {PIN_BTN_MEDICAL, STATUS_MEDICAL_EMERGENCY, "MEDICAL EMERGENCY", HIGH, HIGH, 0}
};
const int NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// --------------------------------------------------------------------------------------
// 5. ESP-NOW TRANSMIT CALLBACK
// --------------------------------------------------------------------------------------
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendStatus = status;
  transmissionDone = true;
  if (status == ESP_NOW_SEND_SUCCESS) {
    ackReceived = true;
  } else {
    ackReceived = false;
  }
}

// --------------------------------------------------------------------------------------
// 6. HELPER: SEND WITH RETRY (PHASE 3)
// --------------------------------------------------------------------------------------
bool sendWithRetry(EmergencyStatus status, const char* label) {
  messageCounter++;
  txMessage.messageId = messageCounter;
  txMessage.status = status;
  strncpy(txMessage.statusText, label, sizeof(txMessage.statusText) - 1);
  txMessage.statusText[sizeof(txMessage.statusText) - 1] = '\0';
  txMessage.timestamp = millis();

  bool isBroadcast = true;
  for (int i = 0; i < 6; i++) {
    if (receiverAddress[i] != 0xFF) {
      isBroadcast = false;
      break;
    }
  }

  Serial.println("\n--------------------------------------------------");
  Serial.printf("[TX] Initiating Transmission #%lu\n", (unsigned long)txMessage.messageId);
  Serial.printf("[TX] Status Payload : %s (Code %d)\n", txMessage.statusText, (int)txMessage.status);
  Serial.printf("[TX] Target Mode    : %s\n", isBroadcast ? "BROADCAST" : "UNICAST (Peer MAC)");

  bool sentSuccess = false;

  for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
    transmissionDone = false;
    ackReceived = false;

    // Visual pulse on onboard LED during attempt
    digitalWrite(PIN_STATUS_LED, HIGH);

    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&txMessage, sizeof(txMessage));
    
    if (result != ESP_OK) {
      Serial.printf("[TX] [Attempt %d/%d] Error queuing packet: %d\n", attempt, MAX_RETRIES, result);
    } else {
      // Wait for transmission callback (up to 100ms)
      unsigned long waitStart = millis();
      while (!transmissionDone && (millis() - waitStart < 100)) {
        delay(5);
      }

      if (isBroadcast) {
        // Broadcast does not return receiver ACK; success means packet transmitted over radio
        Serial.printf("[TX] [Attempt %d/%d] Broadcast transmitted onto air.\n", attempt, MAX_RETRIES);
        sentSuccess = true;
        digitalWrite(PIN_STATUS_LED, LOW);
        break;
      } else {
        if (lastSendStatus == ESP_NOW_SEND_SUCCESS) {
          Serial.printf("[TX] [Attempt %d/%d] ACK Confirmed by Receiver!\n", attempt, MAX_RETRIES);
          sentSuccess = true;
          digitalWrite(PIN_STATUS_LED, LOW);
          break;
        } else {
          Serial.printf("[TX] [Attempt %d/%d] No ACK from Receiver (Delivery Failed).\n", attempt, MAX_RETRIES);
        }
      }
    }

    digitalWrite(PIN_STATUS_LED, LOW);

    if (attempt < MAX_RETRIES) {
      Serial.printf("[TX] Retrying in %lu ms...\n", RETRY_INTERVAL_MS);
      delay(RETRY_INTERVAL_MS);
    }
  }

  // Result summary
  if (sentSuccess) {
    Serial.printf("[TX] >>> Message #%lu Delivered Successfully! <<<\n", (unsigned long)txMessage.messageId);
    // Double blink on success
    for (int b = 0; b < 2; b++) {
      digitalWrite(PIN_STATUS_LED, HIGH);
      delay(80);
      digitalWrite(PIN_STATUS_LED, LOW);
      delay(80);
    }
  } else {
    Serial.printf("[TX] [!] WARNING: Delivery FAILED after %d retries.\n", MAX_RETRIES);
    // Long flash on failure
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(500);
    digitalWrite(PIN_STATUS_LED, LOW);
  }
  Serial.println("--------------------------------------------------");

  return sentSuccess;
}

// --------------------------------------------------------------------------------------
// 7. SETUP
// --------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println(" SafeSignal Transmitter - Initializing           ");
  Serial.println("==================================================");

  // Initialize status LED
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  // Initialize buttons with internal pull-ups
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttons[i].lastReading = digitalRead(buttons[i].pin);
    buttons[i].stableState = buttons[i].lastReading;
    Serial.printf("[INIT] Button configured: GPIO %d -> %s\n", buttons[i].pin, buttons[i].label);
  }

  // Set device as Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("[INIT] Transmitter MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] Failed to initialize ESP-NOW! Rebooting in 3s...");
    delay(3000);
    ESP.restart();
  }

  // Register send callback to monitor transmission / ACK
  esp_now_register_send_cb(OnDataSent);

  // Register Peer (Receiver)
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;     // Pick current Wi-Fi channel
  peerInfo.encrypt = false; // Encryption off for basic prototype

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ERROR] Failed to add ESP-NOW peer!");
    return;
  }

  Serial.println("[INIT] ESP-NOW initialized successfully.");
  Serial.println("[INIT] Ready. Press any emergency button to transmit.");
  Serial.println("==================================================\n");
}

// --------------------------------------------------------------------------------------
// 8. MAIN LOOP
// --------------------------------------------------------------------------------------
void loop() {
  unsigned long currentMillis = millis();

  // Scan all 3 emergency buttons with debounce
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(buttons[i].pin);

    // If reading changed, reset debounce timer
    if (reading != buttons[i].lastReading) {
      buttons[i].lastDebounceTime = currentMillis;
    }

    if ((currentMillis - buttons[i].lastDebounceTime) > DEBOUNCE_DELAY) {
      // If reading has been steady for longer than debounce delay
      if (reading != buttons[i].stableState) {
        buttons[i].stableState = reading;

        // Button is active LOW (pressed down when connected to GND)
        if (buttons[i].stableState == LOW) {
          Serial.printf("\n[EVENT] %s button physically PRESSED on GPIO %d\n", 
                        buttons[i].label, buttons[i].pin);
          sendWithRetry(buttons[i].status, buttons[i].label);
        }
      }
    }

    buttons[i].lastReading = reading;
  }
}

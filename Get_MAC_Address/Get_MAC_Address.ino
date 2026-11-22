/*
 * ======================================================================================
 * SafeSignal - Wireless Emergency Communication System
 * Helper Tool: Get ESP32 MAC Address
 * ======================================================================================
 * Instructions:
 * 1. Upload this sketch to your RECEIVER ESP32 board.
 * 2. Open the Serial Monitor (Baud Rate: 115200).
 * 3. Press the EN / RST button on the ESP32.
 * 4. Copy the displayed MAC address and paste it into 'SafeSignal_Transmitter.ino'.
 * ======================================================================================
 */

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println("\n======================================");
  Serial.println("  SafeSignal - MAC Address Finder     ");
  Serial.println("======================================");
  Serial.print("Device MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("--------------------------------------");
  Serial.println("Format for Transmitter code:");
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("uint8_t receiverAddress[] = {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X};\n", 
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("======================================\n");
}

void loop() {
  // Nothing to do here
  delay(5000);
}

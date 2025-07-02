#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

// Ziel-Adresse (Peer) des Empfangs-ESP
uint8_t peerAddress[] = {0x74, 0x4D, 0xBD, 0xA0, 0xDA, 0xDC};
String serialBuffer;

// Callback nach Sendung
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status to ");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? " SUCCESS" : " FAILURE");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Station Mode (für ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // ESP-NOW initialisieren
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Send-Callback registrieren
  esp_now_register_send_cb(onSent);

  // Peer-Struktur konfigurieren
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0; // Standard-Kanal
  peerInfo.encrypt = false;

  // Peer hinzufügen
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW Sender ready.");
  Serial.println("Tippe eine Nachricht und drücke ENTER, um sie zu senden.");
}

void loop() {
  // Serielle Eingabe sammeln
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        // Nachricht senden
        const char *msg = serialBuffer.c_str();
        esp_err_t result = esp_now_send(peerAddress, (uint8_t *)msg, strlen(msg));
        if (result != ESP_OK) {
          Serial.println("ESP-NOW Send Error");
        }
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }
}

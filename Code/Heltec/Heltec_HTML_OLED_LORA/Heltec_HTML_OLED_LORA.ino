// --- Konfiguration OLED ---
#include "HT_SSD1306Wire.h"

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// --- Konfiguration WIFI ---
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "chat_HTML.h"

#define MAX_MESSAGES 10          // Maximale Anzahl Chat-Nachrichten
const byte DNS_PORT = 53;       // Port für den DNS-Server
const IPAddress apIP(192, 168, 4, 1);

DNSServer dnsServer;            // DNS-Server für Captive Portal
WebServer webServer(80);        // HTTP-Server auf Port 80

// --- Konfiguration MSGS ---
#include <vector>
#include <algorithm>

#include "mbedtls/base64.h"

std::vector<String> messages;   // Speicher für Chat-Nachrichten

// --- Konfiguration LORA ---
#include "LoRaWan_APP.h"

#define RF_FREQUENCY                                865000000 // Hz
#define TX_OUTPUT_POWER                             10        
            // bis 14 dBm erlaubt in EU, je höher desto stärker (Strom!)
#define LORA_BANDWIDTH                              1         
            // 0=hohe Reichweite aber lange Übertragungsdauer/ 2=Gegenteil
#define LORA_SPREADING_FACTOR                       10         
            // SF7=kürzeste Airtime, geringste Reichweite / 12=Gegenteil
#define LORA_CODINGRATE                             2         
            // 1=wenigste Redundanz aber hohe Nutzrate / 4=Gegenteil
#define LORA_PREAMBLE_LENGTH                        8         
            // Muss bei Tx und Rx gleich sein
#define LORA_SYMBOL_TIMEOUT                         0         
            // Wartezeit (0=unbeschränkt)
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
            // false=Paketlänge wird im Header mitübertragen
#define LORA_IQ_INVERSION_ON                        false
            // true=Invertiert die I/Q-Signalanteile (Kollisionen)

static RadioEvents_t RadioEvents;
const uint16_t BUF_SIZE = 256;
char ioBuffer[BUF_SIZE];      

// WEBSTUFF --------------------------------------------------------------
// Sendet die HTML-Seite an den Client
void handleRoot() {
  webServer.send_P(200, "text/html", htmlPage);
}

// Liefert die Nachrichten als JSON-Array
void handleMessages() {
  String json = "[";
  for (size_t i = 0; i < messages.size(); ++i) {
    json += "\"" + messages[i] + "\"";
    if (i + 1 < messages.size()) json += ",";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

// Empfängt neue Nachricht über HTTP POST und speichert sie
void handleSend() {
  if (webServer.hasArg("msg")) {
    String msg = webServer.arg("msg");
    msg.trim();
    if (msg.length()) {
      if (messages.size() >= MAX_MESSAGES) messages.erase(messages.begin());
      messages.push_back(msg);
      // Nachricht per LoRa verschicken
      sendLoRaMessage(msg);
      Serial.println(msg);
    }
  }
  webServer.send(200, "text/plain", "OK");
}
// WEBSTUFF --------------------------------------------------------------

// OLEDSTUFF --------------------------------------------------------------
// Display ON
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}
// Display OFF
void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}
// OLEDSTUFF --------------------------------------------------------------

// LORASTUFF --------------------------------------------------------------
void OnTxDone() {
    // Nach dem Senden direkt wieder in den Empfangsmodus gehen:
    Radio.Rx(0);
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    // Null-Terminierung
    if (size < BUF_SIZE) {
        payload[size] = '\0';
        Serial.print("Empfangen: ");
        String msg = String((char*)payload);
        if (messages.size() >= MAX_MESSAGES) messages.erase(messages.begin());
        messages.push_back(msg);
        Serial.println(msg);
    }
    // Gleich wieder reinhören
    Radio.Rx(0);
}

void sendLoRaMessage(const String &msg) {
    size_t len = msg.length();
    if (len > BUF_SIZE - 1) {
        len = BUF_SIZE - 1;
    }
    msg.toCharArray(ioBuffer, len + 1);
    Radio.Send((uint8_t*)ioBuffer, len);
}
// LORASTUFF --------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  messages.reserve(MAX_MESSAGES);  // Platz für Nachrichten vorreservieren

  // WEBSTUFF --------------------------------------------------------------
  WiFi.mode(WIFI_AP);  
  uint64_t efuseMac = ESP.getEfuseMac();
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {
    mac[i] = (efuseMac >> (8 * (5 - i))) & 0xFF;
  }
  char ssid[32];
  snprintf(ssid, sizeof(ssid),
           "ESPCHAT-%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);

  WiFi.softAP(ssid);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  dnsServer.start(DNS_PORT, "*", apIP);

  // --- Webserver-Routen registrieren ---
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.onNotFound(handleRoot);
  webServer.on("/send", HTTP_POST, handleSend);
  webServer.on("/messages", HTTP_GET, handleMessages);
  webServer.begin();
  // WEBSTUFF --------------------------------------------------------------

  // OLEDSTUFF --------------------------------------------------------------
  VextON();
  delay(100);

  display.init();
  display.clear();
  display.display();
  display.setContrast(255);

  // Basis-Einstellungen für Text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // Display the name of the Access point
  display.drawString(4, 4, "Access-Point:");
  display.drawStringMaxWidth(4, 20, 128, ssid);
  display.display();
  // OLEDSTUFF --------------------------------------------------------------
  
  // LORASTUFF --------------------------------------------------------------
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // Callbacks
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);

  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Radio.Rx(0);  
  // LORASTUFF --------------------------------------------------------------

}

void loop() {
  // WEBSTUFF --------------------------------------------------------------
  dnsServer.processNextRequest();  // DNS-Anfragen beantworten
  webServer.handleClient();        // HTTP-Requests bearbeiten
  // WEBSTUFF --------------------------------------------------------------

  // LORASTUFF --------------------------------------------------------------
  Radio.IrqProcess();
  // LORASTUFF --------------------------------------------------------------
}

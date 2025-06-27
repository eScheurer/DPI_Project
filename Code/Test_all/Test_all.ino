#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <pgmspace.h>
#include "html_page.h"

// --- OLED-Display-Konfiguration ---
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// --- WiFi & WebServer-Konfiguration ---
const char* ssid      = "ESP32-OpenAP";
const byte  DNS_PORT  = 53;
IPAddress   apIP(192,168,4,1);
DNSServer           dnsServer;
WebServer           webServer(80);

// Chat- und Sendewarteschlange
#define MAX_MESSAGES 50
std::vector<String> messages;           // Chat-Historie
std::vector<String> txQueue;            // Nachrichten, die per LoRa gesendet werden

// --- LoRa-Konfiguration ---
#define RF_FREQUENCY               915000000
#define TX_OUTPUT_POWER            5
#define LORA_BANDWIDTH             0
#define LORA_SPREADING_FACTOR      7
#define LORA_CODINGRATE            1
#define LORA_PREAMBLE_LENGTH       8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false
#define BUFFER_SIZE                30

char txpacket[BUFFER_SIZE];
bool lora_idle = true;
static RadioEvents_t RadioEvents;

// Forward-Deklarationen
void OnTxDone( void );
void OnTxTimeout( void );

// liefert die HTML-Seite aus PROGMEM
void handleRoot() {
  webServer.send_P(200, "text/html", htmlPage);
}

// JSON-Endpoint: alle gespeicherten Nachrichten
void handleMessages() {
  String json = "[";
  for (size_t i = 0; i < messages.size(); ++i) {
    json += "\"" + messages[i] + "\"";
    if (i + 1 < messages.size()) json += ",";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

// POST-Endpoint: fügt eine neue Nachricht hinzu & in LoRa-Queue
void handleSend() {
  if (webServer.hasArg("msg")) {
    String msg = webServer.arg("msg");
    msg.trim();
    if (msg.length() > 0) {
      // Chat-Historie aktualisieren
      messages.push_back(msg);
      if (messages.size() > MAX_MESSAGES) {
        messages.erase(messages.begin());
      }
      // Nachricht zur LoRa-Warteschlange hinzufügen
      txQueue.push_back(msg);
    }
  }
  webServer.send(200, "text/plain", "OK");
}

void setup() {
  // Serielle Konsole
  Serial.begin(115200);
  delay(1000);

  // 1) OLED initialisieren
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(100);
  display.init();
  display.clear();
  display.display();
  display.setContrast(255);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);

  // 2) LoRa initialisieren
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(
    MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
    true, 0, 0, LORA_IQ_INVERSION_ON, 3000
  );

  // 3) WiFi-AP und DNS-Captive-Portal
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  Serial.printf("AP gestartet: SSID=%s, IP=%s\n",
                ssid, WiFi.softAPIP().toString().c_str());
  dnsServer.start(DNS_PORT, "*", apIP);

  // 4) Web-Server-Routen
  webServer.on("/",            HTTP_GET,  handleRoot);
  webServer.onNotFound(                   handleRoot);
  webServer.on("/messages",  HTTP_GET,  handleMessages);
  webServer.on("/send",      HTTP_POST, handleSend);
  webServer.begin();
  Serial.println("Webserver und DNS-Server laufen, Chat bereit");
}

void loop() {
  // 1) LoRa-Interrupts abarbeiten
  Radio.IrqProcess();

  // 2) Web-Server und DNS-Server abfragen
  dnsServer.processNextRequest();
  webServer.handleClient();

  // 3) Wenn LoRa frei und Nachrichten in der Queue, sende nächste
  if (lora_idle && !txQueue.empty()) {
    String nextMsg = txQueue.front();
    txQueue.erase(txQueue.begin());

    // Puffer füllen (bis BUFFER_SIZE-1)
    nextMsg = nextMsg.substring(0, BUFFER_SIZE - 1);
    nextMsg.toCharArray(txpacket, BUFFER_SIZE);

    // Anzeige aktualisieren
    display.clear();
    display.drawString(64, 32, nextMsg);
    display.display();

    // Senden
    Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    lora_idle = false;
  }
}

// Callback, wenn Senden erfolgreich war
void OnTxDone(void) {
  lora_idle = true;
}

//hallo luis

// Callback, wenn Senden abgelaufen ist
void OnTxTimeout(void) {
  Radio.Sleep();
  display.clear();
  display.drawString(64, 32, "TX timeout");
  display.display();
  delay(500);
  lora_idle = true;
}

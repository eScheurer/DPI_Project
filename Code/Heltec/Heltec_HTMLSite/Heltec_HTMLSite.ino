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

String serialBuffer;            // Puffer für serielle Eingaben

// --- Konfiguration MSGS ---
#include <vector>

std::vector<String> messages;   // Speicher für Chat-Nachrichten

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
      Serial.println(msg);
    }
  }
  webServer.send(200, "text/plain", "OK");
}
// WEBSTUFF --------------------------------------------------------------

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
}

void loop() {
  // WEBSTUFF --------------------------------------------------------------
  dnsServer.processNextRequest();  // DNS-Anfragen beantworten
  webServer.handleClient();        // HTTP-Requests bearbeiten
  // WEBSTUFF --------------------------------------------------------------
}

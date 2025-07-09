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

// --- Konfiguration CRDT ---
#include <vector>
#include <algorithm>

#include "mbedtls/base64.h"

unsigned long lastSync = 0;
const unsigned long syncInterval = 10000; // every 10 sec

// std::vector<String> messages;   // Speicher für Chat-Nachrichten

struct CRDTNode {
  String id;
  String content;
  uint32_t time;
  String sender;
};

std::vector<CRDTNode> crdtList;
uint32_t localClock = 0;
String nodeID;
char macAddress[32];

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
  for (size_t i = 0; i < crdtList.size(); ++i) {
    json += "\"" + crdtList[i].content + "\"";
    if (i + 1 < crdtList.size()) json += ",";
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
      sendMessage(msg);
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
        String msg = String((char*)payload);
        Serial.println("Empfangen: " + msg);
        onReceiveMessage(msg);
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
    Serial.println("Sent: " + msg);
}
// LORASTUFF --------------------------------------------------------------

// CRDTSTUFF --------------------------------------------------------------
/**
/ Checking if a message already exists based on the unique id
*/
bool messageExists(const String& id) {
  for (auto &msg : crdtList) {
    if (msg.id == id) return true;
  }
  return false;
}

void sortCRDTLog() {
  std::sort(crdtList.begin(), crdtList.end(), [](const CRDTNode& a, const CRDTNode& b) {
    if (a.time != b.time) { 
      return a.time < b.time;
    }
    return a.sender < b.sender;
  });
}

/**
/ Apply a message if it doens't exist yet
*/
void applyMessage(const CRDTNode& newMsg) {
  if (!messageExists(newMsg.id)) {
    localClock = max(localClock, newMsg.time) + 1;
    crdtList.push_back(newMsg);
    sortCRDTLog();
    // TODO test if the sorting is needed/usefull in our case
  }
}

// The message ID is based of the curent local time and the WiFi MAC address
String createMessageID(uint32_t time) {
  return String(time) + "_" + nodeID;
}

/**
/ Sending the id and message over LoRa
/ This is used if some messages are missing on the remote ESP
*/
void sendMessage(const String& content) {
  localClock++;
  uint32_t time = localClock;
  String id = createMessageID(time);
  String packet = "MSG|" + id + "|" + content + "|" + String(time) + "|" + nodeID;
  sendLoRaMessage(packet);
  applyMessage({id, content, time, nodeID});
}

/**
/ Sending a list with or current IDs over LoRa
/ This signals a remote ESP if it has missing messages
*/
void sendIDList() {
  String idList = "IDLIST|";
  // Appending the message IDs to be sent
  for (auto &msg : crdtList) {
    idList += msg.id + ",";
  }
  sendLoRaMessage(idList);
}

/**
/ Received ID list need to be converted to the same format as our own list
*/
void handleIDList(String list) {
  list.remove(0, 7);  // Remove "IDLIST|"
  std::vector<String> remoteIDs;
  int last = 0;
  // Getting index of the first comma
  int comma = list.indexOf(',', last);
  while (comma != -1) {
    // While we are not at the end of the string -> Continue appending
    remoteIDs.push_back(list.substring(last, comma));
    // The next comma needs to be found after the last index+1
    last = comma + 1;
    comma = list.indexOf(',', last);
  }

    // Für jeden lokalen CRDT-Eintrag prüfen, ob er im remote fehlt
  for (auto &msg : crdtList) {
    bool found = false;
    for (auto &rid : remoteIDs) {
      if (msg.id == rid) {
        found = true;
        break;
      }
    }
    // Wenn nicht gefunden, als Einzel-Paket nachsenden
    if (!found) {
      String packet = "MSG|"
        + msg.id + "|" 
        + msg.content + "|" 
        + String(msg.time) + "|" 
        + msg.sender;
      sendLoRaMessage(packet);
      delay(200);  // kurze Pause, damit LoRa nicht überhitzt
    }
  }
}

/**
/ Received message needs to be decoded
*/
void onReceiveMessage(String &msg) { 
  String received = msg;

  // Handle single messages
  if (received.startsWith("MSG|")) {
  int p1 = received.indexOf('|', 4);
  int p2 = received.indexOf('|', p1 + 1);
  int p3 = received.indexOf('|', p2 + 1);
  // now we only need three separators
  if (p1 == -1 || p2 == -1 || p3 == -1) return;

  String id      = received.substring(4,    p1);
  String content = received.substring(p1+1, p2);
  uint32_t time  = received.substring(p2+1, p3).toInt();
  String sender  = received.substring(p3+1);      // rest of string

  applyMessage({ id, content, time, sender });

  // Handle receiving of ID lists
  } else if (received.startsWith("IDLIST|")) {
    handleIDList(received);

  } 
  // Differentiate between a single message (MSG|) and the remote list (IDLIST|)
  // For single messages apply the message with applyMessage
  // For the remote list pass it on to handleIDList
}

// CRDTSTUFF --------------------------------------------------------------


void setup() {
  Serial.begin(115200);

  // WEBSTUFF --------------------------------------------------------------
  WiFi.mode(WIFI_AP);  
  uint64_t efuseMac = ESP.getEfuseMac();
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {
    mac[i] = (efuseMac >> (8 * (5 - i))) & 0xFF;
  }
  char ssid[32];
  snprintf(macAddress, sizeof(macAddress),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
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

  // CRDTSTUFF --------------------------------------------------------------
  // crdtList.reserve(MAX_MESSAGES);  // Platz für Nachrichten vorreservieren
  nodeID = macAddress;
  // CRDTSTUFF --------------------------------------------------------------
}

void loop() {
  // WEBSTUFF --------------------------------------------------------------
  dnsServer.processNextRequest();  // DNS-Anfragen beantworten
  webServer.handleClient();        // HTTP-Requests bearbeiten
  // WEBSTUFF --------------------------------------------------------------

  // LORASTUFF --------------------------------------------------------------
  Radio.IrqProcess();
  // LORASTUFF --------------------------------------------------------------

  // CRDTSTUFF --------------------------------------------------------------
  // Sending ID list every syncInterval seconds
  if (millis() - lastSync > syncInterval) {
    sendIDList();
    lastSync = millis();
  }
  // CRDTSTUFF --------------------------------------------------------------

}

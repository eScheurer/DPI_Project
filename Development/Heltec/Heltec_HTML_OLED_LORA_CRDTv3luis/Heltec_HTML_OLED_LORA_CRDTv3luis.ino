// Configuration ---------------------------------------------------------
// OLED
#include "HT_SSD1306Wire.h"
#include "mbedtls/aes.h" // encription 
#include "mbedtls/base64.h"
#include "esp_system.h" // for esp_random()

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// WIFI
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "chat_HTML.h"

const byte DNS_PORT = 53;       // Port für den DNS-Server
const IPAddress apIP(192, 168, 4, 1);

DNSServer dnsServer;            // DNS-Server für Captive Portal
WebServer webServer(80);        // HTTP-Server auf Port 80
char ssid[32];

// CRDT 
#include <vector>
#include <algorithm>
#include <map>

unsigned long lastSync = 0;
const unsigned long syncInterval = 10000; // every 10 sec
uint64_t parseMac(const String &s);

struct CRDTNode {
  String id;
  String content;
  uint32_t time;
  String sender;
};

struct IDElement {
  int id;
  int numberOfMessages;
};

// Comparator für numerische MAC‑Adresse
struct MacLess {
  bool operator()(const String &a, const String &b) const {
    return parseMac(a) < parseMac(b);
  }
};

std::vector<CRDTNode> crdtList;
std::map<String, IDElement, MacLess> idlist;
uint32_t localClock = 0;
String nodeID;
char macAddress[32];

// LORA
#include "LoRaWan_APP.h"

#define RF_FREQUENCY                                865000000 // Hz
#define TX_OUTPUT_POWER                             10        
            // Higher power increases range but reduces battery life
#define LORA_BANDWIDTH                              1         
            // 0 = long range but long transmission time / 2 = short range but fast transmission
#define LORA_SPREADING_FACTOR                       10         
            // SF7 = shortest airtime, lowest range / SF12 = longest airtime, highest range
#define LORA_CODINGRATE                             2         
            // 1 = least redundancy but highest data rate / 4 = most redundancy but lowest data rate
#define LORA_PREAMBLE_LENGTH                        8         
            // Must be the same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         
            // Timeout (0 = unlimited)
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
            // false = payload length is sent in the header
#define LORA_IQ_INVERSION_ON                        false
            // true = inverts the I/Q signal components (helps avoid collisions)

static RadioEvents_t RadioEvents;
const uint16_t BUF_SIZE = 256;
char ioBuffer[BUF_SIZE];      
// Configuration ---------------------------------------------------------

// SECURITY --------------------------------------------------------------
// key for testing
const uint8_t aesKey[16] = {
  0x12, 0x34, 0x56, 0x78,
  0x9A, 0xBC, 0xDE, 0xF0,
  0x11, 0x22, 0x33, 0x44,
  0x55, 0x66, 0x77, 0x88
};

// const uint8_t aesKey[16] = {0x50, 0xd8, 0x58, 0xe0,
                            // 0x60, 0x41, 0x8a, 0xaf, 
                            // 0x7f, 0x42, 0xc2, 0x57, 
                            // 0xa9, 0xe8, 0xcc, 0xac, };

String encrypt(String rawText){
  mbedtls_aes_context aes;
  uint8_t iv[16]; // Initialisierungsvektor
  size_t length = rawText.length();
  size_t paddedLength = ((length/16) +1) *16; // aes works with 16bit blocks, need padding to fit it
  uint8_t input[paddedLength];
  uint8_t output[paddedLength];
  memset(input, 0, paddedLength); // "Zero-Padding"
  memcpy(input, rawText.c_str(), length);

  esp_fill_random(iv, 16);
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aesKey, 128); //128 Bit == 16 Byte
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLength, iv, input, output);

  uint8_t encoded[16 + paddedLength];
  memcpy(encoded, iv, 16);
  memcpy(encoded + 16, output, paddedLength);

  size_t outputLength;
  char base64[512]; //TODO: size?????????????????????????????????
  mbedtls_base64_encode((unsigned char*)base64, sizeof(base64), &outputLength, encoded, 16+paddedLength); // for transmission
  base64[outputLength] = '\0'; // make it correct String

  mbedtls_aes_free(&aes);
  
  return String((char*)base64);
}

String decrypt(String encriptedText) {
  mbedtls_aes_context aes;
  uint8_t decoded[512];
  size_t decodedLength;
  int result = mbedtls_base64_decode(decoded, sizeof(decoded), &decodedLength,(const uint8_t*)encriptedText.c_str(), encriptedText.length());

  if (result != 0) {
    return "Error: Decoding went wrong!";
  }
  if (decodedLength < 16) {
    return "Error: message too short, likely damaged";
  }
  uint8_t iv[16];
  memcpy(iv, decoded, 16);
  size_t rawLength = decodedLength -16;
  uint8_t* rawText = decoded + 16;

  //uint8_t decrypted[rawLength];
  uint8_t* decrypted = (uint8_t*)malloc(rawLength);
  if (!decrypted){
    return "Error: malloc failed";
  } 

  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, aesKey, 128);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, rawLength, iv, rawText, decrypted);
  mbedtls_aes_free(&aes);

  if (result != 0) {
    free(decrypted);
    return "Error: decryption failed";
  }

  while (decrypted[rawLength -1] == 0 && rawLength > 0) {
    rawLength -= 1; // remove the zeros padding from the end
  }
  String output = String((char*)decrypted, rawLength);
  free(decrypted);
  return output;
}


// WEBSTUFF --------------------------------------------------------------
// Sends HTML to client
void handleRoot() {
  webServer.send_P(200, "text/html", htmlPage);
}

// Gives msg as json array
void handleMessages() {
  String json = "[";
  for (size_t i = 0; i < crdtList.size(); ++i) {
    json += "\"" + crdtList[i].content + "\"";
    if (i + 1 < crdtList.size()) json += ",";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

// Receives msg over POST
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
    Radio.Rx(0);
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    // Null-Terminierung
    if (size < BUF_SIZE) {
        payload[size] = '\0';
        String msg = String((char*)payload);
        Serial.println("Empfangen: " + msg);
        String decrypted = decrypt(msg);
        Serial.println("Decrypted: " + decrypted);
        onReceiveMessage(decrypted);
    }
    Radio.Rx(0);
}

void sendLoRaMessage(const String &msg) {
    Serial.println("Raw: " + msg);
    String encrypted = encrypt(msg);
    size_t len = encrypted.length();
    if (len > BUF_SIZE - 1) {
        len = BUF_SIZE - 1;
    }
    encrypted.toCharArray(ioBuffer, len + 1);
    Radio.Send((uint8_t*)ioBuffer, len);
    Serial.println("Sent: " + encrypted);
}
// LORASTUFF --------------------------------------------------------------

// CRDTSTUFF --------------------------------------------------------------
// Parses MAC‑Strings "AA:BB:CC" into 48‑Bit‑Number for storting
uint64_t parseMac(const String &s) {
  uint64_t v = 0;
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == ':') continue;
    uint8_t nibble;
    if      (c >= '0' && c <= '9') nibble = c - '0';
    else if (c >= 'A' && c <= 'F') nibble = 10 + (c - 'A');
    else if (c >= 'a' && c <= 'f') nibble = 10 + (c - 'a');
    else continue; // ungültiges Zeichen überspringen
    v = (v << 4) | nibble;
  }
  return v;
}

// Update or create ID entry
void updateIdEntry(const String& mac, int id) {
  auto it = idlist.find(mac);
  if (it != idlist.end()) {
    it->second.id = id;
    it->second.numberOfMessages++;
  } else {
    idlist[mac] = { id, 1 };
  }
}

// Checking if a message already exists based on the unique id
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

// Apply a message if it doens't exist yet
void applyMessage(const CRDTNode& newMsg) {
  if (!messageExists(newMsg.id)) {
    localClock = max(localClock, newMsg.time) + 1;
    crdtList.push_back(newMsg);
    sortCRDTLog();
    // TODO test if the sorting is needed/usefull in our case
    // Update ID tracking
    updateIdEntry(newMsg.sender, (int) newMsg.time);
  }
}

// The message ID is based of the curent local time and the WiFi MAC address
String createMessageID(uint32_t time) {
  return String(time) + "_" + nodeID;
}

// Sending the id and message over LoRa. This is used if some messages are missing on the remote ESP
void sendMessage(const String& content) {
  localClock++;
  uint32_t time = localClock;
  String id = createMessageID(time);
  String packet = "MSG|" + id + "|" + content; 
  sendLoRaMessage(packet);
  applyMessage({ id, content, time, nodeID });
}

// Sending a list with or current IDs over LoRa. This signals a remote ESP if it has missing messages
void sendIDList() {
  // Format: "IDLIST|id_mac_count|..."
  String out = "IDLIST|";
  for (auto &kv : idlist) {
    const String& mac = kv.first;
    const IDElement& e = kv.second;
    out += String(e.id) + "_" + mac + "_" + String(e.numberOfMessages) + "|";
  }
  sendLoRaMessage(out);
}

// Received ID list need to be converted to the same format as our own list
void handleIDList(String list) {
  list.remove(0, 7);  // Remove "IDLIST|"

  std::map<String, IDElement, MacLess> remotelist;
  int start = 0;

  while (start < list.length()) {
    int sep = list.indexOf('|', start);
    if (sep == -1) sep = list.length();

    String entry = list.substring(start, sep);
    int c1 = entry.indexOf('_');
    int c2 = entry.indexOf('_', c1 + 1);

    if (c1 > 0 && c2 > c1) {
      int rid = entry.substring(0, c1).toInt();
      String rmac = entry.substring(c1 + 1, c2);
      int rcount = entry.substring(c2 + 1).toInt();
      remotelist[rmac] = { rid, rcount };
    }
    start = sep + 1;
  }

  bool hasEntryForUs = remotelist.find(nodeID) != remotelist.end();

  if (!hasEntryForUs) {
    // Send all our messages (including first)
    for (const auto& msg : crdtList) {
      if (msg.sender == nodeID) {
        for (int i = 0; i < 3; i++) {  // Send 3 times for reliability
          sendLoRaMessage("MSG|" + msg.id + "|" + msg.content);
          delay(300);
        }
      }
    }
  } else {
    // Send missing messages for all nodes we know about
    for (const auto& kv : remotelist) {
      const String& rmac = kv.first;
      int remoteId = kv.second.id;

      for (const auto& local : crdtList) {
        if (local.sender == rmac && local.time > remoteId) {
          for (int i = 0; i < 2; i++) {  // 2 tries for others
            sendLoRaMessage("MSG|" + local.id + "|" + local.content);
            delay(300);
          }
        }
      }
    }
  }
}

// Received message needs to be decoded
void onReceiveMessage(String &received) {
  // Handle single messages
  if (received.startsWith("MSG|")) {
    // Everything after MSG|
    int p1 = received.indexOf('|', 4);
    if (p1 == -1) return;

    // Extract id & content
    String id      = received.substring(4, p1);      // "<time>_<nodeID>"
    String content = received.substring(p1 + 1);     // Rest of msgs

    // Id is time and sender
    int us = id.indexOf('_');
    if (us == -1) return;
    uint32_t time   = id.substring(0, us).toInt();
    String   sender = id.substring(us + 1);

    applyMessage({ id, content, time, sender });

  // Handle receiving of ID lists
  } else if (received.startsWith("IDLIST|")) {
    handleIDList(received);
  }
}
  

// CRDTSTUFF --------------------------------------------------------------


void setup() {
  Serial.begin(115200);

  // WEBSTUFF --------------------------------------------------------------
  WiFi.mode(WIFI_AP);

  // Base-MAC holen
  uint64_t efuseMac = ESP.getEfuseMac();

  // only Byte 0 and 1  (MSB first)
  uint8_t mac0 = (efuseMac >> 40) & 0xFF;
  uint8_t mac1 = (efuseMac >> 32) & 0xFF;

  // macAddress and SSID  
  snprintf(macAddress, sizeof(macAddress), "%02X:%02X", mac0, mac1);
  snprintf(ssid, sizeof(ssid), "ESPCHAT-%s", macAddress);

  WiFi.softAP(ssid);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  dnsServer.start(DNS_PORT, "*", apIP);

  // Webserver-routes
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.onNotFound(handleRoot);
  webServer.on("/send", HTTP_POST, handleSend);
  webServer.on("/messages", HTTP_GET, handleMessages);

  // Webserver-routes
webServer.on("/", HTTP_GET, handleRoot);
webServer.onNotFound(handleRoot);
webServer.on("/send", HTTP_POST, handleSend);
webServer.on("/messages", HTTP_GET, handleMessages);

// Captive portal detection to make sure it works on android
webServer.on("/generate_204", HTTP_GET, []() {
  webServer.send(204, "text/plain", "");
});
webServer.on("/redirect", HTTP_GET, []() {
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
});
webServer.on("/hotspot-detect.html", HTTP_GET, handleRoot); // iOS/macOS
webServer.on("/ncsi.txt", HTTP_GET, handleRoot);             // Windows


  webServer.begin();
  // WEBSTUFF --------------------------------------------------------------

  // OLEDSTUFF --------------------------------------------------------------
  VextON();
  delay(100);

  display.init();
  display.clear();
  display.display();
  display.setContrast(255);

  // Basic setting for displaytext
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  int x = display.width()/2;
  int y = display.height()/2-5;
  display.setFont(ArialMT_Plain_10);

  // Display the name of the Access point
  display.drawString(x, y - 6, "Access-Point:");
  display.drawStringMaxWidth(x, y + 6, 128, ssid);
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
  nodeID = macAddress;
  idlist[nodeID] = { 0, 0 };
  // CRDTSTUFF --------------------------------------------------------------
}

void loop() {
  // WEBSTUFF --------------------------------------------------------------
  dnsServer.processNextRequest();  // DNS replies
  webServer.handleClient();        // HTTP-Requests
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

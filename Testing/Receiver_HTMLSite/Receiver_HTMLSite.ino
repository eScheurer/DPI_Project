#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_now.h>
#include <vector>
#include <Arduino.h>

// --- Configuration ---
#define MAX_MESSAGES     10
const byte DNS_PORT     = 53;
const IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer webServer(80);

String serialBuffer;
std::vector<String> messages;

static const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Chat</title>
  <style>
    body { font-family: sans-serif; max-width:600px; margin:20px auto; }
    #history { border:1px solid #ccc; height:200px; overflow-y:scroll; padding:5px; margin-bottom:10px; }
    input { width:80%; padding:8px; font-size:1em; }
    button { padding:8px 16px; font-size:1em; }
  </style>
  <script>
    async function fetchMessages() {
      try {
        const res = await fetch('/messages'); if (!res.ok) return;
        const data = await res.json();
        const container = document.getElementById('history');
        container.innerHTML = data.map(m => '<div>'+m+'</div>').join('');
        container.scrollTop = container.scrollHeight;
      } catch (e) { console.error('Fetch error', e); }
    }
    async function sendMessage(e) {
      e.preventDefault();
      const input = document.getElementById('msg');
      const text = input.value.trim(); if (!text) return;
      await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(text)});
      input.value = '';
      await fetchMessages();
    }
    window.addEventListener('load', () => {
      document.getElementById('chatForm').onsubmit = sendMessage;
      fetchMessages(); setInterval(fetchMessages, 2000);
    });
  </script>
</head>
<body>
  <h1>ESP32 Chat</h1>
  <div id="history"></div>
  <form id="chatForm">
    <input id="msg" autocomplete="off" placeholder="Enter message..." />
    <button type="submit">Send</button>
  </form>
</body>
</html>
)rawliteral";

void handleRoot() { webServer.send_P(200, "text/html", htmlPage); }
void handleMessages() {
  String json = "[";
  for (size_t i=0; i<messages.size(); ++i) {
    json += "\"" + messages[i] + "\"";
    if (i+1<messages.size()) json += ",";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}
void handleSend() {
  if (webServer.hasArg("msg")) {
    String msg = webServer.arg("msg"); msg.trim();
    if (msg.length()) {
      if (messages.size()>=MAX_MESSAGES) messages.erase(messages.begin());
      messages.push_back(msg);
      Serial.println(msg);
    }
  }
  webServer.send(200, "text/plain", "OK");
}

void onDataRecv(const uint8_t * macAddr, const uint8_t *pdata, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           macAddr[5],macAddr[4],macAddr[3],macAddr[2],macAddr[1],macAddr[0]);
  String msg;
  for (int i=0; i<len; ++i) msg += char(pdata[i]);
  if (messages.size()>=MAX_MESSAGES) messages.erase(messages.begin());
  messages.push_back(msg);
  Serial.printf("ESP-NOW recv from %s: %s\n", macStr, msg.c_str());
}

void setup() {
  Serial.begin(115200); delay(1000);
  messages.reserve(MAX_MESSAGES);

  // WiFi AP+STA for ESP-NOW + Webserver
  WiFi.mode(WIFI_AP_STA);

  // ESP-NOW init
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(onDataRecv));

  // AP setup
  uint64_t efuseMac = ESP.getEfuseMac();
  uint8_t mac[6]; for (int i=0;i<6;i++) mac[i] = (efuseMac>>(8*(5-i)))&0xFF;
  char ssid[32];
  snprintf(ssid,sizeof(ssid),"ESPCHAT-%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  WiFi.softAP(ssid);
  WiFi.softAPConfig(apIP,apIP,IPAddress(255,255,255,0));
  dnsServer.start(DNS_PORT,"*",apIP);
  Serial.printf("AP gestartet: SSID=%s, IP=%s\n", ssid, WiFi.softAPIP().toString().c_str());

  // Webserver
  webServer.on("/",HTTP_GET,handleRoot);
  webServer.onNotFound(handleRoot);
  webServer.on("/send",HTTP_POST,handleSend);
  webServer.on("/messages",HTTP_GET,handleMessages);
  webServer.begin();
  Serial.println("Webserver läuft, Chat bereit");
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  while (Serial.available()) {
    char c = Serial.read();
    if (c=='\n'||c=='\r') {
      if (serialBuffer.length()) {
        if (serialBuffer=="ls") {
          Serial.println("--- Nachrichten ---");
          for (size_t i=0;i<messages.size();++i)
            Serial.printf("%zu: %s\n", i+1, messages[i].c_str());
          Serial.println("--------------------");
        } else if (serialBuffer.equalsIgnoreCase("MAC")) {
          uint8_t macOut[6];
          WiFi.macAddress(macOut);
          Serial.printf("ESP-NOW MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                        macOut[0], macOut[1], macOut[2],
                        macOut[3], macOut[4], macOut[5]);
        } else {
          if (messages.size()>=MAX_MESSAGES) messages.erase(messages.begin());
          messages.push_back(serialBuffer);
          Serial.println(serialBuffer);
        }
        serialBuffer.clear();
      }
    } else serialBuffer+=c;
  }
}

#include <vector>
#include <SPI.h>
#include <WiFi.h>
#include <algorithm>
#include <LoRa.h>
#include "mbedtls/base64.h"  // Base64 for ESP32

// TODO WiFi AP settings
const char* ssid;

// TODO LoRa settings

/**
/ Struct to hold CRDT information
/ Unique id + message content
*/
struct CRDTNode {
  String id;
  String content;
  uint32_t time;
  String sender;
};

std::vector<CRDTNode> crdtList;
uint32_t localClock = 0;
String nodeID;

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
  String packet = "MSG|" + id + "|" + String(time) + "|" + nodeID + "|" + content;
  // TODO Send over LoRa
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
  // TODO Send message over LoRa
}

/**
 * Base64 encoding for String
 */
String encode(String input) {
  size_t outLen;
  unsigned char output[1024];  // Adjust size if needed
  mbedtls_base64_encode(output, sizeof(output), &outLen, (const unsigned char*)input.c_str(), input.length());
  return String((char*)output);
}

/**
 * Base64 decoding for String
 */
String decode(String input) {
  size_t outLen;
  unsigned char output[1024];  // Adjust size if needed
  mbedtls_base64_decode(output, sizeof(output), &outLen, (const unsigned char*)input.c_str(), input.length());
  return String((char*)output);
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

  String fullPacket = "";

  // Looping over our own list
  for (auto &msg : crdtList) {
    bool found = false;
    // Looping over the remote list
    for (auto &id : remoteIDs) {
      // Checking if we already have the same IDs
      if (msg.id == id) {
        found = true;
        break;
      }
    }
    // If we finished comparing the current element of our list with the remote list and didn't find a duplicate, we know that that's missing on the remote
    if (!found) {
      // Sending missing message
      fullPacket += msg.id + "|" + String(msg.time) + "|" + msg.sender + "|" + msg.content + ";;";
      // Before change the message was send here
    }
  }
  if (fullPacket.length() > 0) {
    // Base 64 encoding to reduce transmissing size
    String encoded = encode(fullPacket);
    String packet = "PKT|" + encoded;
    // TODO send over LoRa
  }
}


/**
/ Received message needs to be decoded
*/
void onReceiveMessage() {
  // TODO integrate LoRa here as well
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0) return;

  String received = "";
  
  while (LoRa.available()) {
    received += (char)LoRa.read();
  }
  

  // Handle single messages
  if (received.startsWith("MSG|")) {
    int p1 = received.indexOf('|', 4);
    int p2 = received.indexOf('|', p1 + 1);
    int p3 = received.indexOf('|', p2 + 1);
    int p4 = received.indexOf('|', p3 + 1);

    if (p1 == -1 || p2 == -1 || p3 == -1 || p4 == -1) return;

    String id = received.substring(4, p1);
    uint32_t time = received.substring(p1 + 1, p2).toInt();
    String sender = received.substring(p2 + 1, p3);
    String content = received.substring(p3 + 1);

    applyMessage({id, content, time, sender});

  // Handle receiving of ID lists
  } else if (received.startsWith("IDLIST|")) {
    handleIDList(received);

  // Handling of multiple messages
  } else if (received.startsWith("PKT|")) {
    String encoded = received.substring(4);
    String payload = decode(encoded);
    int start = 0;
    int end = payload.indexOf(";;", start);

    while (end != -1) {
      String part = payload.substring(start, end);
      int p1 = part.indexOf('|');
      int p2 = part.indexOf('|', p1 + 1);
      int p3 = part.indexOf('|', p2 + 1);

      if (p1 != -1 && p2 != -1 && p3 != -1) {
        String id = part.substring(0, p1);
        uint32_t time = part.substring(p1 + 1, p2).toInt();
        String sender = part.substring(p2 + 1, p3);
        String content = part.substring(p3 + 1);

        applyMessage({id, content, time, sender});
      }

      start = end + 2;
      // End of one message is at the delimiter ;;
      end = payload.indexOf(";;", start);
    }
  }
  // Differentiate between a single message (MSG|) and the remote list (IDLIST|)
  // For single messages apply the message with applyMessage
  // For the remote list pass it on to handleIDList
  // PKT of multiple messages need to be decoded
  // TODO Add a size check for the messages
}

/**
/ Setting up the webserver
*/
void setupWebServer() {
  // TODO Setup basic webserver that takes imput
  // TODO append this input as ID (createMessageID) + message into the crdt with the applyMessage
  // TODO Send this message with sendMessage over LoRa as well
}

/**
/ ESP setup function
*/
void setup() {
  // TODO WiFi start with MAC address as SSID
  // TODO Start Webserver
  // Setup LoRa
}

/**
/ Main ESP loop
*/
unsigned long lastSync = 0;
const unsigned long syncInterval = 10000; // every 10 sec

void loop() {
  onReceiveMessage();

// Sending ID list every syncInterval seconds
  if (millis() - lastSync > syncInterval) {
    sendIDList();
    lastSync = millis();
  }
}

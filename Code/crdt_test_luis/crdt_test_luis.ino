#include <vector>
#include <SPI.h>
#include <WiFi.h>

// TODO WiFi AP settings

// TODO LoRa settings

/**
/ Struct to hold CRDT information
/ Unique id + message content
*/
struct CRDTNode {
  String id;
  String content;
};

std::vector<CRDTNode> crdtList;

/**
/ Checking if a message already exists based on the unique id
*/
bool messageExists(const String& id) {
  for (auto &msg : crdtList) {
    if (msg.id == id) return true;
  }
  return false;
}

/**
/ Apply a message if it doens't exist yet
*/
void applyMessage(const CRDTNode& newMsg) {
  // TODO Add a size check to not bloat the crdt
  if (!messageExists(newMsg.id)) {
    crdtList.push_back(newMsg);
  }
}

/**
/ Sending the id and message over LoRa
/ This is used if some messages are missing on the remote ESP
*/
void sendMessage(const String& id, const String& content) {
  String packet = "MSG|" + id + "|" + content;
  // TODO Send message over LoRa
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
      sendMessage(msg.id, msg.content);
    }
  }
}

/**
/ LoRa things go here
*/
void onReceiveMessage() {
  // TODO Differentiate between a single message (MSG|) and the remote list (IDLIST|)
  // TODO For single messages apply the message with applyMessage
  // TODO For the remote list pass it on to handleIDList
  // TODO Add a size check for the messages
}

// The message ID is based of the curent time and the WiFi MAC address
String createMessageID() {
  return String(millis()) + "_" + WiFi.macAddress();
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

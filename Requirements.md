# DPI-Projekt LAN and LoRa

### Lecture requirements
Project Profile: build your own net!
* All projects based on "append-only logs” (will be introduced today)
* Distributed offline-first applications i.e. no servers
- create your Conflict-free Replicated Data Types
- add cryptographic techniques if needed
* Communication based on
- Bluetooth Low Energy (BLE)
- Long-Range Radio (LoRA) … and perhaps USB sticks
- or relays in the Internet
* Integration of multiple projects (mini-app catalogue)

### Unsere Idee
Chatroom (using CRDT)
Create network:
Without internet connection, consisting of..
Local connection: LAN
“Global”, wider range connection: LoRa
.. using ESP’s with LoRa and Wifi

### Technische Grundlagen
* Programmiersprache: Arduino (C++) wegen ESP kompatibilität
* IDE: Arduino IDE
* Als Git verknüpfung: GitKraken oder GitHub Desktop
*   jede Person muss die libaries selbst bei sich herunterladen

## Requirements 
LAN:
* A1 Das System muss LAN austrahlen
* A2 Sobald sich eine Person verbindet, muss sich unsere Webseite automatisch öffnen (beim schliessen der Webseite wird man automatisch aus dem netzwerk gekickt)

Chat:
* C1 Das System hat einen allgemeinen Chat.
* C2 Der Chat ist offline first und basierend auf CRDT und append-only log.
* C3 Die WEbseite soll eine Suchfunktion nach für den Chatinhalt ermöglichen.

LoRa:
* L1 Die ESP müssen sich mittels LoRa verbinden können.
* L2 Die ESP müssen den chat laufend (und gemäss C2) updaten.













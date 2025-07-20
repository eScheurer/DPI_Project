# Crisis Communication Tool (LAN & LoRa)

A self‑contained, peer‑to‑peer chat system for disaster scenarios, combining Wi‑Fi captive‑portal chat with long‑range LoRa relay and CRDT‑based synchronization. Built on an ESP32 Heltec WiFi LoRa 32 V3 module, it provides local encrypted messaging without any external infrastructure.

## Features
- Local Chat via Wi‑Fi Captive Portal
    - Users connect to an ESP32‑hosted Wi‑Fi AP.
    - Automatic redirection to a browser‑based chat interface on Apple devices; Android workaround via http://connectivitycheck.gstatic.com/generate_204.
    - Search and highlight chat history in real time.
    - Input validation (no special characters; configurable length limits).

- Long‑Range Relay via LoRa
    - Uses SX1262 modem at 865 MHz (EU band).
    - Optimized LoRa settings (SF 10, CR 4/5, bandwidth 125 kHz, power 10 dBm).
    - Achieved reliable communication over 600 m in field tests.

- Conflict‑Free Replicated Data Type (CRDT)
    - Each message stored as (ID, content, Lamport timestamp, sender ID).
    - Periodic “IDLIST” broadcasts every 10 s to detect and recover missed messages.
    - Deterministic ordering by timestamp then sender.

- End‑to‑End Encryption
    - AES‑128‑CBC with per‑message random IVs.
    - Base64 encoding for safe LoRa transmission.
    - Built with MbedTLS and ESP32’s esp_fill_random().

- On‑board OLED Display
    - Shows the generated Wi‑Fi SSID (ESPCHAT‑XX:YY).
    - Useful when running multiple nodes.

## Hardware Requirements
- 1× Heltec WiFi LoRa 32 (V3) board (ESP32 + SX1262 + 0.96″ OLED) [Heltec Quick Start Guide](https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html)
- USB‑C cable and 5 V power source
- Optional: 3D‑printed enclosure with through‑board antenna mount

## Software Prerequisites
- Arduino IDE
- For the other prerequisites it is be best to follow Heltec's quick start guide: [Heltec Quick Start Guide](https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html)

## Project Structure
├── Development/  
│   ├── Heltec 
│   │   └── ...               # Multiple Arduino folder leading to the final product
│   └── ...                   # Testing our logic without LoRa but ESPNOW
├── Docs/ 
│   ├── CrdtDraft.pdf         # First idea of the CRDT implementation
│   ├── Protokol.md           # Protocol of our first meeting
│   └── Requirements.md       # Defined requirements of our project
├── LANandLoRa/  
│   ├── CrisisChat.ino        # Main Arduino sketch  
│   └── chat_HTML.h           # Embedded HTML        
├── Prints/ 
│   ├── 3D-Print.png          # Picture of enclosure
│   └── EnclosureHeltec.stl   # stl-file of the enclosure
├── Prototyping/              
│   └── ...                   # Prototypes directly out of the example codes from Heltec
└── README.md                 # This document  

## Limitations & Future Work
### Packet Size
EU LoRa regulations limit payload size. Very long IDLIST messages may be truncated, breaking sync.
For the future: chunk large ID lists across multiple packets.

### Volatile Storage
Messages are lost on power‑cycle.
For the future: add external flash or SD card for persistence.

### Security
Symmetric key is hardcoded—vulnerable if hardware is compromised.
For the future: dynamic key exchange or asymmetric authentication.

## Team & Acknowledgements
- Luis Wenger – CRDT logic & synchronization
- Enya Scheurer – Encryption, HTML/JS interface
- Jannick Seper – LoRa/Wi‑Fi/OLED prototyping

Thanks to the University of Basel DPI lecture staff and Heltec for their example code, libraries and documentation. [Heltec Examples](https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html#example)

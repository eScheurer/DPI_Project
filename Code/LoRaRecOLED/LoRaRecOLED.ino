#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"

// --- OLED-Display-Konfiguration ---
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}
void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

// --- LoRa-Konfiguration ---
#define RF_FREQUENCY               915000000
#define LORA_BANDWIDTH             0
#define LORA_SPREADING_FACTOR      7
#define LORA_CODINGRATE            1
#define LORA_PREAMBLE_LENGTH       8
#define LORA_SYMBOL_TIMEOUT        0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

#define BUFFER_SIZE                30

char rxpacket[BUFFER_SIZE];
bool lora_idle = true;

static RadioEvents_t RadioEvents;

// Prototypen
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

void setup() {
  // OLED initialisieren
  VextON();
  delay(100);
  display.init();
  display.clear();
  display.display();
  display.setContrast(255);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);

  // LoRa initialisieren
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(
    MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
    0, true, 0, 0, LORA_IQ_INVERSION_ON, true
  );
}

void loop() {
  // Radio-Interrupts verarbeiten
  Radio.IrqProcess();

  // In RX-Modus wechseln, wenn gerade frei
  if (lora_idle) {
    lora_idle = false;
    Radio.Rx(0);
  }
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  // Empfangenen Payload kopieren und NUL-terminieren
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  // Display-Ausgabe der Nachricht
  display.clear();
  display.drawString(64, 32, rxpacket);
  display.display();

  // Kurz warten, damit man die Nachricht sehen kann
  delay(1000);

  // Wieder bereit für nächsten Empfang
  Radio.Sleep();
  lora_idle = true;
}

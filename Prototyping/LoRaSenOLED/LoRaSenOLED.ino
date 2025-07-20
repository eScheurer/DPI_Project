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
#define TX_OUTPUT_POWER            5
#define LORA_BANDWIDTH             0
#define LORA_SPREADING_FACTOR      7
#define LORA_CODINGRATE            1
#define LORA_PREAMBLE_LENGTH       8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false

#define RX_TIMEOUT_VALUE           1000
#define BUFFER_SIZE                30

char txpacket[BUFFER_SIZE];
double txNumber;
bool lora_idle = true;

static RadioEvents_t RadioEvents;

// Forward-Deklarationen
void OnTxDone( void );
void OnTxTimeout( void );

void setup() {
  // 1) OLED initialisieren
  VextON();
  delay(100);
  display.init();
  display.clear();
  display.display();
  display.setContrast(255);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);

  // 2) LoRa initialisieren
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  txNumber = 0;

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
}

void loop() {
  // Prozessiere Radio-Interrupts
  Radio.IrqProcess();

  // Wenn frei, sende Paket und zeige es auf dem Display
  if (lora_idle) {
    delay(1000);
    txNumber += 0.01;
    snprintf(txpacket, BUFFER_SIZE, "Hello %.2f", txNumber);

    // Display-Ausgabe
    display.clear();
    display.drawString(64, 32, txpacket);
    display.display();

    Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    lora_idle = false;
  }
}

// Callback, wenn Senden erfolgreich war
void OnTxDone(void) {
  lora_idle = true;
}

// Callback, wenn Senden abgelaufen ist
void OnTxTimeout(void) {
  Radio.Sleep();

  display.clear();
  display.drawString(64, 32, "TX timeout");
  display.display();
  delay(500);

  lora_idle = true;
}

#include "LoRaWan_APP.h"

#define RF_FREQUENCY    865000000
#define TX_OUTPUT_POWER 10
#define LORA_BANDWIDTH  0
#define LORA_SPREADING_FACTOR 12
#define LORA_CODINGRATE 3
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

static RadioEvents_t RadioEvents;
const uint16_t BUF_SIZE = 256;
char ioBuffer[BUF_SIZE];      // für Serielle Ein-/Ausgabe und LoRa

void OnTxDone() {
    Serial.println(F("→ Senden abgeschlossen"));
    // Nach dem Senden direkt wieder in den Empfangsmodus gehen:
    Radio.Rx(0);
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    // Null-Terminierung
    if (size < BUF_SIZE) {
        payload[size] = '\0';
        // Nur den Payload ausgeben:
        Serial.print(F("← Empfangen: "));
        Serial.println((char*)payload);
    }
    // Gleich wieder reinhören
    Radio.Rx(0);
}

void setup() {
    Serial.begin(115200);
    while(!Serial);

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

    // Direkt in Empfangsmodus, damit wir jederzeit lauschen
    Serial.println(F("Setup fertig. Tippe etwas und drücke ENTER."));
    Radio.Rx(0);
}

void loop() {
    // 1) LoRa-Interrupts verarbeiten
    Radio.IrqProcess();

    // 2) Serielle Eingabe lesen und per LoRa senden
    if (Serial.available()) {
        size_t len = Serial.readBytesUntil('\n', ioBuffer, BUF_SIZE - 1);
        if (len > 0) {
            ioBuffer[len] = '\0';
            Serial.print(F("→ Sende: "));
            Serial.println(ioBuffer);
            Radio.Send((uint8_t*)ioBuffer, len);
            // Hier kommt OnTxDone() und dann automatisch wieder Rx
        }
    }
}

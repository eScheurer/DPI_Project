#include "HT_SSD1306Wire.h"
#include "Arduino.h"

//rotate only for GEOMETRY_128_64
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Makro für Display-Power (wie in Deinem Beispiel)
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}
void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  // wie gehabt
  VextON();
  delay(100);

  display.init();
  display.clear();
  display.display();
  display.setContrast(255);

  // Basis-Einstellungen für Text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
}

void loop() {
  static bool isPing = true;

  // Bildschirm löschen, Text zentriert ausgeben
  display.clear();
  display.drawString(64, 32, isPing ? "PING" : "PONG");
  display.display();

  // umschalten und 100 ms warten
  isPing = !isPing;
  delay(1000);
}

#include <ST77916_QSPI_GFX.h>

// Working wiring from the bring-up:
// LCD SCL -> GPIO17
// LCD IO0 -> GPIO16
// LCD IO1 -> GPIO15
// LCD IO2 -> GPIO14
// LCD IO3 -> GPIO13
// LCD RST -> GPIO18
// LCD CS  -> GPIO21
// LCD BL  -> GPIO5
ST77916_QSPI_GFX tft(17, 16, 15, 14, 13, 18, 21, 5);

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!tft.begin(70000000)) {
    Serial.println("ST77916 begin failed");
    while (true) delay(1000);
  }

  tft.setTextWrap(false);
}

void loop() {
  uint32_t start = millis();
  tft.fillScreen(ST77916_RED);
  tft.display();
  Serial.printf("red frame %lu ms\n", (unsigned long)(millis() - start));
  delay(400);

  start = millis();
  tft.fillScreen(ST77916_GREEN);
  tft.display();
  Serial.printf("green frame %lu ms\n", (unsigned long)(millis() - start));
  delay(400);

  start = millis();
  tft.fillScreen(ST77916_BLUE);
  tft.display();
  Serial.printf("blue frame %lu ms\n", (unsigned long)(millis() - start));
  delay(400);

  tft.fillScreen(ST77916_BLACK);
  tft.drawCircle(180, 180, 150, ST77916_CYAN);
  tft.drawCircle(180, 180, 100, ST77916_GREEN);
  tft.drawCircle(180, 180, 50, ST77916_YELLOW);
  tft.drawLine(0, 180, 359, 180, ST77916_GREEN);
  tft.drawLine(180, 0, 180, 359, ST77916_GREEN);
  tft.setTextColor(ST77916_WHITE, ST77916_BLACK);
  tft.setTextSize(3);
  tft.setCursor(62, 90);
  tft.print("ST77916");
  tft.setTextColor(ST77916_GREEN, ST77916_BLACK);
  tft.setTextSize(2);
  tft.setCursor(80, 150);
  tft.print("QSPI GFX OK");
  tft.display();
  delay(1200);
}

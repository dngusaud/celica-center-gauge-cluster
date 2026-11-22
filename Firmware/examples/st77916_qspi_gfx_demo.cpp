#include <ST77916_QSPI.h>

#define TFT_SCLK 17
#define TFT_IO0  16
#define TFT_IO1  15
#define TFT_IO2  14
#define TFT_IO3  13
#define TFT_RST  18
#define TFT_CS   21
#define TFT_BL   5

ST77916_QSPI_GFX tft(TFT_SCLK, TFT_IO0, TFT_IO1, TFT_IO2, TFT_IO3, TFT_RST, TFT_CS, TFT_BL);

void setup() {
  Serial.begin(115200);
  delay(1000);
  ST77916_QSPI_GFX::Config cfg;
  cfg.host = SPI2_HOST;
  cfg.pushTask = false;                   // transmit on the other core

  if (!tft.begin(cfg)) {
    Serial.printf("TFT error: %s\n", tft.lastError());
    while (true) delay(1000);
  }
  else{
    Serial.printf("display %dx%d\n", tft.PANEL_W, tft.PANEL_H);
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

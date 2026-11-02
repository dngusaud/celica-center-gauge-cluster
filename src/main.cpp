#include <Arduino.h>
#include <ST77916_QSPI.h>
#include <AnimatedGIF.h>
#include <SPI.h>
#include <SD.h>

// Pin definitions for the SD card
#define SD_SCK  10
#define SD_MISO 12
#define SD_MOSI 11
#define SD_CS   3

// Pin definitions for the ST77916 display
#define TFT_SCLK 17
#define TFT_IO0  16
#define TFT_IO1  15
#define TFT_IO2  14
#define TFT_IO3  13
#define TFT_RST  18
#define TFT_CS   21
#define TFT_BL   5

// Create instances of the display and SD card SPI interface
ST77916_QSPI_GFX tft(TFT_SCLK, TFT_IO0, TFT_IO1, TFT_IO2, TFT_IO3, TFT_RST, TFT_CS, TFT_BL);
SPIClass sdSPI(FSPI);

void sd_setup() {
  delay(1000); // Wait for Serial to initialize
  Serial.print("Initializing SD card...");
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI, 20000000)){
      Serial.println("SD card initialisation failed");
  }
  else{
      Serial.println("SD card ready");
  }
}

void tft_setup() {
  delay(1000); // Wait for Serial to initialize

  if (!tft.begin(70000000)) {
    Serial.println("ST77916 begin failed");
    while (true) delay(1000);
  }
  else{
    Serial.println("ST77916 begin success");
  }
  tft.setTextWrap(false);
}

void tft_test(){
  // Fill the screen with red, green, and blue colors, displaying each for 400 milliseconds
  tft.fillScreen(ST77916_RED);
  tft.display();
  delay(400);

  tft.fillScreen(ST77916_GREEN);
  tft.display();
  delay(400);

  tft.fillScreen(ST77916_BLUE);
  tft.display();
  delay(400);
}

void setup() {
  Serial.begin(115200);
  
  sd_setup();
  tft_setup();
}

void loop() {
  tft_test();
}
  

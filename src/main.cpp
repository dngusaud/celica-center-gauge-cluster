#include <Arduino.h>
#include <ST77916_QSPI.h>
#include <AnimatedGIF.h>
#include <SPI.h>
#include <SD.h>
#include "esp_heap_caps.h"

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
AnimatedGIF gif;
SPIClass sdSPI(HSPI);

static File gifFile;
static int16_t offX = 0, offY = 0;

// ---- the integration ---------------------------------------------------------
static void GIFDraw(GIFDRAW *pDraw) {
  const int16_t y = (int16_t)(offY + pDraw->iY + pDraw->y);
  const int16_t x = (int16_t)(offX + pDraw->iX);

  // On the first line of a frame that asked for it, clear to the background.
  if (pDraw->y == 0 && pDraw->ucDisposalMethod == 2)
    tft.fillRect(offX, offY, pDraw->iWidth, pDraw->iHeight, ST77916_BLACK);

  tft.pushImage8(x, y, pDraw->iWidth, 1,
                 pDraw->pPixels, pDraw->pPalette,
                 pDraw->ucHasTransparency ? (int16_t)pDraw->ucTransparent : (int16_t)-1,
                 ST77916_PIXELS_BE);
}

// ---- file callbacks ----------------------------------------------------------
static void *gifOpen(const char *fname, int32_t *pSize) {
  gifFile = SD.open(fname, FILE_READ);
  if (!gifFile) return nullptr;
  *pSize = gifFile.size();
  return (void *)&gifFile;
}
static void gifClose(void *handle) {
  File *f = static_cast<File *>(handle);
  if (f) f->close();
}
static int32_t gifRead(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *f = static_cast<File *>(pFile->fHandle);
  const int32_t avail = pFile->iSize - pFile->iPos;
  if (iLen > avail) iLen = avail;
  const int32_t got = f->read(pBuf, iLen);
  pFile->iPos = f->position();
  return got;
}
static int32_t gifSeek(GIFFILE *pFile, int32_t iPosition) {
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}

static void playGif(const char *path) {
  if (!gif.open(path, gifOpen, gifClose, gifRead, gifSeek, GIFDraw)) {
    Serial.printf("cannot open %s (err %d)\n", path, gif.getLastError());
    return;
  }
  offX = (int16_t)((tft.PANEL_W - gif.getCanvasWidth())  / 2);
  offY = (int16_t)((tft.PANEL_H - gif.getCanvasHeight()) / 2);
  Serial.printf("%s  %dx%d\n", path, gif.getCanvasWidth(), gif.getCanvasHeight());

  tft.fillScreen(ST77916_BLACK);
  tft.displayFull();

  int delayMs = 0;
  while (gif.playFrame(false, &delayMs)) {
    tft.display();                       // only the dirty region goes out
    if (delayMs > 0) delay(delayMs);
  }
  gif.close();
}

void sd_setup() {
  delay(1000); // Wait for Serial to initialize
  Serial.print("Initializing SD card...");
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI)) {
      Serial.println("SD card initialisation failed");
      while (true) {delay(1000);}
  }
  else{
      Serial.println("SD card ready");
  }
}

void tft_setup() {
  delay(1000); // Wait for Serial to initialize

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
    delay(1000);


    sd_setup();
    tft_setup();

    gif.begin(GIF_PALETTE_RGB565_BE);
}


void loop() {
  File root = SD.open("/");
  while (File entry = root.openNextFile()) {
    String name = entry.name();
    entry.close();
    if (name.endsWith(".gif") || name.endsWith(".GIF")) {
      String path = name.startsWith("/") ? name : ("/" + name);
      playGif(path.c_str());
    }
  }
  root.close();
}

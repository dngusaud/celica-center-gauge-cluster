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
static uint32_t drawUsAccum = 0;   // time spent inside GIFDraw/pushImage8
static uint32_t readUsAccum = 0;   // time spent inside gifRead (SD I/O)

// ---- PSRAM buffers ---------------------------------------------------------
static uint8_t *gifBuffer = nullptr;
static size_t gifBufferSize = 0;


// ---- Set up functions ----------------------------------------------------------
void sd_setup() {
  delay(1000); // Wait for  Serial to initialize
  Serial.print("Initializing SD card...");
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI, 40000000)) { // 40 MHz
       Serial.println("SD card initialisation failed");
      while (true) {delay(1000);}
  }
  else{
       Serial.println("SD card ready");
  }
}

void tft_setup() {
  ST77916_QSPI_GFX::Config cfg;
  cfg.clockHz = 40000000;                 // 40 MHz
  cfg.pushTask = false;                   // keep this false, otherwise screen flickers
  cfg.doubleBuffer = true;

  if (!tft.begin(cfg)) {
    Serial.printf("TFT error: %s\n", tft.lastError());
    while (true) delay(1000);
  }
  else{
    Serial.printf("display %dx%d\n", tft.PANEL_W, tft.PANEL_H);
  }
}

bool loadGifToPSRAM(const char *path) {
  File file = SD.open(path, FILE_READ);

  // Check if the file was opened successfully
  if (!file) {
      Serial.println("Failed to open GIF");
      return false;
  }

  // Get the size of the GIF and allocate memory in PSRAM
  gifBufferSize = file.size();
  gifBuffer = (uint8_t *)heap_caps_malloc(gifBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!gifBuffer) {
      Serial.println("PSRAM allocation failed");
      file.close();
      return false;
  }
  // Read and copy the GIF data into the PSRAM buffer
  size_t bytesRead = file.read(gifBuffer, gifBufferSize);
  file.close();

  // Check if the entire GIF was read successfully
  if (bytesRead != gifBufferSize) {
      Serial.println("Incomplete GIF read");
      heap_caps_free(gifBuffer);
      gifBuffer = nullptr;
      gifBufferSize = 0;
      return false;
  }
  Serial.printf("GIF loaded: %u bytes\n", (unsigned int)gifBufferSize);
  return true;
}

static void GIFDraw(GIFDRAW *pDraw) {
  const int16_t y = (int16_t)(offY + pDraw->iY + pDraw->y);
  const int16_t x = (int16_t)(offX + pDraw->iX);

  if (pDraw->y == 0 && pDraw->ucDisposalMethod == 2)
    tft.fillRect(offX, offY, pDraw->iWidth, pDraw->iHeight, ST77916_BLACK);

  const uint32_t t0 = micros();
  tft.pushImage8(x, y, pDraw->iWidth, 1,
                pDraw->pPixels, pDraw->pPalette,
                pDraw->ucHasTransparency ? (int16_t)pDraw->ucTransparent : (int16_t)-1,
                ST77916_PIXELS_BE);
  drawUsAccum += micros() - t0;
}

static void *gifOpen(const char *fname, int32_t *pSize) {
  if (!gifBuffer) return nullptr;
  *pSize = (int32_t)gifBufferSize;
  return gifBuffer;
}

static void gifClose(void *handle) {
    // Keep GIF in PSRAM for subsequent playback.
}

static int32_t gifRead(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  uint8_t *buffer = (uint8_t *)pFile->fHandle;
  int32_t available = pFile->iSize - pFile->iPos;

  if (iLen > available) iLen = available;
  if (iLen <= 0) return 0;

  memcpy(pBuf, buffer + pFile->iPos, iLen);

  pFile->iPos += iLen;

  return iLen;
}

static int32_t gifSeek(GIFFILE *pFile, int32_t iPosition) {
  if (iPosition < 0 || iPosition > pFile->iSize)
      return pFile->iPos;

  pFile->iPos = iPosition;

  return pFile->iPos;
}

static void playGif(const char *path) {
  if (!gif.open(path, gifOpen, gifClose, gifRead, gifSeek, GIFDraw)) {
      Serial.printf("cannot open %s (err %d)\n", path, gif.getLastError());
      return;
  }

  // Center the GIF on the display depend on the GIF canvas size.
  // offX and Y are the top left corner of the GIF on the display.
  offX = (int16_t)((tft.PANEL_W - gif.getCanvasWidth()) / 2);
  offY = (int16_t)((tft.PANEL_H - gif.getCanvasHeight()) / 2);
  
  Serial.printf("%s  %dx%d\n", path, gif.getCanvasWidth(), gif.getCanvasHeight());

  int delayMs = 0;
  uint32_t frames = 0, logT0 = millis();
  uint32_t playSum = 0, pushSum = 0, drawSum = 0, readSum = 0;

  while (true) {
    drawUsAccum = 0;
    readUsAccum = 0;
    const uint32_t t0 = micros();
    int rc = gif.playFrame(false, &delayMs);
    if (rc < 0) break;
    const uint32_t playUs = micros() - t0;          // decode + read + GIFDraw
    tft.present();
    const uint32_t pushUs = micros() - t0 - playUs; // panel transfer only

    const int32_t remainMs = delayMs - (int32_t)(playUs + pushUs) / 1000;
    if (remainMs > 0) delay(remainMs);

    frames++;
    playSum  += playUs;
    pushSum  += pushUs;
    drawSum  += drawUsAccum;
    readSum  += readUsAccum;
    if (millis() - logT0 >= 2000) {
      const float n = frames;
      // decode = play - read - draw (the pure LZW work)
      const float decodeUs = (float)playSum - (float)readSum - (float)drawSum;
      Serial.printf("fps=%.1f  play=%.1f  read=%.1f  decode=%.1f  draw=%.1f  push=%.1f  (ms)\n",
                    frames * 1000.0f / (millis() - logT0),
                    playSum  / 1000.0f / n,
                    readSum  / 1000.0f / n,
                    decodeUs / 1000.0f / n,
                    drawSum  / 1000.0f / n,
                    pushSum  / 1000.0f / n);
      frames = 0; playSum = 0; pushSum = 0; drawSum = 0; readSum = 0; logT0 = millis();
    }
  }
}

void setup() {
  Serial.begin(115200);
  sd_setup();
  tft_setup();

  gif.begin(GIF_PALETTE_RGB565_BE);

  if (!loadGifToPSRAM("/sample-butter.gif")) {
      Serial.println("GIF loading failed");
      while (true) delay(1000);
  }
}

void loop() {
  playGif("/sample-butter.gif");
}
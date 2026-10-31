// Animated GIF playback.
//
// Needs bitbank2's AnimatedGIF library (Library Manager: "AnimatedGIF").
//
// The whole integration is the GIFDraw callback below. AnimatedGIF hands over
// one line at a time as 8-bit palette indices plus an RGB565 palette; ask it
// for a big-endian palette and pushImage8 writes straight into the framebuffer
// with no per-pixel conversion at all.
//
// Put .gif files on an SD card. Anything up to 360x360 plays centred.

#include <ST77916_QSPI.h>
#include <AnimatedGIF.h>
#include <SD.h>
#include <SPI.h>

// SD card on its own SPI bus (adjust to your carrier board)
#define SD_SCK  10
#define SD_MISO 12
#define SD_MOSI 11
#define SD_CS   3

// ---- wiring ------------------------------------------------------------
// Edit these to match your board. Any ESP32-S3 with a free QSPI bus (4 data
// lines) plus RST/CS/BL pins works; only the numbers change.
#define TFT_SCLK 17
#define TFT_IO0  16
#define TFT_IO1  15
#define TFT_IO2  14
#define TFT_IO3  13
#define TFT_RST  18
#define TFT_CS   21
#define TFT_BL   5


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

void setup() {
  Serial.begin(115200);
  delay(1000);

  ST77916_QSPI_GFX::Config cfg;
  cfg.pushTask = true;                   // transmit on the other core
  if (!tft.begin(cfg)) {
    Serial.printf("display begin failed: %s\n", tft.lastError());
    while (true) delay(1000);
  }

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("SD mount failed");
    while (true) delay(1000);
  }

  gif.begin(GIF_PALETTE_RGB565_BE);      // matches the framebuffer byte order
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
  delay(500);
}
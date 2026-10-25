#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "driver/spi_master.h"

#define ST77916_BLACK   0x0000
#define ST77916_BLUE    0x001F
#define ST77916_RED     0xF800
#define ST77916_GREEN   0x07E0
#define ST77916_CYAN    0x07FF
#define ST77916_MAGENTA 0xF81F
#define ST77916_YELLOW  0xFFE0
#define ST77916_WHITE   0xFFFF
#define ST77916_ORANGE  0xFD20

class ST77916_QSPI_GFX : public Adafruit_GFX {
public:
  static const int16_t WIDTH = 360;
  static const int16_t HEIGHT = 360;

  ST77916_QSPI_GFX(
    int8_t sclk,
    int8_t io0,
    int8_t io1,
    int8_t io2,
    int8_t io3,
    int8_t rst,
    int8_t cs,
    int8_t bl = -1
  );

  bool begin(uint32_t clockHz = 70000000, bool usePsram = true);
  void display();
  void setBacklight(bool on);
  uint16_t *getBuffer();

  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;

  void pushColor(uint16_t color);
  void pushRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels);

private:
  static const size_t QSPI_CHUNK_BYTES = 8192;

  int8_t _sclk;
  int8_t _io0;
  int8_t _io1;
  int8_t _io2;
  int8_t _io3;
  int8_t _rst;
  int8_t _cs;
  int8_t _bl;
  uint32_t _clockHz;

  spi_device_handle_t _spi;
  uint16_t *_buffer;
  uint8_t *_chunk;

  bool initBus();
  bool allocBuffers(bool usePsram);
  void resetPanel();
  void initPanel();
  void command(uint8_t cmd);
  void command(uint8_t cmd, uint8_t data);
  void command(uint8_t cmd, const uint8_t *data, size_t len);
  void writeCommandPacket(uint8_t cmd);
  void writeDataByte(uint8_t data);
  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void qspiWriteBytes(const uint8_t *data, size_t len);
  void singleWriteBytes(const uint8_t *data, size_t len);
  void writeFramebufferPixels(const uint16_t *pixels, uint32_t count);
  void writeColorPixels(uint16_t color, uint32_t count);
  void csLow();
  void csHigh();
};

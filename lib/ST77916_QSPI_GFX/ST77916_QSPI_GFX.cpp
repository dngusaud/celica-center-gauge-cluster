#include "ST77916_QSPI_GFX.h"
#include "esp_heap_caps.h"

ST77916_QSPI_GFX::ST77916_QSPI_GFX(
  int8_t sclk,
  int8_t io0,
  int8_t io1,
  int8_t io2,
  int8_t io3,
  int8_t rst,
  int8_t cs,
  int8_t bl
) :
  Adafruit_GFX(WIDTH, HEIGHT),
  _sclk(sclk),
  _io0(io0),
  _io1(io1),
  _io2(io2),
  _io3(io3),
  _rst(rst),
  _cs(cs),
  _bl(bl),
  _clockHz(70000000),
  _spi(nullptr),
  _buffer(nullptr),
  _chunk(nullptr) {
}

bool ST77916_QSPI_GFX::begin(uint32_t clockHz, bool usePsram) {
  _clockHz = clockHz;

  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  if (_rst >= 0) {
    pinMode(_rst, OUTPUT);
  }

  if (_bl >= 0) {
    pinMode(_bl, OUTPUT);
    digitalWrite(_bl, HIGH);
  }

  if (!allocBuffers(usePsram)) {
    return false;
  }

  if (!initBus()) {
    return false;
  }

  resetPanel();
  initPanel();
  fillScreen(ST77916_BLACK);
  display();
  return true;
}

bool ST77916_QSPI_GFX::allocBuffers(bool usePsram) {
  if (!_buffer) {
    uint32_t bytes = (uint32_t)WIDTH * (uint32_t)HEIGHT * 2;
    uint32_t caps = MALLOC_CAP_8BIT;
    if (usePsram) {
      caps |= MALLOC_CAP_SPIRAM;
    }

    _buffer = (uint16_t *)heap_caps_malloc(bytes, caps);

    if (!_buffer && usePsram) {
      _buffer = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    }
  }

  if (!_chunk) {
    _chunk = (uint8_t *)heap_caps_malloc(QSPI_CHUNK_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    if (!_chunk) {
      _chunk = (uint8_t *)heap_caps_malloc(QSPI_CHUNK_BYTES, MALLOC_CAP_8BIT);
    }
  }

  return _buffer && _chunk;
}

bool ST77916_QSPI_GFX::initBus() {
  spi_bus_config_t buscfg = {};
  buscfg.sclk_io_num = _sclk;
  buscfg.mosi_io_num = _io0;
  buscfg.miso_io_num = _io1;
  buscfg.quadwp_io_num = _io2;
  buscfg.quadhd_io_num = _io3;
  buscfg.max_transfer_sz = QSPI_CHUNK_BYTES;

  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = _clockHz;
  devcfg.mode = 0;
  devcfg.spics_io_num = -1;
  devcfg.queue_size = 1;
  devcfg.flags = SPI_DEVICE_HALFDUPLEX;

  esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    Serial.printf("ST77916 spi_bus_initialize failed: %d\n", (int)err);
    return false;
  }

  err = spi_bus_add_device(SPI2_HOST, &devcfg, &_spi);
  if (err != ESP_OK) {
    Serial.printf("ST77916 spi_bus_add_device failed: %d\n", (int)err);
    return false;
  }

  return true;
}

void ST77916_QSPI_GFX::setBacklight(bool on) {
  if (_bl >= 0) {
    digitalWrite(_bl, on ? HIGH : LOW);
  }
}

uint16_t *ST77916_QSPI_GFX::getBuffer() {
  return _buffer;
}

void ST77916_QSPI_GFX::csLow() {
  digitalWrite(_cs, LOW);
}

void ST77916_QSPI_GFX::csHigh() {
  digitalWrite(_cs, HIGH);
}

void ST77916_QSPI_GFX::singleWriteBytes(const uint8_t *data, size_t len) {
  if (!_spi || !data || len == 0) return;

  spi_transaction_t trans = {};
  trans.length = len * 8;
  trans.tx_buffer = data;
  spi_device_polling_transmit(_spi, &trans);
}

void ST77916_QSPI_GFX::qspiWriteBytes(const uint8_t *data, size_t len) {
  if (!_spi || !data || len == 0) return;

  spi_transaction_t trans = {};
  trans.flags = SPI_TRANS_MODE_QIO;
  trans.length = len * 8;
  trans.tx_buffer = data;
  spi_device_polling_transmit(_spi, &trans);
}

void ST77916_QSPI_GFX::writeCommandPacket(uint8_t cmd) {
  uint8_t packet[4] = { 0x02, 0x00, cmd, 0x00 };
  singleWriteBytes(packet, sizeof(packet));
}

void ST77916_QSPI_GFX::writeDataByte(uint8_t data) {
  singleWriteBytes(&data, 1);
}

void ST77916_QSPI_GFX::command(uint8_t cmd) {
  csLow();
  writeCommandPacket(cmd);
  csHigh();
}

void ST77916_QSPI_GFX::command(uint8_t cmd, uint8_t data) {
  csLow();
  writeCommandPacket(cmd);
  writeDataByte(data);
  csHigh();
}

void ST77916_QSPI_GFX::command(uint8_t cmd, const uint8_t *data, size_t len) {
  csLow();
  writeCommandPacket(cmd);
  if (data && len) {
    singleWriteBytes(data, len);
  }
  csHigh();
}

void ST77916_QSPI_GFX::resetPanel() {
  if (_rst >= 0) {
    digitalWrite(_rst, LOW);
    delay(100);
    digitalWrite(_rst, HIGH);
    delay(100);
  }

  if (_bl >= 0) {
    digitalWrite(_bl, HIGH);
  }

  delay(100);
}

void ST77916_QSPI_GFX::initPanel() {
  command(0xF0, 0x28);
  command(0xF2, 0x28);
  command(0x73, 0xF0);
  command(0x7C, 0xD1);
  command(0x83, 0xE0);
  command(0x84, 0x61);
  command(0xF2, 0x82);
  command(0xF0, 0x00);
  command(0xF0, 0x01);
  command(0xF1, 0x01);
  command(0xB0, 0x69);
  command(0xB1, 0x4A);
  command(0xB2, 0x2F);
  command(0xB3, 0x01);
  command(0xB4, 0x69);
  command(0xB5, 0x45);
  command(0xB6, 0xAB);
  command(0xB7, 0x41);
  command(0xB8, 0x86);
  command(0xB9, 0x15);
  command(0xBA, 0x00);
  command(0xBB, 0x08);
  command(0xBC, 0x08);
  command(0xBD, 0x00);
  command(0xBE, 0x00);
  command(0xBF, 0x07);
  command(0xC0, 0x80);
  command(0xC1, 0x10);
  command(0xC2, 0x37);
  command(0xC3, 0x80);
  command(0xC4, 0x10);
  command(0xC5, 0x37);
  command(0xC6, 0xA9);
  command(0xC7, 0x41);
  command(0xC8, 0x01);
  command(0xC9, 0xA9);
  command(0xCA, 0x41);
  command(0xCB, 0x01);
  command(0xCC, 0x7F);
  command(0xCD, 0x7F);
  command(0xCE, 0xFF);
  command(0xD0, 0x91);
  command(0xD1, 0x68);
  command(0xD2, 0x68);
  { const uint8_t data[] = { 0x00, 0xA5 }; command(0xF5, data, sizeof(data)); }
  command(0xF1, 0x10);
  command(0xF0, 0x00);
  command(0xF0, 0x02);
  { const uint8_t data[] = { 0xF0, 0x10, 0x18, 0x0D, 0x0C, 0x38, 0x3E, 0x44, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34 }; command(0xE0, data, sizeof(data)); }
  { const uint8_t data[] = { 0xF0, 0x0F, 0x17, 0x0D, 0x0B, 0x07, 0x3E, 0x33, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34 }; command(0xE1, data, sizeof(data)); }
  command(0xF0, 0x10);
  command(0xF3, 0x10);
  command(0xE0, 0x08);
  command(0xE1, 0x00);
  command(0xE2, 0x00);
  command(0xE3, 0x00);
  command(0xE4, 0xE0);
  command(0xE5, 0x06);
  command(0xE6, 0x21);
  command(0xE7, 0x03);
  command(0xE8, 0x05);
  command(0xE9, 0x02);
  command(0xEA, 0xE9);
  command(0xEB, 0x00);
  command(0xEC, 0x00);
  command(0xED, 0x14);
  command(0xEE, 0xFF);
  command(0xEF, 0x00);
  command(0xF8, 0xFF);
  command(0xF9, 0x00);
  command(0xFA, 0x00);
  command(0xFB, 0x30);
  command(0xFC, 0x00);
  command(0xFD, 0x00);
  command(0xFE, 0x00);
  command(0xFF, 0x00);
  command(0x60, 0x40);
  command(0x61, 0x05);
  command(0x62, 0x00);
  command(0x63, 0x42);
  command(0x64, 0xDA);
  command(0x65, 0x00);
  command(0x66, 0x00);
  command(0x67, 0x00);
  command(0x68, 0x00);
  command(0x69, 0x00);
  command(0x6A, 0x00);
  command(0x6B, 0x00);
  command(0x70, 0x40);
  command(0x71, 0x04);
  command(0x72, 0x00);
  command(0x73, 0x42);
  command(0x74, 0xD9);
  command(0x75, 0x00);
  command(0x76, 0x00);
  command(0x77, 0x00);
  command(0x78, 0x00);
  command(0x79, 0x00);
  command(0x7A, 0x00);
  command(0x7B, 0x00);
  command(0x80, 0x48);
  command(0x81, 0x00);
  command(0x82, 0x07);
  command(0x83, 0x02);
  command(0x84, 0xD7);
  command(0x85, 0x04);
  command(0x86, 0x00);
  command(0x87, 0x00);
  command(0x88, 0x48);
  command(0x89, 0x00);
  command(0x8A, 0x09);
  command(0x8B, 0x02);
  command(0x8C, 0xD9);
  command(0x8D, 0x04);
  command(0x8E, 0x00);
  command(0x8F, 0x00);
  command(0x90, 0x48);
  command(0x91, 0x00);
  command(0x92, 0x0B);
  command(0x93, 0x02);
  command(0x94, 0xDB);
  command(0x95, 0x04);
  command(0x96, 0x00);
  command(0x97, 0x00);
  command(0x98, 0x48);
  command(0x99, 0x00);
  command(0x9A, 0x0D);
  command(0x9B, 0x02);
  command(0x9C, 0xDD);
  command(0x9D, 0x04);
  command(0x9E, 0x00);
  command(0x9F, 0x00);
  command(0xA0, 0x48);
  command(0xA1, 0x00);
  command(0xA2, 0x06);
  command(0xA3, 0x02);
  command(0xA4, 0xD6);
  command(0xA5, 0x04);
  command(0xA6, 0x00);
  command(0xA7, 0x00);
  command(0xA8, 0x48);
  command(0xA9, 0x00);
  command(0xAA, 0x08);
  command(0xAB, 0x02);
  command(0xAC, 0xD8);
  command(0xAD, 0x04);
  command(0xAE, 0x00);
  command(0xAF, 0x00);
  command(0xB0, 0x48);
  command(0xB1, 0x00);
  command(0xB2, 0x0A);
  command(0xB3, 0x02);
  command(0xB4, 0xDA);
  command(0xB5, 0x04);
  command(0xB6, 0x00);
  command(0xB7, 0x00);
  command(0xB8, 0x48);
  command(0xB9, 0x00);
  command(0xBA, 0x0C);
  command(0xBB, 0x02);
  command(0xBC, 0xDC);
  command(0xBD, 0x04);
  command(0xBE, 0x00);
  command(0xBF, 0x00);
  command(0xC0, 0x10);
  command(0xC1, 0x47);
  command(0xC2, 0x56);
  command(0xC3, 0x65);
  command(0xC4, 0x74);
  command(0xC5, 0x88);
  command(0xC6, 0x99);
  command(0xC7, 0x01);
  command(0xC8, 0xBB);
  command(0xC9, 0xAA);
  command(0xD0, 0x10);
  command(0xD1, 0x47);
  command(0xD2, 0x56);
  command(0xD3, 0x65);
  command(0xD4, 0x74);
  command(0xD5, 0x88);
  command(0xD6, 0x99);
  command(0xD7, 0x01);
  command(0xD8, 0xBB);
  command(0xD9, 0xAA);
  command(0xF3, 0x01);
  command(0xF0, 0x00);
  command(0x3A, 0x05);
  command(0x35, 0x00);
  command(0x21);
  command(0x11);
  delay(120);
  command(0x29);
  command(0x2C);
}

void ST77916_QSPI_GFX::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  csLow();
  writeCommandPacket(0x2A);
  writeDataByte(x0 >> 8);
  writeDataByte(x0 & 0xFF);
  writeDataByte(x1 >> 8);
  writeDataByte(x1 & 0xFF);
  csHigh();

  csLow();
  writeCommandPacket(0x2B);
  writeDataByte(y0 >> 8);
  writeDataByte(y0 & 0xFF);
  writeDataByte(y1 >> 8);
  writeDataByte(y1 & 0xFF);
  csHigh();

  csLow();
  writeCommandPacket(0x2C);
  csHigh();
}

void ST77916_QSPI_GFX::writeColorPixels(uint16_t color, uint32_t count) {
  while (count > 0) {
    uint32_t chunkPixels = count;
    uint32_t maxChunkPixels = QSPI_CHUNK_BYTES / 2;
    if (chunkPixels > maxChunkPixels) {
      chunkPixels = maxChunkPixels;
    }

    for (uint32_t i = 0; i < chunkPixels; i++) {
      _chunk[i * 2] = color >> 8;
      _chunk[i * 2 + 1] = color & 0xFF;
    }

    qspiWriteBytes(_chunk, chunkPixels * 2);
    count -= chunkPixels;
  }
}

void ST77916_QSPI_GFX::writeFramebufferPixels(const uint16_t *pixels, uint32_t count) {
  uint32_t sentPixels = 0;

  while (sentPixels < count) {
    uint32_t chunkPixels = count - sentPixels;
    uint32_t maxChunkPixels = QSPI_CHUNK_BYTES / 2;
    if (chunkPixels > maxChunkPixels) {
      chunkPixels = maxChunkPixels;
    }

    for (uint32_t i = 0; i < chunkPixels; i++) {
      uint16_t color = pixels[sentPixels + i];
      _chunk[i * 2] = color >> 8;
      _chunk[i * 2 + 1] = color & 0xFF;
    }

    qspiWriteBytes(_chunk, chunkPixels * 2);
    sentPixels += chunkPixels;
  }
}

void ST77916_QSPI_GFX::pushColor(uint16_t color) {
  setAddrWindow(0, 0, WIDTH - 1, HEIGHT - 1);

  csLow();
  const uint8_t prefix[4] = { 0x32, 0x00, 0x2C, 0x00 };
  singleWriteBytes(prefix, sizeof(prefix));
  writeColorPixels(color, (uint32_t)WIDTH * (uint32_t)HEIGHT);
  csHigh();
}

void ST77916_QSPI_GFX::display() {
  if (!_buffer) return;

  setAddrWindow(0, 0, WIDTH - 1, HEIGHT - 1);

  csLow();
  const uint8_t prefix[4] = { 0x32, 0x00, 0x2C, 0x00 };
  singleWriteBytes(prefix, sizeof(prefix));
  writeFramebufferPixels(_buffer, (uint32_t)WIDTH * (uint32_t)HEIGHT);
  csHigh();
}

void ST77916_QSPI_GFX::pushRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels) {
  if (!pixels || w <= 0 || h <= 0) return;
  if (x < 0 || y < 0 || x + w > WIDTH || y + h > HEIGHT) return;

  setAddrWindow(x, y, x + w - 1, y + h - 1);

  csLow();
  const uint8_t prefix[4] = { 0x32, 0x00, 0x2C, 0x00 };
  singleWriteBytes(prefix, sizeof(prefix));
  writeFramebufferPixels(pixels, (uint32_t)w * (uint32_t)h);
  csHigh();
}

void ST77916_QSPI_GFX::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!_buffer) return;
  if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
  _buffer[(uint32_t)y * WIDTH + x] = color;
}

void ST77916_QSPI_GFX::fillScreen(uint16_t color) {
  if (!_buffer) return;

  for (uint32_t i = 0; i < (uint32_t)WIDTH * (uint32_t)HEIGHT; i++) {
    _buffer[i] = color;
  }
}

void ST77916_QSPI_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (!_buffer) return;
  if (w <= 0 || h <= 0) return;
  if (x >= WIDTH || y >= HEIGHT) return;

  if (x < 0) {
    w += x;
    x = 0;
  }

  if (y < 0) {
    h += y;
    y = 0;
  }

  if (x + w > WIDTH) {
    w = WIDTH - x;
  }

  if (y + h > HEIGHT) {
    h = HEIGHT - y;
  }

  if (w <= 0 || h <= 0) return;

  for (int16_t row = 0; row < h; row++) {
    uint16_t *dst = _buffer + (uint32_t)(y + row) * WIDTH + x;
    for (int16_t col = 0; col < w; col++) {
      dst[col] = color;
    }
  }
}

void ST77916_QSPI_GFX::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  fillRect(x, y, w, 1, color);
}

void ST77916_QSPI_GFX::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  fillRect(x, y, 1, h, color);
}

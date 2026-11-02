/*
  This is a simple test sketch to check if PSRAM is available on the ESP32-S3 board. 
  It initializes the serial communication and checks the size of the PSRAM. 
  If PSRAM is available, it prints the size in bytes; otherwise, 
  it indicates that no PSRAM was found.

  Run this test if "framebuffer allocation failed" error occurs during initialization.

  NOTE: if you are using platoformio, make sure to set the following options to enable PSRAM:
  board_build.arduino.memory_type = qio_opi
  board_build.psram_type = opi
  board_upload.flash_size = 16MB
  build_flags =
    -DBOARD_HAS_PSRAM
  at the platformio.ini file. Otherwise, the sketch will always report that no PSRAM is found.
*/

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  size_t psramSize = ESP.getPsramSize();
  if (psramSize > 0) {
    Serial.print("PSRAM is available. Size: ");
    Serial.print(psramSize);
    Serial.println(" bytes");
  } else {
    Serial.println("No PSRAM found.");
  }
}

void loop() {}

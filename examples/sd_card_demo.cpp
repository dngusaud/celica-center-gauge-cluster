#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

SPIClass sdSPI(FSPI);

constexpr int SD_SCK  = 10;
constexpr int SD_MISO = 12;
constexpr int SD_MOSI = 11;
constexpr int SD_CS   = 3;

void setup()
{
    Serial.begin(115200);
    Serial.print("Initializing SD card...");
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS, sdSPI, 20000000))
    {
        Serial.println("SD card initialisation failed");
    }
    else
    {
        Serial.println("SD card ready");
    }   // nothing happens after setup
}

void loop()
{

}
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#define SD_SCK  10
#define SD_MISO 12
#define SD_MOSI 11
#define SD_CS   3

SPIClass sdSPI(FSPI);

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
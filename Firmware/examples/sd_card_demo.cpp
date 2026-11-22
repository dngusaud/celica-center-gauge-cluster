#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#define SD_SPI_MISO  1
#define SD_SPI_SCK   2
#define SD_SPI_CS    41
#define SD_SPI_MOSI  42

SPIClass sdSPI(FSPI);

void setup()
{
    Serial.begin(115200);
    Serial.print("Initializing SD card...");
    sdSPI.begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_CS);

    if (!SD.begin(SD_SPI_CS, sdSPI, 20000000))
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

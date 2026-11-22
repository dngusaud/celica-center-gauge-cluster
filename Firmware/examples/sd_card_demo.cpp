#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#define SPI_MISO  1
#define SPI_SCK   2
#define SPI_CS    41
#define SPI_MOSI  42

SPIClass sdSPI(FSPI);

void setup()
{
    Serial.begin(115200);
    Serial.print("Initializing SD card...");
    sdSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);

    if (!SD.begin(SPI_CS, sdSPI, 20000000))
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

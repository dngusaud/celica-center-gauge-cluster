
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// SD card SPI pins
#define SPI_MISO  1
#define SPI_SCK   2
#define SPI_CS    41
#define SPI_MOSI  42

// ADC1 pins on ESP32-S3
#define ADC_PIN_1  4
#define ADC_PIN_2  5

#define SAMPLE_INTERVAL_US 1000  // Target 1 kHz

SPIClass sdSPI(HSPI);
File logFile;

uint32_t lastSampleTime = 0;
uint32_t lastFlushTime = 0;
uint32_t sampleCount = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Configure ADC
    analogReadResolution(12);

    // Initialise SD SPI
    sdSPI.begin(
        SPI_SCK,
        SPI_MISO,
        SPI_MOSI,
        SPI_CS
    );

    Serial.println("Initialising SD card...");

    if (!SD.begin(SPI_CS, sdSPI, 20000000)) {
        Serial.println("SD card initialisation FAILED!");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("SD card ready.");

    // Open log file
    logFile = SD.open("/datalog.csv", FILE_APPEND);

    if (!logFile) {
        Serial.println("Failed to open datalog.csv!");
        while (true) {
            delay(1000);
        }
    }

    // Write header if file is empty
    if (logFile.size() == 0) {
        logFile.println("timestamp_us,adc1,adc2");
        logFile.flush();
    }

    lastSampleTime = micros();
    lastFlushTime = millis();

    Serial.println("Logging started.");
}

void loop() {
    uint32_t now = micros();

    if ((uint32_t)(now - lastSampleTime) >= SAMPLE_INTERVAL_US) {

        lastSampleTime += SAMPLE_INTERVAL_US;

        uint32_t timestamp = micros();

        uint16_t adc1 = analogRead(ADC_PIN_1);
        uint16_t adc2 = analogRead(ADC_PIN_2);

        // Write directly to SD card
        size_t written = logFile.printf(
            "%lu,%u,%u\n",
            timestamp,
            adc1,
            adc2
        );

        if (written == 0) {
            Serial.println("SD write FAILED!");
        }

        sampleCount++;

        if (sampleCount % 1000 == 0) {
            Serial.printf(
                "Samples: %lu | ADC1: %u | ADC2: %u\n",
                sampleCount,
                adc1,
                adc2
            );
        }
    }

    // Flush once per second
    if (millis() - lastFlushTime >= 1000) {
        logFile.flush();
        lastFlushTime = millis();
    }
}

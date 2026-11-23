
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
#define ADC_RESOLUTION 12
#define ADC_MAX_VALUE  ((1UL << ADC_RESOLUTION) - 1)
#define ADC_REFERENCE_mV 3300

SPIClass sdSPI(HSPI);
File logFile;

uint32_t lastSampleTime = 0;
uint32_t lastFlushTime = 0;
uint32_t sampleCount = 0;

// ---- Set up functions ----------------------------------------------------------
void sd_setup() {
  Serial.print("Initializing SD card...");
  sdSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);

  if (!SD.begin(SPI_CS, sdSPI, 40000000)) { // 40 MHz
       Serial.println("SD card initialisation failed");
      while (true) {delay(1000);}
  }
  else{
       Serial.println("SD card ready");
  }
}

uint16_t adc_to_mV(uint16_t adcValue) {
    return (uint32_t)adcValue * ADC_REFERENCE_mV / ADC_MAX_VALUE;
}

// ---- Logger setup -----------------------------------------------------

void logger_setup() {
    analogReadResolution(ADC_RESOLUTION);

    logFile = SD.open("/datalog.csv", FILE_APPEND);

    if (!logFile) {
        Serial.println("Failed to open datalog.csv");
        while (true) {
            delay(1000);
        }
    }

    if (logFile.size() == 0) {
        logFile.println("timestamp_ms,adc1(mV),adc2(mV)");
        logFile.flush();
    }

    lastSampleTime = micros();
    lastFlushTime = millis();
    Serial.println("Logger ready");
}

void logger_loop() {
    uint32_t now = micros();

    if ((uint32_t)(now - lastSampleTime) >= SAMPLE_INTERVAL_US) {
        lastSampleTime += SAMPLE_INTERVAL_US;
        uint32_t timestamp = millis();
        uint16_t adc1 = analogRead(ADC_PIN_1);
        uint16_t adc2 = analogRead(ADC_PIN_2);
        size_t written = logFile.printf("%lu,%u,%u\n", timestamp, adc_to_mV(adc1), adc_to_mV(adc2));

        if (written == 0) {
            Serial.println("SD write failed!");
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

    // Flush every second
    if (millis() - lastFlushTime >= 1000) {
        logFile.flush();
        lastFlushTime = millis();
    }
}

void logger_task(void *parameter) {
    while (true) {
        logger_loop();

        taskYIELD();
    }
}


void logger_start() {
    xTaskCreatePinnedToCore(logger_task,"ADC Logger",4096,NULL,2,NULL,1);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    sd_setup();
    logger_setup();
    logger_start();
}

void loop() {
    delay(1);
}


#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// ---- SD card configuration -------------------------------------------

#define SPI_MISO  1
#define SPI_SCK   2
#define SPI_CS    41
#define SPI_MOSI  42

#define SD_SPI_FREQUENCY  40000000
#define LOG_FILENAME      "/datalog.csv"
#define FLUSH_INTERVAL_MS 1000

SPIClass sdSPI(HSPI);
File logFile;

// ---- ADC configuration -----------------------------------------------

#define ADC_CHANNELS       4
#define ADC_RESOLUTION     12
#define ADC_MAX_VALUE      ((1UL << ADC_RESOLUTION) - 1)
#define ADC_REFERENCE_mV   3300

const uint8_t ADC_PINS[ADC_CHANNELS] = {
    4, 5, 6, 7
};

// Independent frequency for each channel
const uint16_t ADC_FREQUENCY[ADC_CHANNELS] = {
    1000, 200, 50, 10
};

// ---- Timer configuration ---------------------------------------------

#define TIMER_FREQUENCY_HZ  1000
#define TIMER_INTERVAL_US   1000

hw_timer_t *loggerTimer = NULL;

// ---- Sampling event queue --------------------------------------------

struct SampleEvent {
    uint32_t timestamp;
    uint8_t mask;
};

QueueHandle_t sampleQueue = NULL;

uint16_t sampleInterval[ADC_CHANNELS];

// ---- Conversion -------------------------------------------------------

uint16_t adc_to_mV(uint16_t adcValue) {

    return (uint32_t)adcValue *
           ADC_REFERENCE_mV / ADC_MAX_VALUE;
}

// ---- SD setup ---------------------------------------------------------

void sd_setup() {

    sdSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);

    if (!SD.begin(SPI_CS, sdSPI, SD_SPI_FREQUENCY)) {
        Serial.println("SD card initialisation failed!");
        while (true) delay(1000);
    }

    logFile = SD.open(LOG_FILENAME, FILE_APPEND);

    if (!logFile) {
        Serial.println("Failed to open log file!");
        while (true) delay(1000);
    }

    if (logFile.size() == 0) {

        logFile.println(
            "timestamp_ms,adc1_mV,adc2_mV,adc3_mV,adc4_mV"
        );

        logFile.flush();
    }

    Serial.println("SD card ready");
}

// ---- Logger setup -----------------------------------------------------

void logger_setup() {

    analogReadResolution(ADC_RESOLUTION);

    // Calculate sampling intervals
    for (int i = 0; i < ADC_CHANNELS; i++) {

        if (ADC_FREQUENCY[i] == 0 ||
            TIMER_FREQUENCY_HZ % ADC_FREQUENCY[i] != 0) {

            Serial.println("Invalid ADC sampling frequency!");
            while (true) delay(1000);
        }

        sampleInterval[i] =
            TIMER_FREQUENCY_HZ / ADC_FREQUENCY[i];
    }

    // Create sampling event queue
    sampleQueue = xQueueCreate(2048, sizeof(SampleEvent));

    if (sampleQueue == NULL) {
        Serial.println("Failed to create sample queue!");
        while (true) delay(1000);
    }

    Serial.println("Logger ready");
}

// ---- Timer interrupt --------------------------------------------------

void IRAM_ATTR logger_timer_isr() {

    static uint32_t ticks = 0;

    BaseType_t higherPriorityTaskWoken = pdFALSE;

    ticks++;

    uint8_t mask = 0;

    for (int i = 0; i < ADC_CHANNELS; i++) {

        if (ticks % sampleInterval[i] == 0) {
            mask |= (1 << i);
        }
    }

    if (mask != 0) {

        SampleEvent event = {
            .timestamp = ticks,
            .mask = mask
        };

        xQueueSendFromISR(
            sampleQueue,
            &event,
            &higherPriorityTaskWoken
        );
    }

    if (higherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// ---- Timer start ------------------------------------------------------

void logger_timer_start() {

    // 80 MHz / 80 = 1 MHz timer clock
    loggerTimer = timerBegin(0, 80, true);

    if (loggerTimer == NULL) {
        Serial.println("Timer initialisation failed!");
        while (true) delay(1000);
    }

    timerAttachInterrupt(
        loggerTimer,
        &logger_timer_isr,
        true
    );

    timerAlarmWrite(
        loggerTimer,
        TIMER_INTERVAL_US,
        true
    );

    timerAlarmEnable(loggerTimer);

    Serial.println("Logger timer started");
}

// ---- Logger task ------------------------------------------------------

void logger_task(void *parameter) {

    SampleEvent event;

    uint32_t lastFlushTime = millis();

    while (true) {

        if (xQueueReceive(
            sampleQueue,
            &event,
            pdMS_TO_TICKS(10)
        ) == pdTRUE) {

            uint16_t adcValues[ADC_CHANNELS] = {0};

            // Read scheduled ADC channels
            for (int i = 0; i < ADC_CHANNELS; i++) {

                if (event.mask & (1 << i)) {

                    adcValues[i] = adc_to_mV(
                        analogRead(ADC_PINS[i])
                    );
                }
            }

            // Prepare CSV fields
            char fields[ADC_CHANNELS][12];

            for (int i = 0; i < ADC_CHANNELS; i++) {

                if (event.mask & (1 << i)) {

                    snprintf(
                        fields[i],
                        sizeof(fields[i]),
                        "%u",
                        adcValues[i]
                    );
                }
                else {
                    fields[i][0] = '\0';
                }
            }

            // Write CSV record
            logFile.printf(
                "%lu,%s,%s,%s,%s\n",
                event.timestamp,
                fields[0],
                fields[1],
                fields[2],
                fields[3]
            );
        }

        // Flush SD card periodically
        if (millis() - lastFlushTime >= FLUSH_INTERVAL_MS) {

            logFile.flush();

            lastFlushTime = millis();
        }
    }
}

// ---- Logger start -----------------------------------------------------

void logger_start() {

    BaseType_t result = xTaskCreatePinnedToCore(
        logger_task,
        "ADC Logger",
        4096,
        NULL,
        2,
        NULL,
        1
    );

    if (result != pdPASS) {
        Serial.println("Logger task creation failed!");
        while (true) delay(1000);
    }

    logger_timer_start();
}

// ---- Arduino setup ----------------------------------------------------

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

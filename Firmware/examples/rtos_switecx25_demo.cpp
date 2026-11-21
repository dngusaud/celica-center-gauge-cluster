#include <Arduino.h>
#include <esp_timer.h>
#include <SwitecX25.h>

// Change these pins to your wiring
#define PIN1 4
#define PIN2 5
#define PIN3 6
#define PIN4 7
#define ADC_1_PIN 9

// Handler
esp_timer_handle_t guage_update_timer;
QueueHandle_t adc_1_queue;


// Constants
const int TOTAL_STEPS = 945; // 945 for 315 deg, 600 for 200 deg, 300 for 100 deg 
const int adc_1_SAMPLE_HZ = 500;
const int GAUGE_STEP_THRESHOLD = 3; // Minimum step change to update gauge position

volatile int targetPosition = 0;

// X25 stepper: typically 315 degrees × 3 steps/degree
SwitecX25 gauge(TOTAL_STEPS, PIN1, PIN2, PIN3, PIN4);


void gaugeTimerCallback(void *arg)
{
    gauge.update();
}


void read_adc_1(void *pvParameters)
{
    int adc_1 = 0;
    int increment = 1;
    while (1)
    {
        adc_1 = analogRead(ADC_1_PIN);
        xQueueOverwrite(adc_1_queue, &adc_1);
        vTaskDelay(pdMS_TO_TICKS(1000/adc_1_SAMPLE_HZ));
    }
}

void update_gauge_position(void *pvParameters)
{
    int adc_1_1 = 0;
    float filtered_adc_1_1 = 0.0f;
    bool filterInitialized = false;
    const float alpha = 0.01f; 
    
    while (1)
    {
        if (xQueueReceive(adc_1_queue, &adc_1_1, portMAX_DELAY) == pdTRUE)
        {
            // Apply low-pass filter to the raw target position
            if (!filterInitialized) {
                filtered_adc_1_1 = adc_1_1;
                filterInitialized = true;
            } else {
                filtered_adc_1_1 += alpha * (adc_1_1 - filtered_adc_1_1);
            }

            // Map the adc 1 reading to the gauge's position
            targetPosition = map(filtered_adc_1_1, 0, 4095, 0, TOTAL_STEPS);
            
            // Only update the gauge position if the change exceeds the threshold
            if (abs(static_cast<int>(gauge.currentStep) - targetPosition) >= GAUGE_STEP_THRESHOLD)
            {
                gauge.setPosition(targetPosition);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void setup()
{
    Serial.begin(115200);
    analogReadResolution(12);
    pinMode(ADC_1_PIN, INPUT);

    adc_1_queue = xQueueCreate(1, sizeof(int));
    
    gauge.zero();
    gauge.setPosition(0);


    esp_timer_create_args_t timerArgs = {};
    timerArgs.callback = &gaugeTimerCallback;
    timerArgs.name = "gauge_update";

    esp_timer_create(&timerArgs, &guage_update_timer);

    // 100 microseconds = 10 kHz
    esp_timer_start_periodic(guage_update_timer, 100);

    xTaskCreate(
        read_adc_1,
        "Read ADC 1",
        2048,
        NULL,
        1,
        NULL
    );

    xTaskCreate(
        update_gauge_position,
        "Update Gauge Position",
        2048,
        NULL,
        1,
        NULL
    );

}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
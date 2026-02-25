#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"     // esp loglama için

#define APP_MAIN  "MAIN_TASK" // app_main için log tag
#define TASK1_TAG "LED1_TASK" // task 1 için log tag
#define TASK2_TAG "LED2_TASK" // task 2 için log tag

#define TASK1_LED1_GPIO 4     // task 1 için led pini
#define TASK2_LED2_GPIO 5     // task 2 için led pini

/* --- Task 1 ---*/ 
void led1_task (void *pvParameters)
{

    // GPIO ayarlama yapılır
    gpio_reset_pin(TASK1_LED1_GPIO);
    gpio_set_direction(TASK1_LED1_GPIO, GPIO_MODE_OUTPUT);

    while(1)
    {

        gpio_set_level(TASK1_LED1_GPIO, 1); // led 1'i aç
        ESP_LOGI(TASK1_TAG, "LED 1 Acildi",esp_log_timestamp()); // loglama

        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 saniye bekle

        gpio_set_level(TASK1_LED1_GPIO, 0); // led 1'i kapat
        ESP_LOGI(TASK1_TAG, "LED 1 Kapandi",esp_log_timestamp()); // loglama

        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 saniye bekle
    }
}

/* --- Task 2 ---*/ 
void led2_task (void *pvParameters)
{
    // GPIO ayarlama yapılır
    gpio_reset_pin(TASK2_LED2_GPIO);
    gpio_set_direction(TASK2_LED2_GPIO, GPIO_MODE_OUTPUT);

    while(1)
    {

        gpio_set_level(TASK2_LED2_GPIO, 1); // led 2'yi aç
        ESP_LOGI(TASK2_TAG, "LED 2 Acildi",esp_log_timestamp()); // loglama

        vTaskDelay(2000 / portTICK_PERIOD_MS); // 2 saniye bekle

        gpio_set_level(TASK2_LED2_GPIO, 0); // led 2'yi kapat
        ESP_LOGI(TASK2_TAG, "LED 2 Kapandi",esp_log_timestamp()); // loglama

        vTaskDelay(2000 / portTICK_PERIOD_MS); // 2 saniye bekle
    }
}

void app_main(void)
{
    ESP_LOGI (APP_MAIN,"App main icerisinde task'ler olusturuluyor !",esp_log_timestamp());


    // task1 oluşturma
    xTaskCreate(
        led1_task,          // task fonksiyonu
        "LED1_TASK",        // task adı
        2048,               // stack boyutu
        NULL,               // parametre
        1,                  // öncelik
        NULL                // task handle
    );

    // task2 oluşturma
    xTaskCreate(
        led2_task,
        "LED2_TASK",
        2048,
        NULL,
        1,
        NULL
    );

    // app main'in işi burada biter
    // tasklar çalışmaya devam eder.
    ESP_LOGI(APP_MAIN,"App main gorevini tamamladı, task'ler calismaya devam ediyor !",esp_log_timestamp());
}

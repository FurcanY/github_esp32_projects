#include <stdio.h>

//GPIO için gerekli olan header
#include <driver/gpio.h>

// FreeRTOS için gerekli header'lar
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 4 numaralı GPIO pinine LED ekleyip yakalım
#define BLINK_GPIO 4


void app_main(void)
{

    /*  ---register düzeyinde GPIO için ayarlamalar yapalım--- */

    // ilk olarak tüm ayarları sıfırlayalım
    gpio_reset_pin(BLINK_GPIO);
    // daha sonra OUTPUT olarak ayarlayalım
    gpio_set_direction(BLINK_GPIO,GPIO_MODE_OUTPUT);

    while (true)
    {
        /* ---ledi yakıp söndürme işlemlerini burada yaparız--- */

        // ledi yakalım
        printf("led yandi \n");
        gpio_set_level(BLINK_GPIO,1);

        // FreeRTOS görevini 1sn uyutalım. (şu anki task'i yani)
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // ledi söndürelim
        printf("led sondu \n");
        gpio_set_level(BLINK_GPIO,0);

        // tekrar bekleyelim
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    
}

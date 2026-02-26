#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// interrupt ile buton kullanma için kullanılır.
#include "freertos/queue.h"  

#include "driver/gpio.h"


#define LED_GPIO 4
#define BUTTON_GPIO 5

// kesme ile button kullanımı //

//kesmede button bilgisi tutmak için queue tanımlanır
static QueueHandle_t button_queue = NULL;

// kesme handler fonksiyonu
static void IRAM_ATTR button_isr_handler(void* arg) {
    int pin = (int ) arg;

    bool button_state = gpio_get_level(pin);
    
    // ISR içerisinde Queue'ye buton durumunu gönder
    // bunun için özel kod vardır.
    xQueueSendFromISR(button_queue, &button_state, NULL);
}

// Buton durumunu işlemek için TASK fonksiyonu tanımlanır
static void button_task (void* arg) {
    int button_state;
    bool led_on = false;
    while (1) {
        // Queue'den buton durumunu al, sonuz bekle (portMAX_DELAY)
        if (xQueueReceive(button_queue, &button_state, portMAX_DELAY)) {
            // kesme gelirse bu if içerisine girilir.

            // debounce için delay
            vTaskDelay(50 / portTICK_PERIOD_MS); // 50ms debounce süresi
            
            // Buton durumuna göre LED'i kontrol et
            // baslıdığında 0 olur
            if (button_state == 0) {
                led_on = !led_on; // LED durumunu değiştir
                gpio_set_level(LED_GPIO, led_on); // LED'i aç veya kapat
            }
        }
    }
}


void app_main(void)
{

    // GPIO pinlerini tanımla
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);  // LED söndür

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY); // pull-up direnci kullan

    // button kesmesini ayarla
    // negatif kenar kesmesi (butona basıldığında tetiklenir)
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE); 

    // kuyruğu oluştur
    button_queue = xQueueCreate(10, sizeof(int)); // 10 elemanlı, int tipinde bir kuyruk

    // kesmeyi başlat
    gpio_install_isr_service(0); // ISR hizmetini başlat
    // button kesmesini handler'a bağla
    // BUTTON_GPIO parametre olarak gönder.
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void*)BUTTON_GPIO); 
    
    // button takip TASK'ını oluştur
    // öncelik 10
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    while (true) {
        // Ana döngüde başka işlemler yapılabilir
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 saniye bekle
    }
}

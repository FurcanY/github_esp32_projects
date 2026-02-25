# ESP-IDF

Bu projede hiçbir ek yardım olmadan, sadece esp-idf framework ile beraber esp kodlamayı öğreneceğim.

Burada da temelden öğrenmem gerekenleri yazacağım. (24.02.2026)

## Blink öncesi bilinmesi gerekenler

İlk olarak esp-idf extension ile template boş bir proje oluşturdum.

### app_main

Aslında `app_main` main isimli varsayılan bir FreeRTOS taskidir.

- ESP32 güç aldığında assembly düzeyinde başlar ve ESP-IDF'nin sağladığı `start_app` RTOS başlatıcı fonksiyonu başlatır.
- Sistem watchdog, saat sinyali gibi periferikleri başlatır. FreeRTOS scheduler (zamanlayıcı) devreye girer.
- Sistem `Main Task` adında bir task oluşturur ve bu taski `app_main` fonskiyonuna yerleştirir.
- Bu task'den dolayı sürekli çalışması gereken kodlar while döngüsü içerisinde tutulur.

ESP32 Güç aldığında adım adım gerçekleştirilenler şudur:
- Güç verildi
- **ROM Bootloader**        : ilk çalışan kod burası, temel donanımı başlatır ve flash bellekten 1.stage bootloaderı yükler.
- **1.Stage bootloader**    : 0x1000 adresinde bulunur. Cache ve MMU'yu başlatır. 2.stage bootloader yükler.
- **2.Stage bootloader**    : Partition Table okur. Seçilen partition'dan uygulama imajını yükler. seçilen uygulamaya geçer

    Örnek Partition şu şekildedir:
    ```bash
    // Örnek partition table (partitions.csv)
    # Name,   Type, SubType, Offset,  Size, Flags
    nvs,      data, nvs,     0x9000,  0x4000,
    otadata,  data, ota,     0xd000,  0x2000,
    phy_init, data, phy,     0xf000,  0x1000,
    factory,  app,  factory, 0x10000, 0x200000,
    ota_0,    app,  ota_0,   0x210000,0x200000,
    ota_1,    app,  ota_1,   0x410000,0x200000,
    ```
    

- **start_cpu0 - start_app - main_task**              : FreeRTOS kernel initialize edilir. IDLE task ve Timer task oluşturulur. Sonra Scheduler başlatılır. İnterrupt aktif edilir. En sonda app_main() çağırılır.

    `start_app()` içerisinde:
    ```c
    // components/esp_system/startup.c
    void start_app(void) {
        prvKernelInit();  // FreeRTOS internal
        
        // Idle task
        xTaskCreate(prvIdleTask, "IDLE", configMINIMAL_STACK_SIZE, 
                    NULL, tskIDLE_PRIORITY, NULL);
        
        // Timer task oluştur (eğer kullanılacaksa)
        #if (configUSE_TIMERS == 1)
            xTaskCreate(prvTimerTask, "TMR", configTIMER_TASK_STACK_DEPTH,
                        NULL, configTIMER_TASK_PRIORITY, NULL);
        #endif
        
        // Scheduler'ı başlat
        vTaskStartScheduler();
        // Bu noktadan sonra artık normal C akışı yoktur
        // Scheduler task'leri yönetmeye başlar
    }
    ```
    **Peki Idle Task ne yapar?**
    ```c
    // FreeRTOS içindeki idle task
    void prvIdleTask(void *pvParameters) {
        while(1) {
            // 1. Task'ler çalışmıyorken CPU'yu boşta tut
            // 2. Varsa task silme işlemlerini temizle
            // 3. Derin uyku (sleep) modunu yönet
            // 4. Watchdog'u besle (kick)
            
            asm("wfi");  // Wait For Interrupt - CPU'yu durdur, enerji tasarrufu
        }
    }
    ```
    **Peki Idle Task ne yapar?**
    ```c
    void prvTimerTask(void *pvParameters) {
        while(1) {
            // Timer queue'sunu kontrol et
            // Zamanı gelen timer'ları çalıştır
            xTimerGenericCommandFromTimer();
        }
    }
    ```
    
    `main_task()`:
    - varsayılan olarak 4KB'dir.
    - en yüksek önceliklidir.
    - Çift çekirdekli ESP'lerde genelde CPU0'da çalışır.
    ```c
        // components/esp_system/startup.c
        static void main_task(void* args) {
            // 1. Scheduler çalışır, delay kullanılır.
            vTaskDelay(pdMS_TO_TICKS(10));
            
            // 2.CPU'yu başlat (dual-core ise)
            #if CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME
                gdbstub_init();
            #endif
            
            // app_main çağrılabilir
            app_main();
            
            // 5. Eğer app_main return ederse (olmamalı)
            vTaskDelete(NULL);
        }
    ```

- **app_main**              : sadece bir FreeRTOS taskidir. Kodumuzu bu task içerisinde çalıştırırız. CPU0'da bu task çalışır.

ESP32 açıldığında CPU0'da otomatik oluşan taskler şunlardır:

- 1.**IDLE0     :** Boşta kalma task'i (priority 0)
- 2.**Tmr Svc   :** Timer servis task'i (priority 1)
- 3.**main      :** app_main'in bulunduğu task (priority 1)
- 4.**ipc0      :** Çekirdekler arası iletişim (priority 24)
- 5.**wifi      :** Wi-Fi driver task'i (priority 23-24)
- 6.**bt        :** Bluetooth task'i (priority 23-24)
- 7.**esp_timer :** ESP32 zamanlayıcı task'i (priority 22)

Scheduler bu taskleri cpu0'da zaman ayarlamasını yapar, böylece sistem çökmez. CPU0 istersek task ekleyebiliriz, bu task çok fazla ağır olmamalı çünkü CPU0'da default ayarlar yapan yöneten taskler mevcut.
> priority fazla olursa çalışma sıklığı en yüksek task o olur.

`app_main` içerisinde CPU0'da çalışan child taskler oluşturulabilir. Child task'in ne zaman çalışacağı Scheduler'in görevidir.

```c
void deneme_task(){

}

void app_main(){
    // ESP32'nin oluşturduğu task
    // bunun içerisinde child task oluşturabiliriz.
    // bu task zaten Schedule içerisinde

    xTaskCreatePinnedCore(
        deneme_task,
        "isim",
        ....
    );
}
```
### Mantıklı Kullanım Nasıldır ?
En mantıklı işlem app_main() içerisinde task oluşturup `app_main` içerisinde return etmektir. Ne demek istiyorum açıklayayım. app_main içeriği bizim ayarlama yaptığımız alan olmalıdır. burada oluşturmak istediğimiz taskler ve gerekli tanımlamalar yapılmalıdır. Sonsuz döngü içerisinde yapmak istediğimiz görevleri ise tanımladığımız task'ler içerisinde olmalıdır.

app_main = setup fonksiyonu olarak kullanılmalıdır.
taskler= app_main içerisinde tanımlanır ve daha sonra scheduler bu task'leri kullanır.

Bu mimaride işlemler yaparsak FreeRTOS görevlerinde ölçeklendirme yapabilir, kaynakları verimli kullanabiliriz. Daha modüler hareket ettiğimiz için yönetimi de kolay olacaktır.

Ayrıca bir task çöktüğü zaman diğerleri çalışmaya devam eder ve daha stabil bir sistemimiz olur.

Örnek verecek olursak:

Aşağıdaki kod yanlış kullanımdır. Bu şekildeki bir sistem daha sonra kontrol edilemez hale gelebilir. Ayrıca sistemin performansını bloke eder.
```c
void app_main(void) {
    while(1) {  // app_main'i bloke etmek
        // Her şeyi burada yapmak
        read_sensors();
        send_wifi();
        update_display();
        vTaskDelay(100);
    }
}
```
Aşağıdaki kod ise mantıklı olandır. Her bir işlem önceliği ile beraber task olarak oluşturulur. Bütün ayarlamalar yapıldıktan sonra app_main içerisinden return ile çıkılır.

```c
void app_main(void) {
    xTaskCreate(task_sensor, "sens", 2048, NULL, 5, NULL);
    xTaskCreate(task_wifi, "wifi", 4096, NULL, 4, NULL);
    xTaskCreate(task_display, "disp", 2048, NULL, 2, NULL);
    // return; 
}
```

## Blink Code

Artık basic ESP32 bilgilerine hakim olduysak led yakabiliriz bence.

İlk olarak gerekli kütüphaneleri çağırmalıyız. GPIO kontrolü için `driver/gpio.h` ve gecikme için `freertos/FreeRTOS.h` `freertos/task.h` lazım.

- `freertos/FreeRTOS.h`: Temel veri tiplerini, config makrolarını, kernel ayarlarını, scheduler ile ilgili low-level tanımları, task handle tiplerini içerir. Kısacası FreeRTOS evrenini başlatır.

- `freertos/task.h`: Task API katmanını, task fonksiyonlarını içerir. API'lar ayrılarak gereksiz çağırım yapılması önlenir.

```bash
Application
   ↓
task.h / queue.h / semphr.h / ...
   ↓
FreeRTOS.h
   ↓
Kernel
```
Çağırılma sırası da önemlidir. İlk olarak FreeRTOS.h çağırılır, bu önerilir.

Esp32 kartımızın 4 numaralı pinine led bağlayarak yakıp söndürelim. Bunun için `define` ile pinimi tanımlıyorum

```c
#define BLINK_GPIO 4
```

Daha sonra app_main içerisinde 4 numaralı pinin tüm ayarlarını sıfırlayıp output için ayarlıyorum

```c
gpio_reset_pin(BLINK_GPIO);
gpio_set_direction(BLINK_GPIO,GPIO_MODE_OUTPUT); // çıkış pini yapılır
```
En sonda ise while döngüsü içerisinde 4 numaralı pine 1 saniye aralıklarla 1 ve 0 değeri veriyoruz.

```c
while (true)
{

    printf("led yandi \n");
    gpio_set_level(BLINK_GPIO,1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("led sondu \n");
    gpio_set_level(BLINK_GPIO,0);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
```
Delay için `vTaskDelay` kullanılır. Şu anda app_main cpu0'da koşturulan bir task'dir. 1000ms delay verdiğimiz zaman task scheduler cpu0'dan app_main task'ini çıkarıp priority'si fazla olan diğer task'leri işlemeye devam eder.



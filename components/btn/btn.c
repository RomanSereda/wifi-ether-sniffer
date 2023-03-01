#include "btn.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TRIPLE_BLINK 3
#define BUILDIN_LED 2
#define BUILDIN_BTN 0

void led_blink(int count)
{
    count *= 2;
    bool flag = false;
    while(count--) {
        vTaskDelay(40 / portTICK_RATE_MS);
        gpio_set_level(BUILDIN_LED, flag);
        flag = !flag;
    }
}

void btn_task(void *arg)
{
    while (true) {
        if(!gpio_get_level(BUILDIN_BTN)) {
            led_blink(TRIPLE_BLINK);
        }
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void init_btn()
{
    gpio_set_direction(BUILDIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(BUILDIN_LED, true);

    gpio_config_t buildin_btn_conf = {0};
    buildin_btn_conf.mode = GPIO_MODE_INPUT;
    buildin_btn_conf.pin_bit_mask = 1ULL << BUILDIN_BTN;
    buildin_btn_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&buildin_btn_conf);

    TaskHandle_t taskHandleBtn;
    BaseType_t result = xTaskCreate(btn_task, "btn_task", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, &taskHandleBtn);

    if(result == pdPASS)
    {
        printf("btn error create task: %d\n", result);
    }
}

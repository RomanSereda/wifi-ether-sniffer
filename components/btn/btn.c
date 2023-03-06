#include "btn.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUILDIN_LED 2
#define BUILDIN_BTN 0

static invoke_func_t ifunc;

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

static void btn_task(void *arg)
{
    while (true) {
        if(!gpio_get_level(BUILDIN_BTN)) {
            led_blink(TRIPLE_BLINK);
            if(ifunc) ifunc();
        }
        vTaskDelay(50 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

void init_btn(invoke_func_t ifnc)
{
    ifunc = NULL;
    gpio_set_direction(BUILDIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(BUILDIN_LED, true);

    gpio_config_t buildin_btn_conf = {0};
    buildin_btn_conf.mode = GPIO_MODE_INPUT;
    buildin_btn_conf.pin_bit_mask = 1ULL << BUILDIN_BTN;
    buildin_btn_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&buildin_btn_conf);

    TaskHandle_t taskHandleBtn;
    BaseType_t result = xTaskCreate(btn_task, "btn_task", 
        configMINIMAL_STACK_SIZE * 32, NULL, tskIDLE_PRIORITY + 3, &taskHandleBtn);

    if(result != pdPASS)
    {
        ets_printf("Btn error, not created task: %d\n", result);
    }
    ifunc = ifnc;
}

void disable_blink()
{
    gpio_set_level(BUILDIN_LED, true);
}
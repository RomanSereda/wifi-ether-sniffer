#include "scan_service.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_libc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "parser.h"
#include "container.h"

static uint8_t setup_channel = 13;
static uint32_t milis()
{
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}
static void promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    struct frame_data_t data = {0};
    if(parse(buf, &data)){
        containerize(&data, 0 /*setup_channel*/);
    }

    static uint32_t start_time;
    if(!start_time) start_time = milis();
    if((milis() - start_time) > 12000) {
        start_time = 0;

        if(setup_channel == 13) setup_channel = 0;
        ESP_ERROR_CHECK(esp_wifi_set_channel(++setup_channel, 0));
        ets_printf("Channel changed: %d\n", setup_channel);
    }

    vTaskDelay(10 / portTICK_RATE_MS);
}

static const int START_BIT = BIT0;
static EventGroupHandle_t wifi_event_group;
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        xEventGroupSetBits(wifi_event_group, START_BIT);
    }
}

static const bool filter_frame_payload = false;
static void scan_task(void* pvParameters)
{
    wifi_promiscuous_filter_t filter = {0};

    uint32_t filter_mask = 0;
    filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;   //Receive management packets
    //filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL; //Receive ctrl packets
    //filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA; //Receive data packets
    //filter_mask |= WIFI_PROMIS_FILTER_MASK_MISC;
    //filter_frame_payload = true;                //Receive data frame payload

    filter.filter_mask = filter_mask;
    if(filter_frame_payload){
        extern esp_err_t esp_wifi_set_recv_data_frame_payload(bool enable_recv);
        ESP_ERROR_CHECK(esp_wifi_set_recv_data_frame_payload(true));
    }
    
    xEventGroupWaitBits(wifi_event_group, START_BIT,
                        false, true, portMAX_DELAY);
    ESP_ERROR_CHECK(esp_wifi_set_channel(setup_channel, 0));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promiscuous_rx_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    vTaskDelete(NULL);
}

static bool is_running = false;
static TaskHandle_t taskScanChannel;
esp_err_t start_scan_service()
{
    if(is_running){
        ets_printf("Scan can not start, service running\n");
        return ESP_FAIL;
    }

    if (wifi_event_group != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(esp_wifi_start());

    BaseType_t result = xTaskCreate(scan_task, "scan_task", 
        configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 1, &taskScanChannel);
    if(result != pdPASS)
    {
        ets_printf("Scan error, not created task: %d\n", result);
    }

    is_running = true;
    return ESP_OK;
}

esp_err_t stop_scan_service()
{
    if(!is_running){
        ets_printf("Scan can not stop, service not running.\n");
        return ESP_FAIL;
    }

    if (wifi_event_group == NULL) {
        ets_printf("Scan can not stop, event group not created.\n");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_promiscuous_filter_t filter = {0};
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

    vEventGroupDelete(wifi_event_group);
    wifi_event_group = NULL;

    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));

    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        ets_printf("Scan can not stop, wifi not started.\n");
        return ESP_ERR_WIFI_NOT_INIT;
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_wifi_deinit());
    ets_printf("Scan was stoped.\n");

    is_running = false;
    return ESP_OK;
}


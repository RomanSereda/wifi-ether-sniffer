#include "scan.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_libc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "parser.h"

#ifdef DEBUG
#include "esp_log.h"
#define TAG "scan"
#define MAC_HEADER_LEN 24
#define SCAN_DATA_LEN 112
#define MAC_HDR_LEN_MAX 40
static char printbuf[256];
#endif

static void scan_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    struct frame_data_t data;
    parse(buf, type, &data);

#ifdef DEBUG
    wifi_pkt_rx_ctrl_t* rx_ctrl = (wifi_pkt_rx_ctrl_t*)buf;
    uint8_t* frame = (uint8_t*)(rx_ctrl + 1);
    uint32_t len = rx_ctrl->sig_mode ? rx_ctrl->HT_length : rx_ctrl->legacy_length;
    uint32_t i;

    uint8_t total_num = 1, count = 0;
    uint16_t seq_buf = 0;

    if ((rx_ctrl->aggregation) && (type != WIFI_PKT_MISC)) {
        total_num = rx_ctrl->ampdu_cnt;
    }

    for (count = 0; count < total_num; count++) {
        if (total_num > 1) {
            len = *((uint16_t*)(frame + MAC_HDR_LEN_MAX + 2 * count));

            if (seq_buf == 0) {
                seq_buf = *((uint16_t*)(frame + 22)) >> 4;
            }
            ESP_LOGI(TAG, "seq_num:%d, total_num:%d\r\n", seq_buf, total_num);
        }

        switch (type) {
            case WIFI_PKT_MGMT:
                ESP_LOGI(TAG, "Rx mgmt pkt len:%d", len);
                break;

            case WIFI_PKT_CTRL:
                ESP_LOGI(TAG, "Rx ctrl pkt len:%d", len);
                break;

            case WIFI_PKT_DATA:
                ESP_LOGI(TAG, "Rx data pkt len:%d", len);
                break;

            case WIFI_PKT_MISC:
                 ESP_LOGI(TAG, "Rx misc pkt len:%d", len);             
                len = len > MAC_HEADER_LEN ? MAC_HEADER_LEN : len;
                break;

            default :
                len = 0;
                ESP_LOGE(TAG, "Rx unknown pkt len:%d", len);
                return;
        }

        ++seq_buf;

        if (total_num > 1) {
            *(uint16_t*)(frame + 22) = (seq_buf << 4) | (*(uint16_t*)(frame + 22) & 0xf);
        }
    }

    ESP_LOGI(TAG, "Rx ctrl header:");

    for (i = 0; i < 12; i++) {
        sprintf(printbuf + i * 3, "%02x ", *((uint8_t*)buf + i));
    }

    ESP_LOGI(TAG, "  - %s", printbuf);
    ESP_LOGI(TAG, "Data:");

    len = len > SCAN_DATA_LEN ? SCAN_DATA_LEN : len;

    for (i = 0; i < len; i++) {
        sprintf(printbuf + (i % 16) * 3, "%02x ", *((uint8_t*)frame + i));

        if ((i + 1) % 16 == 0) {
            ESP_LOGI(TAG, "  - %s", printbuf);
        }
    }

    if ((i % 16) != 0) {
        printbuf[((i) % 16) * 3 - 1] = 0;
        ESP_LOGI(TAG, "  - %s", printbuf);
    }
#endif
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

static void scan_task(void* pvParameters)
{
    struct scan_config_t* config = (struct scan_config_t*)pvParameters;
    wifi_promiscuous_filter_t filter = {0};
    filter.filter_mask = config->filter_mask;
    if(config->filter_frame_payload){
        extern esp_err_t esp_wifi_set_recv_data_frame_payload(bool enable_recv);
        ESP_ERROR_CHECK(esp_wifi_set_recv_data_frame_payload(true));
    }
    
    xEventGroupWaitBits(wifi_event_group, START_BIT,
                        false, true, portMAX_DELAY);
    ESP_ERROR_CHECK(esp_wifi_set_channel(config->channel, 0));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(scan_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    vTaskDelete(NULL);
}

static void scan_channel(struct scan_config_t* config)
{
    TaskHandle_t taskScanChannel;
    BaseType_t result = xTaskCreate(scan_task, "scan_task", 
        configMINIMAL_STACK_SIZE * 4, (void*)config, tskIDLE_PRIORITY + 4, &taskScanChannel);
    if(result != pdPASS)
    {
        ets_printf("Scan error, not created task: %d\n", result);
    }
}

static void setup_mac()
{
    uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};
    esp_base_mac_addr_set(&newMACAddress[0]);
}

void scan_init()
{
    setup_mac();
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_event_group = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    struct scan_config_t config = {0};
    config.channel = 1;
    //config.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT; //Receive management packets
    config.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL; //Receive ctrl packets
    config.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA; //Receive data packets
    config.filter_mask |= WIFI_PROMIS_FILTER_MASK_MISC;
    //config.filter_frame_payload = true; //Receive data frame payload

    scan_channel(&config);
}


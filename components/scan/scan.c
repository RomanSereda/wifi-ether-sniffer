#include "scan.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_libc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "parser.h"

static void promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    struct frame_data_t data = {0};
    parse(buf, type, &data);

    printf("ssid=%s, cn=%02d, rssi=%02d,"
		" dest=%02x:%02x:%02x:%02x:%02x:%02x,"
		" source=%02x:%02x:%02x:%02x:%02x:%02x,"
		" bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
		data.ssid,
		data.channel,
		data.rssi,
	
		data.dest[0],data.dest[1],data.dest[2],
		data.dest[3],data.dest[4],data.dest[5],
	
		data.source[0],data.source[1],data.source[2],
		data.source[3],data.source[4],data.source[5],
		
		data.bssid[0],data.bssid[1],data.bssid[2],
		data.bssid[3],data.bssid[4],data.bssid[5]
	);
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
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promiscuous_rx_cb));
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

static void setup_rand_mac()
{
    uint8_t mac_address[6];
    for (int i = 0; i < 6; i++) 
        mac_address[i] =  rand() % 256;

    ESP_ERROR_CHECK(esp_base_mac_addr_set(&mac_address[0]));
}

void scan_init()
{
    setup_rand_mac();
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
    config.channel = 8;
    config.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT; //Receive management packets
    //config.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL; //Receive ctrl packets
    //config.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA; //Receive data packets
    //config.filter_mask |= WIFI_PROMIS_FILTER_MASK_MISC;
    //config.filter_frame_payload = true; //Receive data frame payload

    scan_channel(&config);
}


#include "scan.h"
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
static void print_all_ssid_nodes()
{
    static int count;
    if(count != ssid_nodes_len())
    {
        struct node_ssid_t *node = ssid_root_node();
        while (node->id)
        {
            ets_printf("%u ch=%u, rssi=%02d, dest=%02x:%02x:%02x:%02x:%02x:%02x, id=%u, ->%s", 
                node->timestamp,
                node->channel,	
                node->rssi,	
                node->source[0],node->source[1],node->source[2],
		        node->source[3],node->source[4],node->source[5],
                node->id, node->ssid);

            ets_printf("\n");
            node = node->next;
        };

        ets_printf("\n");
        count = ssid_nodes_len();
    }
}
static uint32_t milis()
{
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}
static void promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    struct frame_data_t data = {0};
    if(parse(buf, &data)){
        containerize(&data, 0 /*setup_channel*/);
        print_all_ssid_nodes();
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

static void setup_rand_mac()
{
    uint8_t mac_address[6];
    for (int i = 0; i < 6; i++) 
        mac_address[i] =  rand() % 256;

    ESP_ERROR_CHECK(esp_base_mac_addr_set(&mac_address[0]));
}

void init_scan()
{
    setup_rand_mac();
    init_container();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    TaskHandle_t taskScanChannel;
    BaseType_t result = xTaskCreate(scan_task, "scan_task", 
        configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 1, &taskScanChannel);
    if(result != pdPASS)
    {
        ets_printf("Scan error, not created task: %d\n", result);
    }
}


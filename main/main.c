#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

#include "btn.h"
#include "container.h"
#include "scan_service.h"
#include "wifi_service.h"
#include "http_service.h"

static httpd_handle_t server = NULL;

static void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ets_printf("Stopping webserver");
        stop_http_service(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ets_printf("Starting webserver");
        *server = start_http_service();
    }
}

static bool is_web_running = false;
void start_web_service_ex()
{
    static char connection_name[32] = "";
    static char connection_passwd[32] = "";

    ESP_ERROR_CHECK(start_wifi_service(connection_name, connection_passwd));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    server = start_http_service();
    is_web_running = true;
}
void stop_web_service_ex()
{
    stop_http_service(server);
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler));
    ESP_ERROR_CHECK(stop_wifi_service());
    is_web_running = false;
}

static void led_blinking()
{
    while(true) { led_blink(MONO_BLINK);}
}

static void mode_change_handler()
{
    static bool mode = true;

    TaskHandle_t taskHandleBlink;
    BaseType_t result = xTaskCreate(led_blinking, "led_blinking", 
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &taskHandleBlink);
    if(result != pdPASS)
    {
        ets_printf("Blink error, not created task: %d\n", result);
    }

    if(mode){
        ets_printf("Scan mode setted. \n");
        if(is_web_running) stop_web_service_ex();
        vTaskMissedYield();
        ESP_ERROR_CHECK(start_scan_service());
    }
    else{
        ets_printf("Web view mode setted. \n");
        ESP_ERROR_CHECK(stop_scan_service());
        vTaskMissedYield();
        if(!is_web_running) start_web_service_ex();
    }
    mode = !mode;

    vTaskDelete(taskHandleBlink);
    turnoff_blink();
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_btn(&mode_change_handler);
    init_container();

    mode_change_handler();
}



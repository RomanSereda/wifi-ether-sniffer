#include "http_service.h"
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_http_server.h"

#include "container.h"

static const char * html_text_head = "\
<!DOCTYPE html><html lang='en'><head>\
  <title>Scan Table</title>\
  <meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>\
  <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/css/bootstrap.min.css'>\
  <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.6.3/jquery.min.js'></script>\
  <script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js'></script>\
</head>\
<body><div class='container'><h2>ESP8266 Scan Table</h2>\
  <table class='table table-condensed'>\
    <thead><tr><th>timestamp</th><th>channel</th><th>rssi</th><th>bssid</th><th>ssid</th></tr></thead><tbody>";

static const char * html_text_tail = "</tbody></table></div></body></html>";

static char* td_mean_rtd(char* mean)
{
    int len = strlen(mean) + 12;
    char* dest = memset(malloc(len), 0 , len);
    dest = strcat(dest, "<td>");
    dest = strcat(dest, mean);
    dest = strcat(dest, "</td>");

    free(mean);
    return dest;
}

static char* bssid2tr(struct node_ssid_t *node)
{
    const int bssid_len = 18 + 1;
    char* bssid = memset(malloc(bssid_len), 0 , bssid_len);

    sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", 
        node->source[0],node->source[1],node->source[2],
		node->source[3],node->source[4],node->source[5]);
    bssid = td_mean_rtd(bssid);
    return bssid;
}

static char* uint2tr(uint32_t uval)
{
    char* u = memset(malloc(12), 0 , 12);
    sprintf(u, "%u", uval);
    return td_mean_rtd(u);   
}

static char* int2tr(int32_t dval)
{
    char* d = memset(malloc(12), 0 , 12);
    sprintf(d, "%02d", dval);
    return td_mean_rtd(d);   
}

static char* text2tr(const char* str)
{
    char* text = (char*)memset(malloc(strlen(str)), 0 , strlen(str));
    sprintf(text, "%s", str);
    text = td_mean_rtd(text);
    return text;   
}

static char* tm2tr(const struct tm* timestamp)
{
    char* text = (char*)memset(malloc(10), 0 , 10);
    sprintf(text, "%02d:%02d:%02d", timestamp->tm_hour, 
        timestamp->tm_min, timestamp->tm_sec);

    text = td_mean_rtd(text);
    return text;   
}

static char* html_text = NULL;
void load_http_table()
{
    if(!ssid_nodes_len()) return;
    if(html_text) free(html_text);

    struct node_ssid_t *node = ssid_root_node();
    char* chunks = memset(malloc(1), 0 , 1);;
    while (node->id)
    {
        char* rssi      = int2tr(node->rssi);
        char* bssid     = bssid2tr(node);
        char* ssid      = text2tr(node->ssid);
        char* timestamp = tm2tr(&node->timestamp);
        char* channel;
        if(node->channel) channel = uint2tr(node->channel);
        channel = text2tr("none");

        int len = strlen(timestamp) + strlen(channel) 
            + strlen(rssi)+ strlen(bssid)+ strlen(ssid) + 12;

        chunks = (char*)realloc(chunks, len + strlen(chunks));

        chunks = strcat(chunks, "<tr>");
        chunks = strcat(chunks, timestamp); free(timestamp);
        chunks = strcat(chunks, channel);   free(channel);
        chunks = strcat(chunks, rssi);      free(rssi);
        chunks = strcat(chunks, bssid);     free(bssid);
        chunks = strcat(chunks, ssid);      free(ssid);
        chunks = strcat(chunks, "</tr>");

        node = node->next;
    };

    int len = strlen(chunks) + strlen(html_text_head) + strlen(html_text_tail);
    html_text = (char*)memset(malloc(len), 0 , len);
    html_text = strcat(html_text, html_text_head);
    html_text = strcat(html_text, chunks);
    html_text = strcat(html_text, html_text_tail);

    free(chunks);
}

static esp_err_t data_get_handler(httpd_req_t *req)
{
    if(ssid_nodes_len())
        httpd_resp_send(req, html_text, strlen(html_text));
    else{
        const char* text = "<div><h1 style='text-align: center;'>Empty Table</h1><h2 style='text-align: center;'>Need to continue scanning</h2></div>";
        httpd_resp_send(req, text, strlen(text));
    }
    return ESP_OK;
}

static httpd_uri_t data = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
};
httpd_handle_t start_http_service()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ets_printf("Starting server on port: '%d' \n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &data);
        return server;
    }

    ets_printf("Error starting server!\n");
    return NULL;
}

void stop_http_service(httpd_handle_t server)
{
    ets_printf("Http stoping ...\n");
    httpd_stop(server);
}

#pragma once
#include "esp_err.h"
typedef void* httpd_handle_t;

httpd_handle_t start_http_service();
void stop_http_service(httpd_handle_t server);

void list2table();


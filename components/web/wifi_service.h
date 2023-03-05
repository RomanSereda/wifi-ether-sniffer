#pragma once
#include "esp_err.h"

esp_err_t start_wifi_service(const char* ssid, const char* pswd);
esp_err_t stop_wifi_service();


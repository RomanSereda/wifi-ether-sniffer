#pragma once
#include "esp_wifi.h"

struct __attribute__((packed)) frame_data_t
{
    char ssid[32];
    uint8_t dest[6];       
    uint8_t source[6];             
    uint8_t bssid[6]; 
    uint16_t len;
    int8_t rssi;
    uint8_t channel; 
};

void parse(void* buf, wifi_promiscuous_pkt_type_t type, struct frame_data_t* frame_data);


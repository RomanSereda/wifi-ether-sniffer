#pragma once
#include <stdint.h>
#include <stdbool.h>

struct frame_data_t
{
    char ssid[32];
    uint8_t dest[6];       
    uint8_t source[6];             
    uint8_t bssid[6]; 
    uint16_t len;
    int8_t rssi;
    uint8_t channel; 
};

bool parse(void* buf, struct frame_data_t* frame_data);


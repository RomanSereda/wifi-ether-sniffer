#pragma once
#include <stdint.h>

struct node_data_t
{
    uint8_t bssid[6]; 
    uint8_t id;
    int8_t rssi;
};

void init_container();


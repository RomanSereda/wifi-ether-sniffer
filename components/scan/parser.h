#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FRAME_CONTROL_TYPE_MANAGEMENT 0

struct frame_data_t
{
    char ssid[32];
    uint8_t addr1[6];       
    uint8_t addr2[6];             
    uint8_t addr3[6]; 
    uint16_t len;
    int8_t rssi;
    uint8_t channel; 
    uint8_t to_ds;
	uint8_t from_ds;
    uint8_t type;
};

bool parse(void* buf, struct frame_data_t* frame_data);


#pragma once
#include <stdint.h>
#include <sys/time.h>
#include "parser.h"

struct node_data_t
{
    struct node_data_t* next;
    uint8_t bssid[6]; 
    uint8_t id;
    int8_t rssi;
};

struct ap_node_data_t
{
    struct ap_node_data_t* next;
    uint8_t bssid[6]; 
    uint8_t id;
    int8_t rssi;
};

void init_container();
void add_container_node(const struct frame_data_t* frame_data);
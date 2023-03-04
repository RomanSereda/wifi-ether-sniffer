#pragma once
#include <stdint.h>
#include <sys/time.h>
#include "parser.h"

struct node_ssid_t
{
    struct node_ssid_t* next;
    char ssid[32]; 
    uint8_t source[6];
    uint32_t id;
    time_t timestamp;
    int8_t rssi;
    uint8_t channel;
};

void init_container();
void containerize(const struct frame_data_t* frame_data, uint8_t external_channel);

struct node_ssid_t* ssid_root_node();
int ssid_nodes_len();


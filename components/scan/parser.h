#pragma once
#include "esp_wifi.h"

struct frame_data_t
{

};

void parse(void* buf, wifi_promiscuous_pkt_type_t type, struct frame_data_t* frame_data);


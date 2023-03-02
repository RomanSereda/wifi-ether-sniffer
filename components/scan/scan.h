#pragma once
#include <stdint.h>

struct scan_config_t
{
    int channel;
    uint32_t filter_mask;
    int filter_frame_payload;
};

void scan_init();

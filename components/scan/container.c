#include "container.h"
#include <string.h>
#include "freertos/FreeRTOS.h"

static uint32_t mac2id(const uint8_t* mac)
{
    uint32_t part;
    memcpy(&part, mac, 4);
    return part + mac[4] + mac[5];
}

static int ssidlen;
static struct node_ssid_t* root_node_ssid;
static struct node_ssid_t* last_node_ssid;
static struct node_ssid_t* create_node_ssid()
{
    struct node_ssid_t* p = malloc(sizeof(struct node_ssid_t));
    memset(p, 0, sizeof(struct node_ssid_t));
    return p;
}

void init_container()
{
    ssidlen = 0;
    root_node_ssid = create_node_ssid();
    last_node_ssid = root_node_ssid;
}

static bool is_contain_ssid(uint32_t id, struct node_ssid_t** finded)
{
    struct node_ssid_t* nb = root_node_ssid;
    while(nb->id) 
    { 
        if(nb->id == id){
            *finded = nb;
            return true;
        }
        nb = nb->next;
    };
    return false;
}

static void add_container_ssid(const struct frame_data_t* data, uint8_t external_channel)
{
    if(*((uint32_t*)&data->addr1[0]) == 0xffffffff 
            && *((uint32_t*)&data->addr3[0]) == 0xffffffff)
    {
        time_t timestamp;
        time(&timestamp);

        uint32_t id = mac2id(&data->addr2[0]);

        struct node_ssid_t* finded;
        if(!is_contain_ssid(id, &finded))
        {
            if(*((uint16_t*)&data->ssid[0]) == 0x00) return;

            last_node_ssid->id = id;
            last_node_ssid->rssi = data->rssi;
            last_node_ssid->timestamp = timestamp;
            memcpy(last_node_ssid->ssid, data->ssid, 32);
            memcpy(last_node_ssid->source, data->addr2, 6);
            last_node_ssid->next = create_node_ssid();

            last_node_ssid = last_node_ssid->next;
            ssidlen++;
        }
        else
        {
            finded->timestamp = timestamp;
            finded->rssi = data->rssi;
            
            if(data->channel) finded->channel = data->channel;
            else finded->channel = external_channel;
        }
    }
}

struct node_ssid_t* ssid_root_node()
{
    return root_node_ssid;
}
int ssid_nodes_len()
{
    if(!root_node_ssid->id) return 0;
    return ssidlen;
}

void containerize(const struct frame_data_t* frame_data, uint8_t external_channel)
{
    if(frame_data->type == FRAME_CONTROL_TYPE_MANAGEMENT){
        add_container_ssid(frame_data, external_channel);
    }
}

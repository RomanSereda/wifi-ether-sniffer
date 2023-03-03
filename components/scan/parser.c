#include "parser.h"
#include <stdint.h>
#include <string.h>
#include "esp_wifi.h"

#define FRAME_CONTROL_TYPE_MANAGEMENT 0
#define FRAME_CONTROL_SUBTYPE_PROBE_REQUEST 4

bool parse(void* buf, struct frame_data_t* frame_data)
{
	struct frame_ctrl_t
	{
		uint8_t protocol_version : 2;
		uint8_t type : 2;
		uint8_t subtype : 4;
		uint8_t to_ds : 1;
		uint8_t from_ds : 1;
		uint8_t more_frag : 1;
		uint8_t retry : 1;
		uint8_t pwr_mgt : 1;
		uint8_t more_data : 1;
		uint8_t protected_frame : 1;
		uint8_t order : 1;
	};

	typedef struct
    {
        struct frame_ctrl_t frame_ctrl;
        unsigned duration_id : 16;
        uint8_t addr1[6];         /* receiver address */
        uint8_t addr2[6];       /* sender address */
        uint8_t addr3[6];        /* filtering address */
        unsigned sequence_ctrl : 16;
    } ieee80211_mac_hdr_t;

    typedef struct
    {
        ieee80211_mac_hdr_t hdr;
        uint8_t payload[0]; 
    } ieee80211_packet_t;

	struct probe_tag
	{
		uint8_t tag_number;
		uint8_t tag_length;
		uint8_t data[];
	};

	const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
	const ieee80211_mac_hdr_t *hdr = &((ieee80211_packet_t*)pkt->payload)->hdr;

	if(hdr->frame_ctrl.type != FRAME_CONTROL_TYPE_MANAGEMENT) return false;

	memset(frame_data, 0, sizeof(struct frame_data_t));

	memcpy(frame_data->dest, hdr->addr1, 24);
	frame_data->rssi = (int8_t)pkt->rx_ctrl.rssi;
	frame_data->channel = pkt->rx_ctrl.channel;
	frame_data->len = pkt->rx_ctrl.sig_mode ? pkt->rx_ctrl.HT_length : pkt->rx_ctrl.legacy_length;

	if(hdr->frame_ctrl.subtype == FRAME_CONTROL_SUBTYPE_PROBE_REQUEST)
	{
		struct probe_tag *pt = (struct probe_tag*)((ieee80211_packet_t *)pkt->payload)->payload;
		if (pt->tag_number == 0 && pt->tag_length > 0 && pt->tag_length <= 32)
		{
			memcpy(frame_data->ssid, pt->data, pt->tag_length);
		}
	}

	return true;
}

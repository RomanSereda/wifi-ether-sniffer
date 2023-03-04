#include "parser.h"
#include <stdint.h>
#include <string.h>
#include "esp_wifi.h"

#define FRAME_CONTROL_SUBTYPE_PROBE_REQUEST 4

bool parse(void* buf, struct frame_data_t* frame_data)
{
	struct __attribute__((packed)) frame_ctrl_t
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

	struct __attribute__((packed)) ieee80211_mac_hdr_t
    {
        struct frame_ctrl_t frame_ctrl;
        unsigned duration_id : 16;
        uint8_t addr1[6];         /* receiver address */
        uint8_t addr2[6];         /* sender address */
        uint8_t addr3[6];       
        unsigned sequence_ctrl : 16;
    };

    struct __attribute__((packed)) ieee80211_packet_t
    {
        struct ieee80211_mac_hdr_t hdr;
        uint8_t payload[0]; 
    };

	struct __attribute__((packed)) probe_tag
	{
		uint8_t tag_number;
		uint8_t tag_length;
		uint8_t data[];
	};

	const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
	const struct ieee80211_mac_hdr_t *hdr = &((struct ieee80211_packet_t*)pkt->payload)->hdr;

	memset(frame_data, 0, sizeof(struct frame_data_t));
	memcpy(frame_data->addr1, hdr->addr1, 18);
	frame_data->rssi = (int8_t)pkt->rx_ctrl.rssi;
	frame_data->len = pkt->rx_ctrl.sig_mode ? pkt->rx_ctrl.HT_length : pkt->rx_ctrl.legacy_length;
	frame_data->type = hdr->frame_ctrl.type;

	if(frame_data->type != FRAME_CONTROL_TYPE_MANAGEMENT) return true;

	frame_data->to_ds = hdr->frame_ctrl.to_ds;
	frame_data->from_ds = hdr->frame_ctrl.from_ds;
	frame_data->channel = pkt->rx_ctrl.channel;
	if(hdr->frame_ctrl.subtype == FRAME_CONTROL_SUBTYPE_PROBE_REQUEST)
	{
		struct probe_tag *pt = (struct probe_tag*)((struct ieee80211_packet_t *)pkt->payload)->payload;
		if (pt->tag_number == 0 && pt->tag_length > 0 && pt->tag_length <= 32)
		{
			memcpy(frame_data->ssid, pt->data, pt->tag_length);
		}
	}

	return true;
}

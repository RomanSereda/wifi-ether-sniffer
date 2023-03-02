#include "parser.h"
#include <stdint.h>

static const char* wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
	switch(type) {
	case WIFI_PKT_MGMT: return "MGMT";
	case WIFI_PKT_DATA: return "DATA";
	default:	
	case WIFI_PKT_MISC: return "MISC";
	}
}

void parse(void* buf, wifi_promiscuous_pkt_type_t type, struct frame_data_t* frame_data)
{
    typedef struct
    {
        unsigned frame_ctrl : 16;
        unsigned duration_id : 16;
        uint8_t addr1[6]; /* receiver address */
        uint8_t addr2[6]; /* sender address */
        uint8_t addr3[6]; /* filtering address */
        unsigned sequence_ctrl : 16;
        uint8_t addr4[6]; /* optional */
    } wifi_ieee80211_mac_hdr_t;

    typedef struct
    {
        wifi_ieee80211_mac_hdr_t hdr;
        uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
    } wifi_ieee80211_packet_t;

    if (type != WIFI_PKT_MGMT)
		return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
	const wifi_ieee80211_packet_t *payload = (wifi_ieee80211_packet_t *)pkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &payload->hdr;

	printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
		" ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
		" ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
		" ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
		wifi_sniffer_packet_type2str(type),
		pkt->rx_ctrl.channel,
		pkt->rx_ctrl.rssi,
		/* ADDR1 */
		hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
		hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
		/* ADDR2 */
		hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
		hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
		/* ADDR3 */
		hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
		hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
	);

}

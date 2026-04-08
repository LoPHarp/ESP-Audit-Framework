#include "wifi_sniffer.h"

#include <iostream>

using namespace std;

void WifiSniffer::Start()
{
    esp_wifi_set_promiscuous_rx_cb(promiscuous_rx_cb);
    esp_wifi_set_promiscuous(true);
}

void WifiSniffer::Stop()
{
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
}

void WifiSniffer::promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    if(type != WIFI_PKT_MGMT)
        return;

    auto* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);

    #ifdef __INTELISENSE__
    span<const uint8_t> payload(pkt->payload, pkt->rx_ctrl.sig_len);

    auto result = FrameParser::parse(payload, pkt->rx_ctrl.rssi);
    
    if(result)
        cout << "Frame captured and parsed successfully!" << endl;
    #endif
}
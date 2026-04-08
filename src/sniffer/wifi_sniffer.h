#pragma once
#include "frame_parser.h"

#include "esp_wifi.h"

class WifiSniffer
{
public:
    void Start();
    void Stop();

private:
    static void promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type);
};
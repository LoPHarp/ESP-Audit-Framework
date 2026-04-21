#include "wifi_sniffer.hpp"
#include "../storage/AccessPointManager.hpp"

#include <iostream>

using namespace std;

WifiSniffer& WifiSniffer::GetInstance()
{
    static WifiSniffer instance;
    return instance;
}

void WifiSniffer::Start()
{
    if (isRunning_) return;
    isRunning_ = true;

    wifi_promiscuous_filter_t filter = {};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&filter);

    esp_wifi_set_promiscuous_rx_cb(promiscuous_rx_cb);
    esp_wifi_set_promiscuous(true);

    xTaskCreate(HopperTask, "HopperTask", 2048, this, 1, &hopperTaskHandle_);
}

void WifiSniffer::Stop()
{
    if (!isRunning_) return;
    isRunning_ = false;

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);

    if (hopperTaskHandle_ != nullptr)
    {
        vTaskDelete(hopperTaskHandle_);
        hopperTaskHandle_ = nullptr;
    }
}

void WifiSniffer::promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    if(type != WIFI_PKT_MGMT)
        return;

#ifdef __INTELISENSE__
    auto* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);

    span<const uint8_t> payload(pkt->payload, pkt->rx_ctrl.sig_len);

    auto result = FrameParser::parse(payload, pkt->rx_ctrl.rssi);
    
    if(result)
        AccessPointManager::GetInstance().AddOrUpdateAccessPoint(result.value());
#endif
}

void WifiSniffer::HopperTask(void* arg)
{
    uint8_t channel = 1;
    while (true)
    {
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        
        channel++;
        if (channel > 13) channel = 1;

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}
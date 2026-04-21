#pragma once
#include "frame_parser.hpp"

#include "esp_wifi.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
class WifiSniffer
{
public:
    static WifiSniffer& GetInstance();

    void Start();
    void Stop();

private:
    WifiSniffer() = default;
    ~WifiSniffer() = default;

    WifiSniffer(const WifiSniffer&) = delete;
    WifiSniffer& operator=(const WifiSniffer&) = delete;

    static void promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type);
    static void HopperTask(void* arg);

    TaskHandle_t hopperTaskHandle_ = nullptr;
    bool isRunning_ = false;
};
#pragma once

#include "../sniffer/sniffer.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <string_view>

constexpr uint8_t BEACON_SPAM_HOPPING = 0;

class BeaconSpamManager
{
public:
    static BeaconSpamManager& GetInstance();

    void Start(uint8_t channel = BEACON_SPAM_HOPPING);
    void Stop();
    
    bool IsActive() const;
    uint32_t GetPacketsSent() const;
    uint8_t GetTargetChannel() const;

private:
    BeaconSpamManager() = default;
    ~BeaconSpamManager() = default;

    BeaconSpamManager(const BeaconSpamManager&) = delete;
    BeaconSpamManager& operator=(const BeaconSpamManager&) = delete;

    static void SpamTask(void* arg);
    void SendFakeBeacon(std::string_view ssid, const MacAddress& bssid, uint8_t channel);

    std::atomic<bool> isActive_{false};
    std::atomic<uint32_t> packetsSent_{0};
    uint8_t targetChannel_{BEACON_SPAM_HOPPING}; 
    uint8_t currentHoppingChannel_{1}; // Для отслеживания текущего канала в режиме Hopping
    TaskHandle_t spamTaskHandle_{nullptr};
};
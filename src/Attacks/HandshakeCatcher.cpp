#include "HandshakeCatcher.hpp"
#include "PmkidManager.hpp"
#include <cstring>
#include <nvs.h>

using namespace std;

HandshakeCatcher& HandshakeCatcher::GetInstance()
{
    static HandshakeCatcher instance;
    return instance;
}

void HandshakeCatcher::Initialize()
{
    if (eapolQueue_ == nullptr)
    {
        eapolQueue_ = xQueueCreate(40, sizeof(EapolPacket));
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) 
    {
        uint8_t passiveFlag = 0;
        if (nvs_get_u8(nvs_handle, "pass_eapol", &passiveFlag) == ESP_OK) 
        {
            passiveCollection_.store(passiveFlag > 0);
        }
        nvs_close(nvs_handle);
    }
}

void HandshakeCatcher::SetPassiveCollection(bool enabled)
{
    passiveCollection_.store(enabled);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) 
    {
        nvs_set_u8(nvs_handle, "pass_eapol", enabled ? 1 : 0);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
}

bool HandshakeCatcher::IsPassiveCollectionEnabled() const
{
    return passiveCollection_.load();
}

void HandshakeCatcher::SetTarget(const MacAddress& apMac, const MacAddress& clientMac)
{
    portENTER_CRITICAL(&targetMux_);
    targetAp_ = apMac;
    targetClient_ = clientMac;
    hasTarget_ = true;
    portEXIT_CRITICAL(&targetMux_);
    
    targetEapolCount_.store(0);
}

void HandshakeCatcher::ClearTarget()
{
    portENTER_CRITICAL(&targetMux_);
    hasTarget_ = false;
    portEXIT_CRITICAL(&targetMux_);
    
    targetEapolCount_.store(0);
}

size_t HandshakeCatcher::GetTargetEapolCount() const
{
    return targetEapolCount_.load();
}

void HandshakeCatcher::ProcessEapolPacket(span<const uint8_t> rawPacket)
{
    if (rawPacket.size() < 16) return;

    MacAddress dstMac;
    MacAddress srcMac;
    MacAddress bssid;

    memcpy(dstMac.addr, rawPacket.data() + 4, 6);
    memcpy(srcMac.addr, rawPacket.data() + 10, 6);
    memcpy(bssid.addr, rawPacket.data() + 16, 6);

    bool localHasTarget;
    MacAddress localTargetAp;
    MacAddress localTargetClient;

    portENTER_CRITICAL_ISR(&targetMux_);
    localHasTarget = hasTarget_;
    localTargetAp = targetAp_;
    localTargetClient = targetClient_;
    portEXIT_CRITICAL_ISR(&targetMux_);

    bool isRelatedToTarget = false;

    if (localHasTarget)
    {
        MacAddress broadcastMac = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
        if (bssid == localTargetAp || srcMac == localTargetAp || dstMac == localTargetAp)
        {
            if (localTargetClient == broadcastMac)
                isRelatedToTarget = true;
            else if (srcMac == localTargetClient || dstMac == localTargetClient)
                isRelatedToTarget = true;
        }
    }

    if (localHasTarget && isRelatedToTarget)
        targetEapolCount_.fetch_add(1);

    bool shouldSave = passiveCollection_.load() || (localHasTarget && isRelatedToTarget);

    if (!shouldSave) return;

    sessionEapolCount_.fetch_add(1);

    if (rawPacket.size() <= 256 && eapolQueue_ != nullptr)
    {
        EapolPacket pkt;
        pkt.length = rawPacket.size();
        memcpy(pkt.data, rawPacket.data(), pkt.length);

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (xQueueSendFromISR(eapolQueue_, &pkt, &xHigherPriorityTaskWoken) == errQUEUE_FULL)
        {
            EapolPacket oldPkt;
            xQueueReceiveFromISR(eapolQueue_, &oldPkt, &xHigherPriorityTaskWoken);
            xQueueSendFromISR(eapolQueue_, &pkt, &xHigherPriorityTaskWoken);
        }

        PmkidManager::GetInstance().OnEapolReceived(srcMac);

        if (xHigherPriorityTaskWoken == pdTRUE)
            portYIELD_FROM_ISR();
    }
}

vector<vector<uint8_t>> HandshakeCatcher::GetAndClearBuffer()
{
    vector<vector<uint8_t>> packets;
    if (eapolQueue_ == nullptr) return packets;

    EapolPacket pkt;
    while (xQueueReceive(eapolQueue_, &pkt, 0) == pdTRUE)
        packets.emplace_back(pkt.data, pkt.data + pkt.length);

    return packets;
}

size_t HandshakeCatcher::GetBufferedBytesCount() const
{
    if (eapolQueue_ == nullptr) return 0;
    return uxQueueMessagesWaiting(eapolQueue_) * sizeof(EapolPacket);
}

size_t HandshakeCatcher::GetPacketsInQueue() const
{
    if (eapolQueue_ == nullptr) return 0;
    return uxQueueMessagesWaiting(eapolQueue_);
}

uint32_t HandshakeCatcher::GetSessionEapolCount() const
{
    return sessionEapolCount_.load();
}
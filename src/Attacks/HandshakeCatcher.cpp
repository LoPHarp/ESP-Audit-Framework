#include "HandshakeCatcher.hpp"
#include "PmkidManager.hpp"
#include <cstring>

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
        eapolQueue_ = xQueueCreate(30, sizeof(EapolPacket));
    }
}

void HandshakeCatcher::ProcessEapolPacket(span<const uint8_t> rawPacket)
{
    if (rawPacket.size() < 16) return; 

    if (rawPacket.size() <= 256 && eapolQueue_ != nullptr)
    {
        EapolPacket pkt;
        pkt.length = rawPacket.size();
        memcpy(pkt.data, rawPacket.data(), pkt.length);

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        if (xQueueSendFromISR(eapolQueue_, &pkt, &xHigherPriorityTaskWoken) == pdPASS)
        {
            MacAddress srcMac;
            memcpy(srcMac.addr, rawPacket.data() + 10, 6);
            
            PmkidManager::GetInstance().OnEapolReceived(srcMac);
        }

        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR();
        }
    }
}

vector<vector<uint8_t>> HandshakeCatcher::GetAndClearBuffer()
{
    vector<vector<uint8_t>> packets;
    if (eapolQueue_ == nullptr) return packets;

    EapolPacket pkt;
    while (xQueueReceive(eapolQueue_, &pkt, 0) == pdTRUE)
    {
        packets.emplace_back(pkt.data, pkt.data + pkt.length);
    }
    
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
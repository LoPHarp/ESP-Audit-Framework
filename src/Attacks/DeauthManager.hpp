#pragma once

#include "../sniffer/sniffer.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>

enum class AttackMode
{
    None,
    SingleTarget,
    BroadcastAP,
    SpamTarget,
    SpamAP,
    GlobalSpam
};

class DeauthManager
{
public:
    static DeauthManager& GetInstance();

    void SendDeauthPacket(const MacAddress& dest, const MacAddress& src, const MacAddress& bssid, uint16_t reasonCode = 7);

    void StartAttack(AttackMode mode, const MacAddress& apMac = {0}, uint8_t channel = 1, const MacAddress& clientMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
    void StopAttack();
    bool IsAttacking() const { return isAttacking_.load(); }

    uint32_t GetPacketsSent() const { return packetsSent_.load(); }
    AttackMode GetCurrentMode() const { return currentMode_; }
    uint8_t GetTargetChannel() const { return targetChannel_; }
private:
    DeauthManager() = default;
    ~DeauthManager() = default;

    DeauthManager(const DeauthManager&) = delete;
    DeauthManager& operator=(const DeauthManager&) = delete;

    static void AttackTask(void* arg);

    std::atomic<bool> isAttacking_{false};
    AttackMode currentMode_{AttackMode::None};
    MacAddress targetAp_;
    MacAddress targetClient_;
    uint8_t targetChannel_{1};
    TaskHandle_t attackTaskHandle_{nullptr};

    std::atomic<uint32_t> packetsSent_{0};
};
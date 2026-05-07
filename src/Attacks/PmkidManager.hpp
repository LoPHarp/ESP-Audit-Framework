#pragma once

#include "../sniffer/sniffer.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <string>

class PmkidManager
{
public:
    static PmkidManager& GetInstance();

    void StartAttack(const MacAddress& apMac, std::string_view ssid, uint8_t channel);
    void StopAttack();
    
    void OnEapolReceived(const MacAddress& srcMac);

    bool IsActive() const { return isActive_.load(); }
    bool IsHandshakeCaught() const { return handshakeCaught_.load(); }
    
    uint32_t GetAuthSent() const { return authSent_.load(); }
    uint32_t GetAssocSent() const { return assocSent_.load(); }
    const MacAddress& GetTargetAp() const { return targetAp_; }

private:
    PmkidManager() = default;
    ~PmkidManager() = default;

    PmkidManager(const PmkidManager&) = delete;
    PmkidManager& operator=(const PmkidManager&) = delete;

    static void AttackTask(void* arg);
    void SendAuthFrame(const MacAddress& apMac, const MacAddress& myMac);
    void SendAssocReqFrame(const MacAddress& apMac, const MacAddress& myMac, std::string_view ssid);

    std::atomic<bool> isActive_{false};
    std::atomic<bool> handshakeCaught_{false};

    std::atomic<uint32_t> authSent_{0};
    std::atomic<uint32_t> assocSent_{0};
    
    MacAddress targetAp_;
    std::string targetSsid_;
    uint8_t targetChannel_{1};
    
    TaskHandle_t attackTaskHandle_{nullptr};
};
#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../sniffer/sniffer.hpp"

struct EapolPacket
{
    uint8_t data[256];
    size_t length;
};

class HandshakeCatcher
{
public:
    static HandshakeCatcher& GetInstance();

    void Initialize();
    void ProcessEapolPacket(std::span<const uint8_t> rawPacket);
    std::vector<std::vector<uint8_t>> GetAndClearBuffer();

    size_t GetBufferedBytesCount() const;
    size_t GetPacketsInQueue() const;

    void SetPassiveCollection(bool enabled);
    bool IsPassiveCollectionEnabled() const;

    void SetTarget(const MacAddress& apMac, const MacAddress& clientMac);
    void ClearTarget();
    size_t GetTargetEapolCount() const;

    uint32_t GetSessionEapolCount() const;
private:
    HandshakeCatcher() = default;
    ~HandshakeCatcher() = default;

    HandshakeCatcher(const HandshakeCatcher&) = delete;
    HandshakeCatcher& operator=(const HandshakeCatcher&) = delete;

    QueueHandle_t eapolQueue_{nullptr};

    std::atomic<bool> passiveCollection_{false};
    std::atomic<size_t> targetEapolCount_{0};

    portMUX_TYPE targetMux_ = portMUX_INITIALIZER_UNLOCKED;
    bool hasTarget_{false};
    MacAddress targetAp_{0};
    MacAddress targetClient_{0};

    std::atomic<uint32_t> sessionEapolCount_{0};
};